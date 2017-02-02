
#include <dash/dart/base/logging.h>
#include <dash/dart/base/atomic.h>
#include <dash/dart/if/dart_tasking.h>
#include <dash/dart/if/dart_active_messages.h>
#include <dash/dart/base/hwinfo.h>
#include <dash/dart/tasking/dart_tasking_priv.h>
#include <dash/dart/tasking/dart_tasking_ayudame.h>
#include <dash/dart/tasking/dart_tasking_taskqueue.h>
#include <dash/dart/tasking/dart_tasking_tasklist.h>
#include <dash/dart/tasking/dart_tasking_taskqueue.h>
#include <dash/dart/tasking/dart_tasking_datadeps.h>
#include <dash/dart/tasking/dart_tasking_remote.h>

#include <pthread.h>
#include <stdbool.h>
#include <time.h>
#include <errno.h>



// true if threads should process tasks. Set to false to quit parallel processing
static bool parallel;

static int num_threads;

static bool initialized = false;

static pthread_key_t tpd_key;

typedef struct {
  int           thread_id;
} tpd_t;

static pthread_cond_t task_avail_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t thread_pool_mutex = PTHREAD_MUTEX_INITIALIZER;

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
    .num_children = 0};


static void wait_for_work()
{
  pthread_cond_wait(&task_avail_cond, &thread_pool_mutex);
  DART_LOG_INFO("Thread %i received signal", dart__base__tasking__thread_num());
}

static void destroy_tsd(void *tsd)
{
  free(tsd);
}

static void set_current_task(dart_task_t *t)
{
  thread_pool[((tpd_t*)pthread_getspecific(tpd_key))->thread_id].current_task = t;
}

