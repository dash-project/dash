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
#include <dash/dart/tasking/dart_tasking_affinity.h>
#include <dash/dart/tasking/dart_tasking_envstr.h>
#include <dash/dart/tasking/dart_tasking_wait.h>

#include <stdlib.h>
#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include <errno.h>
#include <setjmp.h>
#include <time.h>

#define CLOCK_TIME_USEC(ts) \
  ((ts).tv_sec*1E6 + (ts).tv_nsec/1E3)

#define CLOCK_DIFF_USEC(start, end)  \
  (uint64_t)((((end).tv_sec - (start).tv_sec)*1E6 + ((end).tv_nsec - (start).tv_nsec)/1E3))
// the grace period after which idle thread go to sleep
#define IDLE_THREAD_GRACE_USEC 1000
// the amount of usec idle threads should sleep within the grace period
#define IDLE_THREAD_GRACE_SLEEP_USEC 100
// the number of us a thread should sleep if IDLE_THREAD_SLEEP is not defined
#define IDLE_THREAD_DEFAULT_USLEEP 1000
// the number of tasks to wait until remote progress is triggered (10ms)
#define REMOTE_PROGRESS_INTERVAL_USEC  1E4

// true if threads should process tasks. Set to false to quit parallel processing
static volatile bool parallel         = false;
// true if the tasking subsystem has been initialized
static          bool initialized      = false;
// true if the worker threads are running (delayed thread-startup)
static          bool threads_running  = false;
// whether or not worker threads should poll for incoming remote messages
// Disabling this in the task setup phase might be beneficial due to
// MPI-internal congestion
static volatile bool worker_poll_remote = false;

static int num_threads;

// thread-private data
static pthread_key_t tpd_key;

// mutex and conditional variable to wait for tasks to get ready
static pthread_cond_t  task_avail_cond   = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t thread_pool_mutex = PTHREAD_MUTEX_INITIALIZER;

// task life-cycle lists
static dart_task_t *task_recycle_list     = NULL;
static dart_task_t *task_free_list        = NULL;
static pthread_mutex_t task_recycle_mutex = PTHREAD_MUTEX_INITIALIZER;

static dart_thread_t **thread_pool;

static bool bind_threads = false;

#ifndef DART_TASK_THREADLOCAL_Q
dart_taskqueue_t task_queue;
#endif // DART_TASK_THREADLOCAL_Q

enum dart_thread_idle_t {
  DART_THREAD_IDLE_POLL,
  DART_THREAD_IDLE_USLEEP,
  DART_THREAD_IDLE_WAIT
};

static struct dart_env_str2int thread_idle_env[] = {
  {"POLL",   DART_THREAD_IDLE_POLL},
  {"USLEEP", DART_THREAD_IDLE_POLL},
  {"WAIT",   DART_THREAD_IDLE_POLL},
};
static enum dart_thread_idle_t thread_idle_method;

static struct timespec thread_idle_sleeptime;

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

static
void remote_progress(dart_thread_t *thread, bool force);

static inline
double current_time_us() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return CLOCK_TIME_USEC(ts);
}

static void
invoke_taskfn(dart_task_t *task)
{
  DART_ASSERT(task->fn != NULL);
  DART_LOG_DEBUG("Invoking task %p (fn:%p data:%p)", task, task->fn, task->data);
  if (setjmp(task->taskctx->cancel_return) == 0) {
    task->fn(task->data);
    DART_LOG_DEBUG("Done with task %p (fn:%p data:%p)", task, task->fn, task->data);
  } else {
    // we got here through longjmp, the task is cancelled
    task->state = DART_TASK_CANCELLED;
    DART_LOG_DEBUG("Task %p (fn:%p data:%p) cancelled", task, task->fn, task->data);
  }
}

#ifdef USE_UCONTEXT

