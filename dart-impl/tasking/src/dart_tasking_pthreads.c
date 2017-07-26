
#include <dash/dart/base/logging.h>
#include <dash/dart/base/atomic.h>
#include <dash/dart/base/assert.h>
#include <dash/dart/if/dart_tasking.h>
#include <dash/dart/if/dart_active_messages.h>
#include <dash/dart/base/hwinfo.h>
#include <dash/dart/base/env.h>
#include <dash/dart/tasking/dart_tasking_priv.h>
#include <dash/dart/tasking/dart_tasking_ayudame.h>
#include <dash/dart/tasking/dart_tasking_taskqueue.h>
#include <dash/dart/tasking/dart_tasking_tasklist.h>
#include <dash/dart/tasking/dart_tasking_taskqueue.h>
#include <dash/dart/tasking/dart_tasking_datadeps.h>
#include <dash/dart/tasking/dart_tasking_remote.h>

#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include <errno.h>



// true if threads should process tasks. Set to false to quit parallel processing
static volatile bool parallel = false;

static int num_threads;

static size_t task_stack_size = DEFAULT_TASK_STACK_SIZE;

static bool initialized = false;

static pthread_key_t tpd_key;

typedef struct {
  int           thread_id;
} tpd_t;

static pthread_cond_t  task_avail_cond   = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t thread_pool_mutex = PTHREAD_MUTEX_INITIALIZER;

static dart_task_t *task_recycle_list     = NULL;
static dart_task_t *task_free_list        = NULL;
static pthread_mutex_t task_recycle_mutex = PTHREAD_MUTEX_INITIALIZER;

static dart_thread_t *thread_pool;

// a dummy task that serves as a root task for all other tasks
static dart_task_t root_task = {
    .next = NULL,
    .prev = NULL,
    .fn   = NULL,
    .data = NULL,
    .data_size = 0,
    .unresolved_deps = 0,
    .successor = NULL,
    .parent = NULL,
    .remote_successor = NULL,
    .num_children = 0,
    .epoch  = DART_EPOCH_ANY,
    .state  = DART_TASK_ROOT};

static void
destroy_threadpool(bool print_stats);

static void
init_threadpool();

static inline
void set_current_task(dart_task_t *t);

static inline
dart_task_t * get_current_task();

static inline
dart_thread_t * get_current_thread();

static
dart_task_t * next_task(dart_thread_t *thread);

static
void handle_task(dart_task_t *task, dart_thread_t *thread);

#ifdef USE_UCONTEXT

static
void requeue_task(dart_task_t *task)
{
  dart_thread_t *thread = get_current_thread();
  dart_taskqueue_t *q = &thread->queue;
  int delay = task->delay;
  if (delay == 0) {
    dart_tasking_taskqueue_push(q, task);
  } else if (delay > 0) {
    dart_tasking_taskqueue_insert(q, task, delay);
  } else {
    dart_tasking_taskqueue_pushback(q, task);
  }
}

static
void wrap_task(dart_task_t *task)
{
  DART_ASSERT(task != &root_task);
  // save current task and set to new task
  dart_task_t *prev_task = get_current_task();
  if (prev_task->state == DART_TASK_SUSPENDED) {
    requeue_task(prev_task);
  }
  // update current task
  set_current_task(task);
  // invoke the new task
  task->fn(task->data);
  // return into the current thread's context
  // this is not necessarily the thread that originally invoked the task
  dart_thread_t *thread = get_current_thread();
  setcontext(&thread->retctx);
}

static
void invoke_task(dart_task_t *task, dart_thread_t *thread)
{
  dart_task_t *current_task = get_current_task();
  if (current_task->state == DART_TASK_SUSPENDED) {
    // store current task's state and jump into new task
    swapcontext(&current_task->taskctx, &task->taskctx);
  } else {
    volatile int invoked = 0;
    // set thread's return context if this is not inside a yield
    getcontext(&thread->retctx);
    // do not invoke the task twice if returning to this context
    if (!invoked) {
      invoked = 1;
      // invoke the task
      setcontext(&task->taskctx);
    }
  }
}

