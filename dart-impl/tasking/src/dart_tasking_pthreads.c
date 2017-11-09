#include <dash/dart/base/logging.h>
#include <dash/dart/base/atomic.h>
#include <dash/dart/base/assert.h>
#include <dash/dart/base/macro.h>
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
#include <dash/dart/tasking/dart_tasking_context.h>
#include <dash/dart/tasking/dart_tasking_cancellation.h>

#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include <errno.h>
#include <setjmp.h>



// true if threads should process tasks. Set to false to quit parallel processing
static volatile bool parallel         = false;
// true if the tasking subsystem has been initialized
static          bool initialized      = false;

static int num_threads;

// thread-private data
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
    .local_deps = NULL,
    .num_children = 0,
    .state  = DART_TASK_ROOT};

static void
destroy_threadpool(bool print_stats);

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

static void
invoke_taskfn(dart_task_t *task)
{
  DART_ASSERT(task->fn != NULL);
  DART_LOG_DEBUG("Invoking task %p (fn:%p data:%p)", task, task->fn, task->data);
  if (setjmp(task->cancel_return) == 0) {
    task->fn(task->data);
    DART_LOG_DEBUG("Done with task %p (fn:%p data:%p)", task, task->fn, task->data);
  } else {
    // we got here through longjmp, the task is cancelled
    task->state = DART_TASK_CANCELLED;
    printf("Thread %d: Task %p cancelled!\n", get_current_thread()->thread_id, task);
    DART_LOG_DEBUG("Task %p (fn:%p data:%p) cancelled", task, task->fn, task->data);
  }
}

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
  // save current task and requeue it if necessary
  dart_task_t *prev_task = get_current_task();
  if (prev_task->state == DART_TASK_SUSPENDED) {
    requeue_task(prev_task);
  }
  // update current task
  set_current_task(task);
  // invoke the new task
  invoke_taskfn(task);
  // return into the current thread's main context
  // this is not necessarily the thread that originally invoked the task
  dart_thread_t *thread = get_current_thread();
  dart__tasking__context_invoke(&thread->retctx);
}

static
void invoke_task(dart_task_t *task, dart_thread_t *thread)
{
  dart_task_t *current_task = get_current_task();

  if (task->taskctx == NULL) {
    // create a context for a task invoked for the first time
    task->taskctx = dart__tasking__context_create(
                      (context_func_t*)&wrap_task, task);
  }

  printf("Thread %d invoking task %p\n", thread->thread_id, task);

  if (current_task->state == DART_TASK_SUSPENDED) {
    // store current task's state and jump into new task
    dart__tasking__context_swap(current_task->taskctx, task->taskctx);
  } else {
    // store current thread's context and jump into new task
    dart__tasking__context_swap(&thread->retctx, task->taskctx);
  }
}


dart_ret_t
dart__tasking__yield(int delay)
{
  dart_thread_t *thread = get_current_thread();

  // progress
  dart_tasking_remote_progress();

  if (dart__tasking__cancellation_requested())
    dart__tasking__abort_current_task(thread);

  // first get a replacement task
  dart_task_t *next = next_task(thread);

  if (next) {
    // save the current task
    dart_task_t *current_task = dart_task_current_task();
    current_task->delay = delay;
    // mark task as suspended to avoid invoke_task to update the retctx
    // the next task should return to where the current task would have
    // returned
    current_task->state = DART_TASK_SUSPENDED;
    // set new task to running state, protected to prevent race conditions
    // with dependency handling code
    dart__base__mutex_lock(&(next->mutex));
    next->state = DART_TASK_RUNNING;
    dart__base__mutex_unlock(&(next->mutex));
    // here we leave this task
    invoke_task(next, thread);

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
  if (dart__tasking__cancellation_requested())
    dart__tasking__abort_current_task(thread);

  return DART_OK;
}


static
void invoke_task(dart_task_t *task, dart_thread_t *thread)
{
  // set new task
  set_current_task(task);

  //invoke the task function
  invoke_taskfn(task);
}
#endif // USE_UCONTEXT


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
      num_threads = hw.num_cores * ((hw.max_threads > 0) ? hw.max_threads : 1);
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
  // stop processing tasks if they are cancelled
  if (dart__tasking__cancellation_requested()) return NULL;

  dart_task_t *task = dart_tasking_taskqueue_pop(&thread->queue);
  if (task == NULL) {
    // try to steal from another thread, round-robbing starting at the last
    // successful thread
    int target = thread->last_steal_thread;
    for (int i = 0; i < num_threads; ++i) {
      task = dart_tasking_taskqueue_popback(&thread_pool[target].queue);
      if (task != NULL) {
        DART_LOG_DEBUG("Stole task %p from thread %i", task, i);
        thread->last_steal_thread = target;
        break;
      }
      target = (target + 1) % num_threads;
    }
  }
  return task;
}

static
dart_task_t * allocate_task()
{
  dart_task_t *task = malloc(sizeof(dart_task_t));
  dart__base__mutex_init(&task->mutex);

  return task;
}