static dart_task_t * get_current_task()
{
  return thread_pool[((tpd_t*)pthread_getspecific(tpd_key))->thread_id].current_task;
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


/**
 * Execute the given task.
 */
static
void handle_task(dart_task_t *task)
{
  if (task != NULL)
  {
    DART_LOG_INFO("Thread %i executing task %p", dart__base__tasking__thread_num(), task);

    // save current task and set to new task
    dart_task_t *current_task = get_current_task();
    set_current_task(task);

    dart_task_action_t fn = task->fn;
    void *data = task->data;

    DART_LOG_DEBUG("Invoking task %p (fn:%p data:%p)", task, task->fn, task->data);
    //invoke the task function
    fn(data);
    DART_LOG_DEBUG("Done with task %p (fn:%p data:%p)", task, fn, data);

    // let the parent know that we are done
    int32_t nc = DART_FETCH_AND_DEC32(&task->parent->num_children);
    DART_LOG_DEBUG("Parent %p has %i children left\n", task->parent, nc);

    // clean up
    dart_tasking_datadeps_release_local_task(&thread_pool[dart__base__tasking__thread_num()], task);
    if (task->data_size) {
      free(task->data);
    }
    task->data = NULL;
    free(task);

    // return to previous task
    set_current_task(current_task);
  }
}


static
void* thread_main(void *data)
{
  tpd_t *tpd = (tpd_t*)data;
  pthread_setspecific(tpd_key, tpd);

  dart_thread_t *thread = &thread_pool[tpd->thread_id];

  set_current_task(&root_task);

  // enter work loop
  while (parallel) {
    // look for incoming remote tasks and responses
    dart_tasking_remote_progress();
    // only go to sleep if no tasks are in flight
    if (DART_FETCH_AND_ADD32(&(root_task.num_children), 0) == 0) {

      pthread_mutex_lock(&thread_pool_mutex);
      wait_for_work();
      pthread_mutex_unlock(&thread_pool_mutex);
    }
    dart_task_t *task = next_task(thread);
    handle_task(task);
  }

  return NULL;
}

static
void dart_thread_init(dart_thread_t *thread, int threadnum)
{
  thread->thread_id = threadnum;
  thread->current_task = NULL;
  dart_tasking_taskqueue_init(&thread->queue);
}

static
void dart_thread_finalize(dart_thread_t *thread)
{
  thread->thread_id = -1;
  thread->current_task = NULL;
  dart_tasking_taskqueue_finalize(&thread->queue);
}


dart_thread_t *
dart__base__tasking_current_thread()
{
  return &thread_pool[dart__base__tasking__thread_num()];
}


dart_ret_t
dart__base__tasking__init()
{
  dart_hwinfo_t hw;
  dart_hwinfo(&hw);
  if (hw.num_cores > 0) {
    num_threads = hw.num_cores * hw.max_threads;
  } else {
    DART_LOG_INFO("Failed to get number of cores! Playing it safe with 2 threads...");
    num_threads = 2;
  }
  DART_LOG_INFO("Creating %i threads", num_threads);

  dart_amsg_init();

  // keep threads running
  parallel = true;

  // set up the active message queue
  dart_tasking_datadeps_init();

  pthread_key_create(&tpd_key, &destroy_tsd);
  // set master thread id
  tpd_t *tpd = malloc(sizeof(tpd_t));
  tpd->thread_id = 0;
  pthread_setspecific(tpd_key, tpd);

  thread_pool = malloc(sizeof(dart_thread_t) * num_threads);
  dart_thread_init(&thread_pool[0], 0);
  set_current_task(&root_task);
  for (int i = 1; i < num_threads; i++)
  {
    tpd = malloc(sizeof(tpd_t));
    tpd->thread_id = i; // 0 is reserved for master thread
    dart_thread_init(&thread_pool[i], i);
    int ret = pthread_create(&thread_pool[i].pthread, NULL, &thread_main, tpd);
    if (ret != 0) {
      DART_LOG_ERROR("Failed to create thread %i of %i!", i, num_threads);
    }
  }

  dart__tasking__ayudame_init();

  initialized = true;

  return DART_OK;
}

int
dart__base__tasking__thread_num()
{
  return (initialized ? ((tpd_t*)pthread_getspecific(tpd_key))->thread_id : 0);
}

int
dart__base__tasking__num_threads()
{
  return (initialized ? num_threads : 1);
}


dart_ret_t
dart__base__tasking__create_task(void (*fn) (void *), void *data, size_t data_size, dart_task_dep_t *deps, size_t ndeps)
{
  // TODO: maybe use a free list?
  dart_task_t *task = malloc(sizeof(dart_task_t));
  if (data_size) {
    task->data_size = data_size;
    task->data = malloc(data_size);
    memcpy(task->data, data, data_size);
  } else {
    task->data = data;
    task->data_size = 0;
  }
  task->fn = fn;
  task->num_children = 0;
  task->parent = get_current_task();

  int32_t nc = DART_FETCH_AND_INC32(&task->parent->num_children);
  DART_LOG_DEBUG("Parent %p now has %i children", task->parent, nc);

  dart_tasking_datadeps_handle_task(task, deps, ndeps);

  if (task->unresolved_deps == 0) {
    dart_tasking_taskqueue_push(&thread_pool[dart__base__tasking__thread_num()].queue, task);

    // signal that a task is available
    // TODO: disabled for easier debugging
//    pthread_cond_signal(&task_avail_cond);
  }

  return DART_OK;
}


dart_ret_t
dart__base__tasking__task_complete()
{
  // TODO: How to determine that all tasks have successfully finished?

  // 1) wake up all threads (might later be done earlier)
  pthread_cond_broadcast(&task_avail_cond);

  dart_thread_t *thread = &thread_pool[dart__base__tasking__thread_num()];

  if (thread->current_task == &(root_task) && thread->thread_id != 0) {
    DART_LOG_ERROR("dart__base__tasking__task_complete() called on ROOT task only valid on MASTER thread!");
    return DART_ERR_INVAL;
  }

  // 2) start processing ourselves
  dart_task_t *task = get_current_task();
  while (DART_FETCH_AND_ADD32(&(task->num_children), 0) > 0) {
    // a) look for incoming remote tasks and responses
    dart_tasking_remote_progress();
    // b) process our tasks
    dart_task_t *task = next_task(thread);
    handle_task(task);
  }

  return DART_OK;
}


dart_ret_t
dart__base__tasking__fini()
{
  int i;

  parallel = false;

  for (i = 1; i < num_threads; i++) {
    pthread_cancel(thread_pool[i].pthread);
    dart_thread_finalize(&thread_pool[i]);
  }

  dart__tasking__ayudame_fini();

  initialized = false;

  return DART_OK;
}