dart_ret_t
dart__tasking__yield(int delay)
{
  dart_thread_t *thread = get_current_thread();

  // first get a replacement task
  dart_task_t *next = next_task(thread);

  if (next) {
    volatile int guard = 0;
    // save the current task
    dart_task_t *current_task = dart_task_current_task();
    current_task->delay = delay;
    getcontext(&current_task->taskctx);
    if (!guard) {
      guard = 1;
      // mark task as suspended to avoid invoke_task to update the retctx
      // the next task should return to where the current task would have
      // returned
      current_task->state = DART_TASK_SUSPENDED;
      // here we leave this task
      invoke_task(next, thread);
    }

    // requeue the previous task if necessary
    dart_task_t *prev_task = dart_task_current_task();
    if (prev_task->state == DART_TASK_SUSPENDED) {
      requeue_task(prev_task);
    }
    // resume this task
    current_task->state = DART_TASK_RUNNING;
    // reset to the resumed task and continue processing it
    set_current_task(current_task);
  }

  return DART_OK;
}

#else
dart_ret_t
dart__tasking__yield(int delay)
{
  // "nothing to be done here" (libgomp)
  // we do not execute another task to prevent serialization
  DART_LOG_INFO("Skipping dart__task__yield");
}

static
void invoke_task(dart_task_t *task, dart_thread_t *thread)
{
  // save current task and set to new task
  dart_task_t *current_task = get_current_task();
  set_current_task(task);

  dart_task_action_t fn = task->fn;
  void *data = task->data;

  DART_LOG_DEBUG("Invoking task %p (fn:%p data:%p)", task, task->fn, task->data);
  //invoke the task function
  DART_ASSERT(fn != NULL);
  fn(data);
  DART_LOG_DEBUG("Done with task %p (fn:%p data:%p)", task, fn, data);
}
#endif


static void wait_for_work()
{
  pthread_mutex_lock(&thread_pool_mutex);
  if (parallel) {
    pthread_cond_wait(&task_avail_cond, &thread_pool_mutex);
  }
  pthread_mutex_unlock(&thread_pool_mutex);
}

static int determine_num_threads()
{
  int num_threads = dart__base__env__num_threads();

  if (num_threads == -1) {
    // query hwinfo
    dart_hwinfo_t hw;
    dart_hwinfo(&hw);
    if (hw.num_cores > 0) {
      num_threads = hw.num_cores * (hw.max_threads > 0) ? hw.max_threads : 1;
      if (num_threads <= 0) {
        num_threads = -1;
      }
    }
  }

  if (num_threads == -1) {
    DART_LOG_WARN("Failed to get number of cores! Playing it safe with 2 threads...");
    num_threads = 2;
  }

  return num_threads;
}

static void destroy_tsd(void *tsd)
{
  free(tsd);
}

static inline
dart_thread_t * get_current_thread()
{
  tpd_t *tpd = (tpd_t*)pthread_getspecific(tpd_key);
  return &thread_pool[tpd->thread_id];
}

static inline
void set_current_task(dart_task_t *t)
{
  get_current_thread()->current_task = t;
}

static inline
dart_task_t * get_current_task()
{
  return get_current_thread()->current_task;
}

static
dart_task_t * next_task(dart_thread_t *thread)
{
  dart_task_t *task = dart_tasking_taskqueue_pop(&thread->queue);
  if (task == NULL) {
    // try to steal from another thread, round-robbing starting to the right
    for (int i  = (thread->thread_id + 1) % num_threads;
             i != thread->thread_id;
             i  = (i + 1) % num_threads) {
      task = dart_tasking_taskqueue_popback(&thread_pool[i].queue);
      if (task != NULL) {
        DART_LOG_DEBUG("Stole task %p from thread %i", task, i);
        break;
      }
    }
  }
  return task;
}