static
dart_task_t * create_task(
  void (*fn) (void *),
  void             *data,
  size_t            data_size,
  dart_task_prio_t  prio)
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
  task->phase        = task->parent->state == DART_TASK_ROOT ?
                                                dart__tasking__phase_current()
                                                : DART_PHASE_ANY;
  task->has_ref      = false;
  task->remote_successor = NULL;
  task->local_deps   = NULL;
  task->prev         = NULL;
  task->successor    = NULL;
  task->prio         = prio;
  task->taskctx      = NULL;
  task->unresolved_deps = 0;
  task->unresolved_remote_deps = 0;

  return task;
}

void dart__tasking__destroy_task(dart_task_t *task)
{
  if (task->data_size) {
    free(task->data);
  }
  // reset some of the fields
  task->data             = NULL;
  task->data_size        = 0;
  task->fn               = NULL;
  task->parent           = NULL;
  task->prev             = NULL;
  task->remote_successor = NULL;
  task->successor        = NULL;
  task->state            = DART_TASK_DESTROYED;
  task->phase            = DART_PHASE_ANY;
  task->has_ref          = false;

  dart_tasking_datadeps_reset(task);

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

    // set task to running state, protected to prevent race conditions with
    // dependency handling code
    dart__base__mutex_lock(&(task->mutex));
    task->state = DART_TASK_RUNNING;
    dart__base__mutex_unlock(&(task->mutex));

    // start execution, change to another task in between
    invoke_task(task, thread);

    if (!dart__tasking__cancellation_requested()) {
      // Implicit wait for child tasks
      dart__tasking__task_complete();
    }

    // the task may have changed once we get back here
    task = get_current_task();

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

    // release the context
    dart__tasking__context_release(task->taskctx);
    task->taskctx = NULL;

    // clean up
    if (!task->has_ref){
      // only destroy the task if there are no references outside
      // referenced tasks will be destroyed in task_wait/task_freeref
      dart__tasking__destroy_task(task);
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

    // check whether cancellation has been activated
    dart__tasking__check_cancellation(thread);

    // look for incoming remote tasks and responses
    dart_tasking_remote_progress();
    // process the next task
    dart_task_t *task = next_task(thread);
    handle_task(task, thread);
  }

  DART_ASSERT_MSG(
    thread == get_current_thread(), "Detected invalid thread return!");

  // clean up the current thread's contexts before leaving
  dart__tasking__context_cleanup();

  DART_LOG_INFO("Thread %i exiting", dart__tasking__thread_num());

  return NULL;
}

static
void dart_thread_init(dart_thread_t *thread, int threadnum)
{
  thread->thread_id = threadnum;
  thread->current_task = NULL;
  thread->taskcntr  = 0;
  thread->ctxlist   = NULL;
  thread->last_steal_thread = 0;
  dart_tasking_taskqueue_init(&thread->queue);
  dart_tasking_taskqueue_init(&thread->defered_queue);
  DART_LOG_TRACE("Thread %i has task queue %p and deferred queue %p",
    threadnum, &thread->queue, &thread->defered_queue);

  printf("sizeof(dart_task_t) = %zu\n", sizeof(dart_task_t));
}