static
void requeue_task(dart_task_t *task)
{
#ifdef DART_TASK_THREADLOCAL_Q
  dart_thread_t *thread = get_current_thread();
  dart_taskqueue_t *q = &thread->queue;
#else
  dart_taskqueue_t *q = &task_queue;
#endif // DART_TASK_THREADLOCAL_Q
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
  } else if (prev_task->state == DART_TASK_BLOCKED) {
    dart__task__wait_enqueue(prev_task);
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

  if (current_task->state == DART_TASK_SUSPENDED ||
      current_task->state == DART_TASK_BLOCKED) {
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
  if (!threads_running) {
    // threads are not running --> no tasks to yield to
    return DART_OK;
  }

  bool check_requeue = false;

  dart_thread_t *thread = get_current_thread();
  // save the current task
  dart_task_t *current_task = dart_task_current_task();

  dart_task_t *next = NULL;

  if (dart__tasking__cancellation_requested())
    dart__tasking__abort_current_task(thread);

  if ( // check whether we got into this task from the root task
       // if so we jump back into the root_task
      (thread->yield_target == DART_YIELD_TARGET_ROOT &&
       current_task != &root_task) ||
       // check whether the current task is blocked
       // and jump back into the root-task if there is no replacement
      ((next = next_task(thread)) == NULL &&
        current_task->state == DART_TASK_BLOCKED)) {
    // we came into this task through a yield from the root_task or the
    // current task is blocked and there is no replacement task to yield to
    // so we just jump back into the root_task (only happens on the master thread)
    DART_ASSERT(thread->thread_id   == 0 ||
                current_task->state == DART_TASK_BLOCKED);

    // store current tasks's context and jump back into the master thread
    DART_LOG_TRACE("Yield: jumping back into master thread from task %p",
                   current_task);
    thread->yield_target = DART_YIELD_TARGET_YIELD;
    if (current_task->wait_handle == NULL) {
      current_task->state  = DART_TASK_SUSPENDED;
    } else {
      current_task->state  = DART_TASK_BLOCKED;
    }
    dart__tasking__context_swap(current_task->taskctx, &thread->retctx);
    // upon return into this task we may have to requeue the previous task
    check_requeue = true;
  } else {

    // progress
    remote_progress(thread, (next == NULL));
    if (next == NULL) {
      // try again
      next = next_task(thread);
    }

    if (next) {
      current_task->delay = delay;

      if (current_task == &root_task && thread->thread_id == 0) {
        // the master thread should return to the root_task on the next yield
        thread->yield_target = DART_YIELD_TARGET_ROOT;
        // NOTE: the root task is not suspended and requeued, the master thread
        //       will jump back into it (see above)
      } else {
        // mark task as suspended to avoid invoke_task to update the retctx
        // the next task should return to where the current task would have
        // returned
        if (current_task->wait_handle == NULL) {
          current_task->state  = DART_TASK_SUSPENDED;
        }
        thread->yield_target = DART_YIELD_TARGET_YIELD;
      }
      // set new task to running state, protected to prevent race conditions
      // with dependency handling code
      dart__base__mutex_lock(&(next->mutex));
      next->state = DART_TASK_RUNNING;
      dart__base__mutex_unlock(&(next->mutex));
      // here we leave this task
      DART_LOG_TRACE("Yield: yielding from task %p to next task %p",
                      current_task, next);
      invoke_task(next, thread);
      // upon return into this task we may have to requeue the previous task
      check_requeue = true;
    } else {
      DART_LOG_TRACE("Yield: no task to yield to from task %p",
                      current_task);
    }
  }

  if (check_requeue) {
    // we're coming back into this task here
    // requeue the previous task if necessary
    dart_task_t *prev_task = dart_task_current_task();
    if (prev_task->state == DART_TASK_SUSPENDED) {
      DART_LOG_TRACE("Yield: requeueing task %p from task %p",
                     prev_task, current_task);
      requeue_task(prev_task);
    } else if (prev_task->state == DART_TASK_BLOCKED) {
      dart__task__wait_enqueue(prev_task);
    }

    if (current_task != &root_task) {
      // resume this task
      current_task->state = DART_TASK_RUNNING;
    }
    // reset to the resumed task and continue processing it
    set_current_task(current_task);
  }
  return DART_OK;
}

#else
dart_ret_t
dart__tasking__yield(int delay)
{
  if (!threads_running) {
    // threads are not running --> no tasks to yield to
    return DART_OK;
  }

  // "nothing to be done here" (libgomp)
  // we do not execute another task to prevent serialization
  DART_LOG_INFO("Skipping dart__task__yield");
  // progress
  remote_progress(get_current_thread(), false);
  // check for abort
  if (dart__tasking__cancellation_requested())
    dart__tasking__abort_current_task(thread);

  return DART_OK;
}


static
void invoke_task(dart_task_t *task, dart_thread_t *thread)
{
  // set new task
  set_current_task(task);

  // allocate a context (required for setjmp)
  task->taskctx = dart__tasking__context_create(
                    (context_func_t*)&wrap_task, task);

  //invoke the task function
  invoke_taskfn(task);
}
#endif // USE_UCONTEXT


static void wait_for_work(enum dart_thread_idle_t method)
{
  if (method == DART_THREAD_IDLE_WAIT) {
    DART_LOG_TRACE("Thread %d going to sleep waiting for work",
                  get_current_thread()->thread_id);
    pthread_mutex_lock(&thread_pool_mutex);
    if (parallel) {
      pthread_cond_wait(&task_avail_cond, &thread_pool_mutex);
    }
    pthread_mutex_unlock(&thread_pool_mutex);
    DART_LOG_TRACE("Thread %d waking up", get_current_thread()->thread_id);
  } else if (method == DART_THREAD_IDLE_USLEEP) {
    nanosleep(&thread_idle_sleeptime, NULL);
  }
}

static void wakeup_thread_single()
{
  if (thread_idle_method == DART_THREAD_IDLE_WAIT) {
    pthread_mutex_lock(&thread_pool_mutex);
    pthread_cond_signal(&task_avail_cond);
    pthread_mutex_unlock(&thread_pool_mutex);
  }
}

static void wakeup_thread_all()
{
  if (thread_idle_method == DART_THREAD_IDLE_WAIT) {
    pthread_mutex_lock(&thread_pool_mutex);
    pthread_cond_broadcast(&task_avail_cond);
    pthread_mutex_unlock(&thread_pool_mutex);
  }
}

static int determine_num_threads()
{
  int num_threads = dart__base__env__number(DART_NUMTHREADS_ENVSTR, -1);

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

static inline
dart_thread_t * get_current_thread()
{
  return (dart_thread_t*)pthread_getspecific(tpd_key);
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
#ifdef DART_TASK_THREADLOCAL_Q
  dart_task_t *task = dart_tasking_taskqueue_pop(&thread->queue);
  if (task == NULL) {
    // try to steal from another thread, round-robbing starting at the last
    // successful thread
    int target = thread->last_steal_thread;
    for (int i = 0; i < num_threads; ++i) {
      dart_thread_t *target_thread = thread_pool[target];
      if (dart__likely(target_thread != NULL)) {
        task = dart_tasking_taskqueue_pop(&target_thread->queue);
        if (task != NULL) {
          DART_LOG_DEBUG("Stole task %p from thread %i", task, target);
          thread->last_steal_thread = target;
          break;
        }
      }
      target = (target + 1) % num_threads;
    }
  }
#else
  dart_task_t *task = dart_tasking_taskqueue_pop(&task_queue);
#endif // DART_TASK_THREADLOCAL_Q
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
  task->wait_handle  = NULL;

  return task;
}


void remote_progress(dart_thread_t *thread, bool force)
{
  if (force ||
      thread->last_progress_ts + REMOTE_PROGRESS_INTERVAL_USEC >= current_time_us())
  {
    dart_tasking_remote_progress();
    thread->last_progress_ts = current_time_us();
  }
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

    // we're coming back into this task here
    dart_task_t *prev_task = dart_task_current_task();
    if (prev_task->state == DART_TASK_BLOCKED) {
      // we came back here because there were no other tasks to yield from
      // the blocked task so we have to make sure this task is enqueued as
      // blocked (see dart__tasking__yield)
      dart__task__wait_enqueue(prev_task);
    } else {
      DART_ASSERT(prev_task->state == DART_TASK_RUNNING);
      if (!dart__tasking__cancellation_requested()) {
        // Implicit wait for child tasks
        dart__tasking__task_complete();
      }

      // the task may have changed once we get back here
      task = get_current_task();

      // we need to lock the task shortly here before releasing datadeps
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

      bool has_ref = task->has_ref;

      // clean up
      if (!has_ref){
        // only destroy the task if there are no references outside
        // referenced tasks will be destroyed in task_wait/task_freeref
        dart__tasking__destroy_task(task);
      }

    }
    // return to previous task
    set_current_task(current_task);
    ++(thread->taskcntr);
  }
}

static
void dart_thread_init(dart_thread_t *thread, int threadnum)
{
  thread->thread_id = threadnum;
  thread->current_task = &root_task;
  thread->taskcntr  = 0;
  thread->ctxlist   = NULL;
  thread->last_steal_thread = 0;
  thread->yield_target = DART_YIELD_TARGET_YIELD;
#ifdef DART_TASK_THREADLOCAL_Q
  dart_tasking_taskqueue_init(&thread->queue);
  DART_LOG_TRACE("Thread %i (%p) has task queue %p",
    threadnum, thread, &thread->queue);
#endif // DART_TASK_THREADLOCAL_Q

  if (threadnum == 0)
    DART_LOG_INFO("sizeof(dart_task_t) = %zu", sizeof(dart_task_t));
}

struct thread_init_data {
  pthread_t pthread;
  int       threadid;
};

static
void* thread_main(void *data)
{
  DART_ASSERT(data != NULL);
  struct thread_init_data* tid = (struct thread_init_data*)data;

  DART_LOG_INFO("Thread %d starting up", tid->threadid);

  if (bind_threads) {
    dart__tasking__affinity_set(tid->pthread, tid->threadid);
  }

  dart_thread_t *thread = malloc(sizeof(dart_thread_t));

  // populate the thread-private data
  int threadid    = tid->threadid;
  dart_thread_init(thread, threadid);
  thread->pthread = tid->pthread;
  free(tid);
  tid = NULL;

  // set thread-private data
  pthread_setspecific(tpd_key, thread);
  // make thread available to other threads
  thread_pool[threadid] = thread;

  set_current_task(&root_task);

  // cache the idle_method here to reduce NUMA effects
  enum dart_thread_idle_t idle_method = thread_idle_method;

  DART_LOG_INFO("Thread %d starting to process tasks", threadid);

  struct timespec begin_idle_ts;
  bool in_idle = false;
  // sleep-time: 100us
  const struct timespec sleeptime = {0, IDLE_THREAD_GRACE_SLEEP_USEC*1000};
  // enter work loop
  while (parallel) {

    // check whether cancellation has been activated
    dart__tasking__check_cancellation(thread);

    // process the next task
    dart_task_t *task = next_task(thread);
    handle_task(task, thread);

    // look for incoming remote tasks and responses
    // NOTE: only the first worker thread does the polling
    //       if polling is enabled or we have no runnable tasks anymore

    if ((task == NULL || worker_poll_remote) && threadid == 1) {
      remote_progress(thread, (task == NULL));
      dart__task__wait_progress();
      // wait for 100us to reduce pressure on master thread
      if (task == NULL) {
        nanosleep(&sleeptime, NULL);
      }
    } else if (task == NULL) {
      struct timespec curr_ts;
      if (!in_idle) {
        // start idle time
        clock_gettime(CLOCK_MONOTONIC, &begin_idle_ts);
        in_idle = true;
      } else {
        // check whether we should go to idle
        clock_gettime(CLOCK_MONOTONIC, &curr_ts);
        uint64_t idle_time = CLOCK_DIFF_USEC(begin_idle_ts, curr_ts);
        // go to sleep if we exceeded the max idle time
        if (idle_time > IDLE_THREAD_GRACE_USEC) {
          wait_for_work(idle_method);
          in_idle = false;
        }
      }
      // wait for 100us to reduce pressure on master thread
      nanosleep(&sleeptime, NULL);
    } else {
      in_idle = false;
    }
  }

  DART_ASSERT_MSG(
    thread == get_current_thread(), "Detected invalid thread return!");

  // clean up the current thread's contexts before leaving
  dart__tasking__context_cleanup();

  DART_LOG_INFO("Thread %i exiting", dart__tasking__thread_num());

  // unset thread-private data
  pthread_setspecific(tpd_key, NULL);

  return NULL;
}

static
void dart_thread_finalize(dart_thread_t *thread)
{
  thread->thread_id = -1;
  thread->current_task = NULL;
  thread->ctxlist = NULL;

#ifdef DART_TASK_THREADLOCAL_Q
  dart_tasking_taskqueue_finalize(&thread->queue);
#endif // DART_TASK_THREADLOCAL_Q
}

static void
start_threads(int num_threads)
{
  DART_ASSERT(!threads_running);
  DART_LOG_INFO("Starting %d threads", num_threads);

  // determine thread idle method
  uint64_t thread_idle_sleeptime_us =
                      dart__base__env__us(DART_THREAD_IDLE_SLEEP_ENVSTR,
                                          IDLE_THREAD_DEFAULT_USLEEP);
  if (thread_idle_method == DART_THREAD_IDLE_USLEEP) {
    thread_idle_sleeptime.tv_sec  = thread_idle_sleeptime_us / (1000*1000);
    thread_idle_sleeptime.tv_nsec =
        (thread_idle_sleeptime_us-(thread_idle_sleeptime.tv_sec*1000*1000))*1000;
    DART_LOG_INFO("Using idle thread method SLEEP with %lu sleep time",
                  thread_idle_sleeptime_us);
  } else {
    DART_LOG_INFO("Using idle thread method %s",
                  thread_idle_method == DART_THREAD_IDLE_POLL ? "POLL" : "WAIT");
  }

  // start-up all worker threads
  for (int i = 1; i < num_threads; i++)
  {
    // will be free'd by the thread
    struct thread_init_data *tid = malloc(sizeof(*tid));
    tid->threadid = i;
    int ret = pthread_create(&tid->pthread, NULL,
                             &thread_main, tid);
    if (ret != 0) {
      DART_LOG_ERROR("Failed to create thread %i of %i!", i, num_threads);
    }
  }
  threads_running = true;
}

static void
init_threadpool(int num_threads)
{
  dart__tasking__affinity_init();
  // bind the master thread before allocating meta-data objects
  if (bind_threads) {
    dart__tasking__affinity_set(pthread_self(), 0);
  }
  thread_pool = calloc(num_threads, sizeof(dart_thread_t*));
  dart_thread_t *master_thread = malloc(sizeof(dart_thread_t));
  // initialize master thread data, the other threads will do it themselves
  dart_thread_init(master_thread, 0);
  thread_pool[0] = master_thread;
}

dart_ret_t
dart__tasking__init()
{
  if (initialized) {
    DART_LOG_ERROR("DART tasking subsystem can only be initialized once!");
    return DART_ERR_INVAL;
  }

  thread_idle_method = dart__base__env__str2int(DART_THREAD_IDLE_ENVSTR,
                                                thread_idle_env,
                                                DART_THREAD_IDLE_USLEEP);

  num_threads = determine_num_threads();
  DART_LOG_INFO("Using %i threads", num_threads);

  DART_LOG_TRACE("root_task: %p", &root_task);

  dart__tasking__context_init();

#ifndef DART_TASK_THREADLOCAL_Q
  dart_tasking_taskqueue_init(&task_queue);
#endif // DART_TASK_THREADLOCAL_Q

  // keep threads running
  parallel = true;

  // set up the active message queue
  dart_tasking_datadeps_init();

  pthread_key_create(&tpd_key, NULL);

  bind_threads = dart__base__env__bool(DART_THREAD_AFFINITY_ENVSTR, false);

  // initialize all task threads before creating them
  init_threadpool(num_threads);

  // set master thread private data
  pthread_setspecific(tpd_key, thread_pool[0]);

  set_current_task(&root_task);

#ifdef DART_ENABLE_AYUDAME
  dart__tasking__ayudame_init();
#endif // DART_ENABLE_AYUDAME

  dart__task__wait_init();

  initialized = true;

  return DART_OK;
}

int
dart__tasking__thread_num()
{
  dart_thread_t *t = get_current_thread();
  return (dart__likely(t) ? t->thread_id : 0);
}

int
dart__tasking__num_threads()
{
  return (dart__likely(initialized) ? num_threads : 1);
}

void
dart__tasking__enqueue_runnable(dart_task_t *task)
{
  if (dart__tasking__cancellation_requested()) {
    dart__tasking__cancel_task(task);
    return;
  }

  bool enqueued = false;
  // check whether the task has to be deferred
  if (!dart__tasking__phase_is_runnable(task->phase)) {
    dart_tasking_taskqueue_lock(&local_deferred_tasks);
    if (!dart__tasking__phase_is_runnable(task->phase)) {
      DART_LOG_TRACE("Deferring release of task %p in phase %d",
                     task, task->phase);
      dart_tasking_taskqueue_push_unsafe(&local_deferred_tasks, task);
      enqueued = true;
    }
    dart_tasking_taskqueue_unlock(&local_deferred_tasks);
  }
  if (!enqueued){
#ifdef DART_TASK_THREADLOCAL_Q
    dart_thread_t *thread = get_current_thread();
    dart_taskqueue_t *q = &thread->queue;
#else
    dart_taskqueue_t *q = &task_queue;
#endif // DART_TASK_THREADLOCAL_Q
    dart_tasking_taskqueue_push(q, task);
    // wakeup a thread to execute this task
    wakeup_thread_single();
  }
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

  // start threads upon first task creation
  if (dart__likely(!threads_running)) {
    start_threads(num_threads);
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

  // start threads upon first task creation
  if (dart__likely(!threads_running)) {
    start_threads(num_threads);
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


void
dart__tasking__perform_matching(dart_thread_t *thread, dart_taskphase_t phase)
{
  // make sure all incoming requests are served
  dart_tasking_remote_progress_blocking(DART_TEAM_ALL);
  // release unhandled remote dependencies
  dart_tasking_datadeps_handle_defered_remote();
  DART_LOG_DEBUG("task_complete: releasing deferred tasks of all threads");
  // make sure all newly incoming requests are served
  dart_tasking_remote_progress_blocking(DART_TEAM_ALL);
  // reset the active epoch
  dart__tasking__phase_set_runnable(phase);
  // release the deferred queue
  dart_tasking_datadeps_handle_defered_local(thread);
  // wakeup all thread to execute potentially available tasks
  wakeup_thread_all();
}


dart_ret_t
dart__tasking__task_complete()
{
  if (!threads_running) {
    // threads are not running --> nothing to be done here
    return DART_OK;
  }

  dart_thread_t *thread = get_current_thread();

  DART_ASSERT_MSG(
    (thread->current_task != &(root_task) || thread->thread_id == 0),
    "Calling dart__tasking__task_complete() on ROOT task "
    "only valid on MASTER thread!");

  dart_taskphase_t entry_phase = dart__tasking__phase_current();

  if (thread->current_task == &(root_task) &&
      entry_phase > DART_PHASE_FIRST) {
    dart__tasking__perform_matching(thread, DART_PHASE_ANY);
    // enable worker threads to poll for remote messages
    worker_poll_remote = true;
  }

  // 1) wake up all threads (might later be done earlier)
  pthread_cond_broadcast(&task_avail_cond);


  // 2) start processing ourselves
  dart_task_t *task = get_current_task();

  DART_LOG_DEBUG("dart__tasking__task_complete: waiting for children of task %p", task);

  // save context
  context_t tmpctx  = thread->retctx;

  // main task processing routine
  while (DART_FETCH32(&(task->num_children)) > 0) {
    dart_task_t *next = next_task(thread);
    // a) look for incoming remote tasks and responses
    remote_progress(thread, (next == NULL));
    // b) check cancellation
    dart__tasking__check_cancellation(thread);
    // c) check whether blocked tasks are ready
    dart__task__wait_progress();
    // d) process our tasks
    handle_task(next, thread);
  }

  // restore context (in case we're called from within another task)
  thread->retctx = tmpctx;

  dart__tasking__check_cancellation(thread);

  // 3) clean up if this was the root task and thus no other tasks are running
  if (thread->current_task == &(root_task)) {
    if (entry_phase > DART_PHASE_FIRST) {
      // wait for all units to finish their tasks
      dart_tasking_remote_progress_blocking(DART_TEAM_ALL);
    }
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
    // disable remote polling of worker threads
    worker_poll_remote = false;
    // reset the phase counter
    dart__tasking__phase_reset();
  }
  dart_tasking_datadeps_reset(thread->current_task);


  return DART_OK;
}

dart_ret_t
dart__tasking__taskref_free(dart_taskref_t *tr)
{
  if (tr == NULL || *tr == DART_TASK_NULL) {
    return DART_ERR_INVAL;
  }

  // free the task if already destroyed
  dart__base__mutex_lock(&(*tr)->mutex);
  if ((*tr)->state == DART_TASK_FINISHED && (*tr)->has_ref) {
    dart__base__mutex_unlock(&(*tr)->mutex);
    dart__tasking__destroy_task(*tr);
    *tr = DART_TASK_NULL;
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

  if (tr == NULL || *tr == NULL || (*tr)->state == DART_TASK_DESTROYED) {
    return DART_ERR_INVAL;
  }

  dart_task_t *reftask = *tr;
  // the task has to be locked to avoid race conditions
  dart__base__mutex_lock(&(reftask->mutex));

  // the thread just contributes to the execution
  // of available tasks until the task waited on finishes
  while (reftask->state != DART_TASK_FINISHED) {
    dart__base__mutex_unlock(&(reftask->mutex));

    dart_thread_t *thread = get_current_thread();

    dart_tasking_remote_progress();
    dart_task_t *task = next_task(thread);
    handle_task(task, thread);

    // lock the task for the check in the while header
    dart__base__mutex_lock(&(reftask->mutex));
  }

  // finally we have to destroy the task
  dart__base__mutex_unlock(&(reftask->mutex));
  reftask->has_ref = false;
  dart__tasking__destroy_task(reftask);

  *tr = DART_TASK_NULL;

  return DART_OK;
}

dart_ret_t
dart__tasking__task_test(dart_taskref_t *tr, int *flag)
{
  if (flag == NULL) {
    return DART_ERR_INVAL;
  }
  *flag = 0;
  if (tr == NULL || *tr == NULL || (*tr)->state == DART_TASK_DESTROYED) {
    return DART_ERR_INVAL;
  }

  dart_task_t *reftask = *tr;
  // the task has to be locked to avoid race conditions
  dart__base__mutex_lock(&(reftask->mutex));
  dart_task_state_t state = reftask->state;
  dart__base__mutex_unlock(&(reftask->mutex));

  // if this is the only available thread we have to execute at least one task
  if (num_threads == 1 && state != DART_TASK_FINISHED) {
    dart_thread_t *thread = get_current_thread();
    dart_tasking_remote_progress();
    dart_task_t *task = next_task(thread);
    handle_task(task, thread);

    // check if this was our task
    dart__base__mutex_lock(&(reftask->mutex));
    state = reftask->state;
    dart__base__mutex_unlock(&(reftask->mutex));
  }

  if (state == DART_TASK_FINISHED) {
    *flag = 1;
    dart__tasking__destroy_task(reftask);
    *tr = DART_TASK_NULL;
  }
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

  // wake up all threads to finish
  wakeup_thread_all();

  // use a volatile pointer to wait for threads to set their data
  dart_thread_t* volatile *thread_pool_v = (dart_thread_t* volatile *)thread_pool;

  // wait for all threads to finish
  for (int i = 1; i < num_threads; i++) {
    // wait for the thread to populate it's thread data
    // just make sure all threads are awake
    wakeup_thread_all();
    while (thread_pool_v[i] == NULL) {}
    pthread_join(thread_pool_v[i]->pthread, NULL);
  }

  threads_running = false;
}

static void
destroy_threadpool(bool print_stats)
{
  for (int i = 1; i < num_threads; i++) {
    dart_thread_finalize(thread_pool[i]);
  }

#ifdef DART_ENABLE_LOGGING
  if (print_stats) {
    DART_LOG_INFO("######################");
    for (int i = 0; i < num_threads; ++i) {
      DART_LOG_INFO("Thread %i executed %lu tasks", i, thread_pool[i]->taskcntr);
    }
    DART_LOG_INFO("######################");
  }
#endif // DART_ENABLE_LOGGING

  // unset thread-private data
  pthread_setspecific(tpd_key, NULL);

  for (int i = 0; i < num_threads; ++i) {
    free(thread_pool[i]);
    thread_pool[i] = NULL;
  }

  free(thread_pool);
  thread_pool = NULL;
  dart__tasking__affinity_fini();
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
  if (threads_running) {
    stop_threads();
  }
  dart_tasking_datadeps_fini();
  dart__tasking__context_cleanup();
  destroy_threadpool(true);

#ifndef DART_TASK_THREADLOCAL_Q
  dart_tasking_taskqueue_finalize(&task_queue);
#endif

  dart__task__wait_fini();

  initialized = false;
  DART_LOG_DEBUG("dart__tasking__fini(): Finished with tear-down");

  return DART_OK;
}