static
dart_task_t * allocate_task()
{
  dart_task_t *task = malloc(sizeof(dart_task_t) + task_stack_size);
  dart__base__mutex_init(&task->mutex);

#ifdef USE_UCONTEXT
  // initialize context and set stack
  getcontext(&task->taskctx);
  task->taskctx.uc_link           = NULL;
  task->taskctx.uc_stack.ss_sp    = (char*)(task + 1);
  task->taskctx.uc_stack.ss_size  = task_stack_size;
  task->taskctx.uc_stack.ss_flags = 0;
#endif

  return task;
}

static
dart_task_t * create_task(void (*fn) (void *), void *data, size_t data_size)
{
  dart_task_t *task = NULL;
  if (task_free_list != NULL) {
    pthread_mutex_lock(&task_recycle_mutex);
    if (task_free_list != NULL) {
      DART_STACK_POP(task_free_list, task);
    }
    pthread_mutex_unlock(&task_recycle_mutex);
  } else {
    task = allocate_task();
  }

  if (data_size) {
    task->data_size  = data_size;
    task->data       = malloc(data_size);
    memcpy(task->data, data, data_size);
  } else {
    task->data       = data;
    task->data_size  = 0;
  }
  task->fn           = fn;
  task->num_children = 0;
  task->parent       = get_current_task();
  task->state        = DART_TASK_NASCENT;
  task->epoch        = task->parent->state != DART_TASK_ROOT ?
                          task->parent->epoch : DART_EPOCH_ANY;
  task->has_ref      = false;
  task->remote_successor = NULL;
  task->prev         = NULL;
  task->successor    = NULL;
  task->unresolved_deps = 0;

#ifdef USE_UCONTEXT
  // set the stack guards
  char *stack = task->taskctx.uc_stack.ss_sp;
  *(uint64_t*)(stack) = 0xDEADBEEF;
  *(uint64_t*)(stack + task_stack_size - sizeof(uint64_t)) = 0xDEADBEEF;
  // create the new context
  typedef void (context_func_t) (void);
  makecontext(&task->taskctx, (context_func_t*)&wrap_task, 1, task);
#endif


  return task;
}

static inline
void destroy_task(dart_task_t *task)
{
  if (task->data_size) {
    free(task->data);
  }
  // reset some of the fields
  // IMPORTANT: the state may not be rewritten!
  task->data             = NULL;
  task->data_size        = 0;
  task->fn               = NULL;
  task->parent           = NULL;
  task->epoch            = DART_EPOCH_ANY;
  task->prev             = NULL;
  task->remote_successor = NULL;
  task->successor        = NULL;
  task->state            = DART_TASK_DESTROYED;
  task->has_ref          = false;


#ifdef USE_UCONTEXT
  // check the stack guards
  char *stack = task->taskctx.uc_stack.ss_sp;
  if (*(uint64_t*)(stack) != 0xDEADBEEF &&
      *(uint64_t*)(stack + task_stack_size - sizeof(uint64_t)) != 0xDEADBEEF)
  {
    DART_LOG_WARN(
        "Possible TASK STACK OVERFLOW detected! "
        "Consider changing the stack size via DART_TASK_STACKSIZE! "
        "(current stack size: %zu)", task_stack_size);
  }
#endif

  pthread_mutex_lock(&task_recycle_mutex);
  DART_STACK_PUSH(task_recycle_list, task);
  pthread_mutex_unlock(&task_recycle_mutex);
}

/**
 * Execute the given task.
 */