static
void dart_thread_finalize(dart_thread_t *thread)
{
  thread->thread_id = -1;
  thread->current_task = NULL;
  thread->ctxlist = NULL;
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

dart_ret_t
dart__tasking__init()
{
  if (initialized) {
    DART_LOG_ERROR("DART tasking subsystem can only be initialized once!");
    return DART_ERR_INVAL;
  }

  num_threads = determine_num_threads();
  DART_LOG_INFO("Using %i threads", num_threads);

  dart__tasking__context_init();

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

void
dart__tasking__enqueue_runnable(dart_task_t *task)
{
  if (dart__tasking__cancellation_requested()) {
    dart__tasking__cancel_task(task);
    return;
  }

  dart_thread_t *thread = get_current_thread();
  dart_taskqueue_t *q = &thread->queue;
  if (!dart__tasking__phase_is_runnable(task->phase)) {
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
    const dart_task_dep_t *deps,
          size_t           ndeps,
          dart_task_prio_t prio)
{
  if (dart__tasking__cancellation_requested()) {
    DART_LOG_DEBUG("dart__tasking__create_task: Ignoring task creation while "
                   "canceling tasks!");
    return DART_OK;
  }
  dart_task_t *task = create_task(fn, data, data_size, prio);

  int32_t nc = DART_INC_AND_FETCH32(&task->parent->num_children);
  DART_LOG_DEBUG("Parent %p now has %i children", task->parent, nc);

  dart_tasking_datadeps_handle_task(task, deps, ndeps);

  task->state = DART_TASK_CREATED;
  if (dart_tasking_datadeps_is_runnable(task)) {
    dart__tasking__enqueue_runnable(task);
  }

  return DART_OK;
}

dart_ret_t
dart__tasking__create_task_handle(
          void           (*fn) (void *),
          void            *data,
          size_t           data_size,
    const dart_task_dep_t *deps,
          size_t           ndeps,
          dart_task_prio_t prio,
          dart_taskref_t  *ref)
{
  if (dart__tasking__cancellation_requested()) {
    DART_LOG_DEBUG("dart__tasking__create_task_handle: Ignoring task creation "
                   "while canceling tasks!");
    return DART_OK;
  }
  dart_task_t *task = create_task(fn, data, data_size, prio);
  task->has_ref = true;

  int32_t nc = DART_INC_AND_FETCH32(&task->parent->num_children);
  DART_LOG_DEBUG("Parent %p now has %i children", task->parent, nc);

  dart_tasking_datadeps_handle_task(task, deps, ndeps);

  task->state = DART_TASK_CREATED;
  if (dart_tasking_datadeps_is_runnable(task)) {
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
    dart__tasking__phase_set_runnable(DART_PHASE_ANY);
    // make sure all incoming requests are served
    dart_tasking_remote_progress_blocking(DART_TEAM_ALL);
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

  // save context
  context_t tmpctx  = thread->retctx;

  while (DART_FETCH32(&(task->num_children)) > 0) {
    // a) look for incoming remote tasks and responses
    dart_tasking_remote_progress();
    // b) check cancellation
    dart__tasking__check_cancellation(thread);
    // c) process our tasks
    dart_task_t *next = next_task(thread);
    handle_task(next, thread);
  }

  // restore context (in case we're called from within another task)
  thread->retctx = tmpctx;

  dart__tasking__check_cancellation(thread);

  // 3) clean up if this was the root task and thus no other tasks are running
  if (thread->current_task == &(root_task)) {
    // recycled tasks can now be used again
    pthread_mutex_lock(&task_recycle_mutex);
    if (task_free_list == NULL) {
      task_free_list = task_recycle_list;
    } else {
      task_free_list->next = task_recycle_list;
    }
    task_recycle_list = NULL;
    pthread_mutex_unlock(&task_recycle_mutex);
    // reset the runnable phase
    dart__tasking__phase_set_runnable(DART_PHASE_FIRST);
  }
  dart_tasking_datadeps_reset(thread->current_task);


  return DART_OK;
}

dart_ret_t
dart__tasking__taskref_free(dart_taskref_t *tr)
{
  if (tr == NULL || *tr == NULL) {
    return DART_ERR_INVAL;
  }

  // free the task if already destroyed
  dart__base__mutex_lock(&(*tr)->mutex);
  if ((*tr)->state == DART_TASK_DESTROYED && (*tr)->has_ref) {
    dart__base__mutex_unlock(&(*tr)->mutex);
    dart__tasking__destroy_task(*tr);
    *tr = NULL;
    return DART_OK;
  }

  // the task is unfinished, just mark it as free'able
  (*tr)->has_ref = false;

  dart__base__mutex_unlock(&(*tr)->mutex);

  return DART_OK;

}

dart_ret_t
dart__tasking__task_wait(dart_taskref_t *tr)
{
  dart_thread_t *thread = get_current_thread();

  if (tr == NULL || *tr == NULL || (*tr)->state == DART_TASK_DESTROYED) {
    return DART_ERR_INVAL;
  }

  // the thread just contributes to the execution
  // of available tasks until the task waited on finishes
  while ((*tr)->state != DART_TASK_FINISHED) {
    dart_tasking_remote_progress();
    dart_task_t *task = next_task(thread);
    handle_task(task, thread);
    dart_tasking_remote_progress();
  }

  dart__tasking__destroy_task(*tr);
  *tr = NULL;

  return DART_OK;
}

dart_taskref_t
dart__tasking__current_task()
{
  return get_current_task();
}

dart_thread_t *
dart__tasking__current_thread()
{
  return get_current_thread();
}

/**
 * Tear-down related functions.
 */


static void
stop_threads()
{
  // wait for all threads to finish
  pthread_mutex_lock(&thread_pool_mutex);
  parallel = false;
  pthread_mutex_unlock(&thread_pool_mutex);

  // wake up all threads waiting for work
  pthread_cond_broadcast(&task_avail_cond);

  // wait for all threads to finish
  for (int i = 1; i < num_threads; i++) {
    pthread_join(thread_pool[i].pthread, NULL);
  }
}

static void
destroy_threadpool(bool print_stats)
{
  for (int i = 1; i < num_threads; i++) {
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

static void
free_tasklist(dart_task_t *tasklist)
{
  dart_task_t *task = tasklist;
  while (task != NULL) {
    dart_task_t *tmp = task;
    task = task->next;
    tmp->next = NULL;
    free(tmp);
  }
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

  free_tasklist(task_recycle_list);
  task_recycle_list = NULL;
  free_tasklist(task_free_list);
  task_free_list = NULL;
  stop_threads();
  dart_tasking_datadeps_fini();
  dart__tasking__context_cleanup();
  destroy_threadpool(true);

  initialized = false;
  DART_LOG_DEBUG("dart__tasking__fini(): Finished with tear-down");

  return DART_OK;
}