static
void handle_task(dart_task_t *task, dart_thread_t *thread)
{
  if (task != NULL)
  {
    DART_LOG_INFO("Thread %i executing task %p", thread->thread_id, task);

    dart_task_t *current_task = get_current_task();

    dart__base__mutex_lock(&(task->mutex));
    task->state = DART_TASK_RUNNING;
    dart__base__mutex_unlock(&(task->mutex));

    // start execution, might yield in between
    invoke_task(task, thread);

    // the task may have changed once we get back here
    task = get_current_task();

    // Implicit wait for child tasks
    dart__tasking__task_complete();

    // we need to lock the task shortly here
    // to allow for atomic check and update
    // of remote successors in dart_tasking_datadeps_handle_remote_task
    dart__base__mutex_lock(&(task->mutex));
    task->state = DART_TASK_FINISHED;
    dart__base__mutex_unlock(&(task->mutex));
    dart_tasking_datadeps_release_local_task(task);

    // let the parent know that we are done
    int32_t nc = DART_DEC_AND_FETCH32(&task->parent->num_children);
    DART_LOG_DEBUG("Parent %p has %i children left\n", task->parent, nc);

    // clean up
    if (!task->has_ref){
      // only destroy the task if there are no references outside
      // referenced tasks will be destroyed in task_wait
      destroy_task(task);
    }

    // return to previous task
    set_current_task(current_task);
    ++(thread->taskcntr);
  }
}


static
void* thread_main(void *data)
{
  DART_ASSERT(data != NULL);
  tpd_t *tpd = (tpd_t*)data;
  pthread_setspecific(tpd_key, tpd);

  dart_thread_t *thread = get_current_thread();

  set_current_task(&root_task);

  // enter work loop
  while (parallel) {
    // look for incoming remote tasks and responses
    dart_tasking_remote_progress();
    dart_task_t *task = next_task(thread);
    handle_task(task, thread);
    // only go to sleep if no tasks are in flight
//    if (DART_FETCH32(&(root_task.num_children)) == 0) {
      if (thread->thread_id == dart__tasking__num_threads() - 1)
      {
        // the last thread is responsible for ensuring progress on the
        // message queue even if all others are sleeping
//        DART_LOG_WARN("triggering remote progress");
//        dart_tasking_remote_progress();
      }
//      else {
//        wait_for_work();
//      }
//    }
  }

  DART_LOG_INFO("Thread %i exiting", dart__tasking__thread_num());

  return NULL;
}

static
void dart_thread_init(dart_thread_t *thread, int threadnum)
{
  thread->thread_id = threadnum;
  thread->current_task = NULL;
  thread->taskcntr  = 0;
  dart_tasking_taskqueue_init(&thread->queue);
  dart_tasking_taskqueue_init(&thread->defered_queue);
  DART_LOG_TRACE("Thread %i has task queue %p and deferred queue %p",
    threadnum, &thread->queue, &thread->defered_queue);
}

static
void dart_thread_finalize(dart_thread_t *thread)
{
  thread->thread_id = -1;
  thread->current_task = NULL;
  dart_tasking_taskqueue_finalize(&thread->queue);
  dart_tasking_taskqueue_finalize(&thread->defered_queue);
}


static void
init_threadpool(int num_threads)
{
  // initialize all task threads before creating them
  thread_pool = malloc(sizeof(dart_thread_t) * num_threads);
  for (int i = 0; i < num_threads; i++)
  {
    dart_thread_init(&thread_pool[i], i);
  }

  for (int i = 1; i < num_threads; i++)
  {
    tpd_t *tpd = malloc(sizeof(tpd_t));
    tpd->thread_id = i; // 0 is reserved for master thread
    int ret = pthread_create(&thread_pool[i].pthread, NULL, &thread_main, tpd);
    if (ret != 0) {
      DART_LOG_ERROR("Failed to create thread %i of %i!", i, num_threads);
    }
  }
}

dart_thread_t *
dart__tasking_current_thread()
{
  return &thread_pool[dart__tasking__thread_num()];
}


dart_ret_t
dart__tasking__init()
{
  if (initialized) {
    DART_LOG_ERROR("DART tasking subsystem can only be initialized once!");
    return DART_ERR_INVAL;
  }

  num_threads = determine_num_threads();
  DART_LOG_INFO("Using %i threads", num_threads);

#ifdef USE_UCONTEXT
  task_stack_size = dart__base__env__task_stacksize();
  if (task_stack_size == -1) task_stack_size = DEFAULT_TASK_STACK_SIZE;
  DART_LOG_INFO("Using per-task stack of size %zu", task_stack_size);
#else
  task_stack_size = 0;
#endif

  // keep threads running
  parallel = true;

  // set up the active message queue
  dart_tasking_datadeps_init();

  pthread_key_create(&tpd_key, &destroy_tsd);

  // initialize all task threads before creating them
  init_threadpool(num_threads);

  // set master thread id
  tpd_t *tpd = malloc(sizeof(tpd_t));
  tpd->thread_id = 0;
  pthread_setspecific(tpd_key, tpd);

  set_current_task(&root_task);

#ifdef DART_ENABLE_AYUDAME
  dart__tasking__ayudame_init();
#endif // DART_ENABLE_AYUDAME

  initialized = true;

  return DART_OK;
}

int
dart__tasking__thread_num()
{
  return (initialized ? ((tpd_t*)pthread_getspecific(tpd_key))->thread_id : 0);
}

int
dart__tasking__num_threads()
{
  return (initialized ? num_threads : 1);
}

int32_t
dart__tasking__epoch_bound()
{
  return root_task.epoch;
}

void
dart__tasking__enqueue_runnable(dart_task_t *task)
{
  dart_thread_t *thread = get_current_thread();
  dart_taskqueue_t *q = &thread->queue;
  int32_t bound = dart__tasking__epoch_bound();
  if (bound != DART_EPOCH_ANY && task->epoch > bound) {
    // if the task's phase is outside the phase bound we defer it
    q = &thread->defered_queue;
  }
  dart_tasking_taskqueue_push(q, task);
}

dart_ret_t
dart__tasking__create_task(
    void           (*fn) (void *),
    void            *data,
    size_t           data_size,
    dart_task_dep_t *deps,
    size_t           ndeps)
{
  dart_task_t *task = create_task(fn, data, data_size);

  int32_t nc = DART_INC_AND_FETCH32(&task->parent->num_children);
  DART_LOG_DEBUG("Parent %p now has %i children", task->parent, nc);

  dart_tasking_datadeps_handle_task(task, deps, ndeps);

  task->state = DART_TASK_CREATED;
  if (task->unresolved_deps == 0) {
    dart__tasking__enqueue_runnable(task);
  }

  return DART_OK;
}

dart_ret_t
dart__tasking__create_task_handle(
    void           (*fn) (void *),
    void            *data,
    size_t           data_size,
    dart_task_dep_t *deps,
    size_t           ndeps,
    dart_taskref_t  *ref)
{
  dart_task_t *task = create_task(fn, data, data_size);
  task->has_ref = true;

  int32_t nc = DART_INC_AND_FETCH32(&task->parent->num_children);
  DART_LOG_DEBUG("Parent %p now has %i children", task->parent, nc);

  dart_tasking_datadeps_handle_task(task, deps, ndeps);

  task->state = DART_TASK_CREATED;
  if (task->unresolved_deps == 0) {
    dart__tasking__enqueue_runnable(task);
  }

  *ref = task;

  return DART_OK;
}


dart_ret_t
dart__tasking__task_complete()
{
  dart_thread_t *thread = get_current_thread();

  DART_ASSERT_MSG(
    (thread->current_task != &(root_task) || thread->thread_id == 0),
    "Calling dart__tasking__task_complete() on ROOT task "
    "only valid on MASTER thread!");

  if (thread->current_task == &(root_task)) {
    // reset the active epoch
    root_task.epoch = DART_EPOCH_ANY;
    // once again make sure all incoming requests are served
    dart_tasking_remote_progress_blocking();
    // release unhandled remote dependencies
    dart_tasking_datadeps_release_unhandled_remote();
    DART_LOG_DEBUG("task_complete: releasing deferred tasks of all threads");
    // release the deferred queue
    for (int i = 0; i < num_threads; ++i) {
      dart_thread_t *t = &thread_pool[i];
      dart_tasking_taskqueue_move(&t->queue, &t->defered_queue);
    }
  }

  // 1) wake up all threads (might later be done earlier)
  pthread_cond_broadcast(&task_avail_cond);


  // 2) start processing ourselves
  dart_task_t *task = get_current_task();

  DART_LOG_DEBUG("dart__tasking__task_complete: waiting for children of task %p", task);

#ifdef USE_UCONTEXT
  // save context
  context_t tmpctx  = thread->retctx;
#endif
  while (DART_FETCH32(&(task->num_children)) > 0) {
//    DART_LOG_DEBUG("dart__tasking__task_complete: task %p has %lu tasks left", task, task->num_children);
    // a) look for incoming remote tasks and responses
// TODO: DEBUG
//    dart_tasking_remote_progress();
    // b) process our tasks
    dart_task_t *task = next_task(thread);
    handle_task(task, thread);
  }
#ifdef USE_UCONTEXT
  // restore context (in case we're called from within another task)
  thread->retctx = tmpctx;
#endif

  // 3) clean up if this was the root task and thus no other tasks are running
  if (thread->current_task == &(root_task)) {
    dart_tasking_datadeps_reset();
    // recycled tasks can now be used again
    pthread_mutex_lock(&task_recycle_mutex);
    task_free_list = task_recycle_list;
    task_recycle_list = NULL;
    pthread_mutex_unlock(&task_recycle_mutex);
  }

  return DART_OK;
}


dart_ret_t
dart__tasking__task_wait(dart_taskref_t *tr)
{
  dart_thread_t *thread = &thread_pool[dart__tasking__thread_num()];

  if (tr == NULL || *tr == NULL || (*tr)->state == DART_TASK_DESTROYED) {
    return DART_ERR_INVAL;
  }

  // the thread just contributes to the execution
  // of available tasks until the task waited on finishes
  while ((*tr)->state != DART_TASK_FINISHED) {
    dart_tasking_remote_progress();
    dart_task_t *task = next_task(thread);
    handle_task(task, thread);
  }

  destroy_task(*tr);
  *tr = NULL;

  return DART_OK;
}



dart_taskref_t
dart__tasking__current_task()
{
  return get_current_task();
}

static void
destroy_threadpool(bool print_stats)
{
  pthread_mutex_lock(&thread_pool_mutex);
  parallel = false;
  pthread_mutex_unlock(&thread_pool_mutex);

  // wake up all threads waiting for work
  pthread_cond_broadcast(&task_avail_cond);

  // wait for all threads to finish
  for (int i = 1; i < num_threads; i++) {
    pthread_join(thread_pool[i].pthread, NULL);
    dart_thread_finalize(&thread_pool[i]);
  }

#ifdef DART_ENABLE_LOGGING
  if (print_stats) {
    DART_LOG_INFO("######################");
    for (int i = 0; i < num_threads; ++i) {
      DART_LOG_INFO("Thread %i executed %lu tasks", i, thread_pool[i].taskcntr);
    }
    DART_LOG_INFO("######################");
  }
#endif // DART_ENABLE_LOGGING

  free(thread_pool);

  thread_pool = NULL;
}

dart_ret_t
dart__tasking__fini()
{
  if (!initialized) {
    DART_LOG_ERROR("DART tasking subsystem has not been initialized!");
    return DART_ERR_INVAL;
  }

  DART_LOG_DEBUG("dart__tasking__fini(): Tearing down task subsystem");

#ifdef DART_ENABLE_AYUDAME
  dart__tasking__ayudame_fini();
#endif // DART_ENABLE_AYUDAME

  dart_task_t *task = task_recycle_list;
  while (task != NULL) {
    dart_task_t *tmp = task;
    task = task->next;
    tmp->next = NULL;
    free(tmp);
  }
  task_recycle_list = NULL;

  destroy_threadpool(true);

  initialized = false;
  DART_LOG_DEBUG("dart__tasking__fini(): Finished with tear-down");

  return DART_OK;
}
