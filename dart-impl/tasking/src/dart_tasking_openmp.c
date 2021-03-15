

#include <dash/dart/base/logging.h>
#include <dash/dart/base/atomic.h>
#include <dash/dart/base/assert.h>
#include <dash/dart/base/macro.h>
#include <dash/dart/if/dart_tasking.h>
#include <dash/dart/if/dart_active_messages.h>
#include <dash/dart/base/hwinfo.h>
#include <dash/dart/base/env.h>
#include <dash/dart/base/stack.h>
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
#include <dash/dart/tasking/dart_tasking_copyin.h>
#include <dash/dart/tasking/dart_tasking_extrae.h>
#include <dash/dart/tasking/dart_tasking_craypat.h>

#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <errno.h>
#include <stddef.h>

#ifdef DART_TASKING_USE_OPENMP

#include <omp.h>

/**
 *  This to do:
 * - DONE: dispatch nested tasks directly to OpenMP
 * - detach detached tasks
 * - make sure we handle tasks with references correctly
 * - handle priorities correctly
 */

#define EVENT_ENTER(_ev) do {\
  EXTRAE_ENTER(_ev);         \
  CRAYPAT_ENTER(_ev); \
} while (0)

#define EVENT_EXIT(_ev) do {\
  EXTRAE_EXIT(_ev);         \
  CRAYPAT_EXIT(_ev); \
} while (0)

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


// we know that the stack member entry is the first element of the struct
// so we can cast directly
#define DART_TASKLIST_ELEM_POP(__freelist) \
  (dart_task_t*)((void*)dart__base__stack_pop(&__freelist))

#define DART_TASKLIST_ELEM_PUSH(__freelist, __elem) \
  dart__base__stack_push(&__freelist, &DART_STACK_MEMBER_GET(__elem))

// true if the tasking subsystem has been initialized
static          bool initialized      = false;

static int num_threads;

// whether or not to respect numa placement
static bool respect_numa = false;
// the number of numa nodes
static int num_numa_nodes = 1;

// task life-cycle list
static dart_stack_t task_free_list = DART_STACK_INITIALIZER;

static dart_thread_t **thread_pool;

static bool bind_threads = false;

#ifndef DART_TASK_THREADLOCAL_Q
static dart_taskqueue_t *task_queue;
#endif // DART_TASK_THREADLOCAL_Q

static size_t num_units;

// a dummy task that serves as a root task for all other tasks
static dart_task_t root_task = {
    .next             = NULL,
    .prev             = NULL,
    .flags            = 0,
    .fn               = NULL,
    .data             = NULL,
    .successor        = NULL,
    .parent           = NULL,
    .remote_successor = NULL,
    .local_deps       = NULL,
    .prio             = DART_PRIO_DEFAULT,
    .num_children     = 0,
    .state            = DART_TASK_ROOT,
    .descr            = "root_task"};

// thread-private data
static dart_thread_t* __tpd = NULL;
#pragma omp threadprivate(__tpd)

static void
destroy_threadpool(bool print_stats);

static inline
void set_current_task(dart_task_t *t);

static inline
dart_task_t * get_current_task();

static inline
dart_thread_t * get_current_thread();

static
void handle_task(dart_task_t *task, dart_thread_t *thread);

static
void remote_progress(dart_thread_t *thread, bool force);

static
void dart_thread_init(dart_thread_t *thread, int threadnum);

dart_task_t *
dart__tasking__root_task()
{
  return &root_task;
}

void
dart__tasking__mark_detached(dart_taskref_t task)
{
  LOCK_TASK(task);
  task->state = DART_TASK_DETACHED;
  UNLOCK_TASK(task);
}

void
dart__tasking__release_detached(dart_taskref_t task)
{
  DART_ASSERT(task->state == DART_TASK_DETACHED);

  dart_thread_t *thread = get_current_thread();

  dart_tasking_datadeps_release_local_task(task, thread);

  // we need to lock the task shortly here before releasing datadeps
  // to allow for atomic check and update
  // of remote successors in dart_tasking_datadeps_handle_remote_task
  LOCK_TASK(task);
  task->state = DART_TASK_FINISHED;
  bool has_ref = DART_TASK_HAS_FLAG(task, DART_TASK_HAS_REF);
  UNLOCK_TASK(task);

  dart_task_t *parent = task->parent;

  // signal release to OpenMP
#ifdef DART_OPENMP_HAVE_DETACH
  omp_fulfill_event(task->detach_handle);
#endif

  // clean up
  if (!has_ref){
    // only destroy the task if there are no references outside
    // referenced tasks will be destroyed in task_wait/task_freeref
    // TODO: this needs some more thoughts!
    dart__tasking__destroy_task(task);
  }


  // let the parent know that we are done
  int32_t nc = DART_DEC_AND_FETCH32(&parent->num_children);
  DART_LOG_DEBUG("Parent %p has %i children left\n", parent, nc);
}

static void
invoke_taskfn(dart_task_t *task)
{
  DART_ASSERT(task != NULL && task->fn != NULL);
  DART_LOG_DEBUG("Invoking task %p (fn:%p data:%p descr:'%s')",
                 task, task->fn, task->data, task->descr);
  if (setjmp(task->taskctx->cancel_return) == 0) {
    task->fn(task->data);
    DART_LOG_DEBUG("Done with task %p (fn:%p data:%p descr:'%s')",
                   task, task->fn, task->data, task->descr);
  } else {
    // we got here through longjmp, the task is cancelled
    task->state = DART_TASK_CANCELLED;
    DART_LOG_DEBUG("Task %p (fn:%p data:%p) cancelled", task, task->fn, task->data);
  }
}

static
void invoke_task(dart_task_t *task, dart_thread_t *thread)
{
  DART_LOG_TRACE("invoke_task: %p, cancellation %d", task, dart__tasking__cancellation_requested());
  if (!dart__tasking__cancellation_requested()) {
    if (task->taskctx == NULL) {
      DART_ASSERT(task->fn != NULL);
      // create a context for a task invoked for the first time, needed for longjmp
      task->taskctx = dart__tasking__context_create(NULL, task);
    }

    // update current task
    set_current_task(task);
    // invoke task function directly
    invoke_taskfn(task);
    DART_LOG_TRACE("Returning from task %p ('%s')", task, task->descr);
  } else {
    DART_LOG_TRACE("Skipping task %p because cancellation has been requested!",
                   task);

    // simply set the current task
    set_current_task(task);
  }
}

dart_ret_t
dart__tasking__yield(int delay)
{
  (void)delay;

  // save current task
  dart_task_t *task = get_current_task();

  if (task != dart__tasking__root_task()) {
    task->state = DART_TASK_SUSPENDED;
  }

#pragma omp taskyield

  if (task != dart__tasking__root_task()) {
    task->state = DART_TASK_RUNNING;
  }

  // set back to this task
  set_current_task(task);

  return DART_OK;
}

static inline
dart_thread_t * get_current_thread()
{
  // this happens only on the first invocation
  if (dart__unlikely(__tpd == NULL)) {
    __tpd = malloc(sizeof(dart_thread_t));
    dart_thread_init(__tpd, omp_get_thread_num());
    // register the thread for later clean-up
    thread_pool[omp_get_thread_num()] = __tpd;
  }
  return __tpd;
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

static void
schedule_runnable_tasks()
{
  dart_task_t *task = NULL;
  do {
    task = dart_tasking_taskqueue_pop(task_queue);
    if (task == NULL) {
      break;
    }

    // dispatch the task to the runtime
    dart__tasking__enqueue_runnable(task);
  } while (1);
}

static
dart_task_t * allocate_task()
{
  dart_task_t *task = DART_TASKLIST_ELEM_POP(task_free_list);

  if (task == NULL) {
    task = malloc(sizeof(dart_task_t));
    TASKLOCK_INIT(task);
  }

  return task;
}

static
dart_task_t * create_task(
  void (*fn) (void *),
  void             *data,
  size_t            data_size,
  dart_task_prio_t  prio,
  const char       *descr)
{
  dart_task_t *task = allocate_task();
  task->flags        = 0;
  task->remote_successor = NULL;
  task->local_deps    = NULL;
  task->prev          = NULL;
  task->successor     = NULL;
  task->fn            = fn;
  task->num_children  = 0;
  task->parent        = get_current_task();
  task->state         = DART_TASK_NASCENT;
  task->taskctx       = NULL;
  task->unresolved_deps = 0;
  task->unresolved_remote_deps = 0;
  task->deps_owned    = NULL;
  task->wait_handle   = NULL;
  task->numaptr       = NULL;

  if (data_size) {
    DART_TASK_SET_FLAG(task, DART_TASK_DATA_ALLOCATED);
    task->data           = malloc(data_size);
    memcpy(task->data, data, data_size);
  } else {
    task->data           = data;
    DART_TASK_UNSET_FLAG(task, DART_TASK_DATA_ALLOCATED);
  }

  if (task->parent->state == DART_TASK_ROOT) {
    task->phase      = dart__tasking__phase_current();
    dart__tasking__phase_add_task();
  } else {
    task->phase      = DART_PHASE_ANY;
  }

  //task->prio          = (prio == DART_PRIO_PARENT) ? task->parent->prio : prio;
  switch (prio) {
    case DART_PRIO_PARENT:
      task->prio       = task->parent->prio;
      break;
    case DART_PRIO_INLINE:
      task->prio       = DART_PRIO_HIGH;
      DART_TASK_SET_FLAG(task, DART_TASK_INLINE);
      break;
    default:
      task->prio       = prio;
      break;
  }

  // if descr is an absolute path (as with __FILE__) we only use the basename
  if (descr && descr[0] == '/') {
    const char *descr_base = strrchr(descr, '/');
    task->descr            = descr_base+1;
  } else {
    task->descr = descr;
  }

  return task;
}

void dart__tasking__destroy_task(dart_task_t *task)
{
  if (DART_TASK_HAS_FLAG(task, DART_TASK_DATA_ALLOCATED)) {
    DART_TASK_UNSET_FLAG(task, DART_TASK_DATA_ALLOCATED);
    free(task->data);
  }

  // take the task out of the phase
  if (dart__tasking__is_root_task(task->parent)) {
    dart__tasking__phase_take_task(task->phase);
  }

  // reset some of the fields
  task->data             = NULL;
  DART_TASK_UNSET_FLAG(task, DART_TASK_HAS_REF);
  task->fn               = NULL;
  task->parent           = NULL;
  task->prev             = NULL;
  task->remote_successor = NULL;
  task->successor        = NULL;
  task->state            = DART_TASK_DESTROYED;
  task->phase            = DART_PHASE_ANY;
  task->descr            = NULL;

  dart_tasking_datadeps_reset(task);

  DART_TASKLIST_ELEM_PUSH(task_free_list, task);
}

dart_task_t *
dart__tasking__allocate_dummytask()
{
  dart_task_t *task = allocate_task();
  memset(task, 0, sizeof(*task));
  task->state  = DART_TASK_DUMMY;
  task->parent = dart__tasking__current_task();

  if (task->parent->state == DART_TASK_ROOT) {
    task->phase      = dart__tasking__phase_current();
    dart__tasking__phase_add_task();
  } else {
    task->phase      = DART_PHASE_ANY;
  }
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


/**
 * Execute the given task.
 */
static
void handle_task(dart_task_t *task, dart_thread_t *thread)
{
  if (task != NULL)
  {
    DART_LOG_DEBUG("Thread %i executing task %p ('%s')",
                  thread->thread_id, task, task->descr);

    dart_task_t *current_task = get_current_task();

    DART_ASSERT_MSG(IS_ACTIVE_TASK(task), "Invalid state of task %p: %d",
                    task, task->state);

    // set task to running state, protected to prevent race conditions with
    // dependency handling code
    LOCK_TASK(task);
    task->state = DART_TASK_RUNNING;
    UNLOCK_TASK(task);

    // start execution, change to another task in between
    invoke_task(task, thread);

    // we're coming back into this task here
    dart_task_t *prev_task = dart_task_current_task();

    DART_LOG_TRACE("Returned from invoke_task(%p, %p): prev_task=%p, state=%d",
                   task, thread, prev_task, prev_task->state);

    if (prev_task->state == DART_TASK_DETACHED) {

      // release the context
      dart__tasking__context_release(task->taskctx);
      task->taskctx = NULL;
      dart__task__wait_enqueue(prev_task);

    } else {
      DART_ASSERT_MSG(prev_task->state == DART_TASK_RUNNING ||
                      prev_task->state == DART_TASK_CANCELLED,
                      "Unexpected task state: %d", prev_task->state);
      if (DART_FETCH32(&prev_task->num_children) &&
          !dart__tasking__cancellation_requested()) {
        // Implicit wait for child tasks
        dart__tasking__task_complete(true);
      }

      bool has_ref;

      // the task may have changed once we get back here
      task = get_current_task();

      DART_ASSERT(task != dart__tasking__root_task());

      dart_tasking_datadeps_release_local_task(task, thread);

      // we need to lock the task shortly here before releasing datadeps
      // to allow for atomic check and update
      // of remote successors in dart_tasking_datadeps_handle_remote_task
      LOCK_TASK(task);
      task->state = DART_TASK_FINISHED;
      has_ref = DART_TASK_HAS_FLAG(task, DART_TASK_HAS_REF);
      UNLOCK_TASK(task);

      // release the context
      dart__tasking__context_release(task->taskctx);
      task->taskctx = NULL;

      dart_task_t *parent = task->parent;

#if DART_OPENMP_HAVE_DETACH
      // release task in OpenMP
      omp_fulfill_event(task->detach_handle);
#endif

      // clean up
      if (!has_ref){
        // only destroy the task if there are no references outside
        // referenced tasks will be destroyed in task_wait/task_freeref
        // TODO: this needs some more thoughts!
        dart__tasking__destroy_task(task);
      }

      // let the parent know that we are done
      int32_t nc = DART_DEC_AND_FETCH32(&parent->num_children);
      DART_LOG_DEBUG("Parent %p has %i children left\n", parent, nc);
      ++(thread->taskcntr);
    }
    // return to previous task
    set_current_task(current_task);
  }
}

/**
 * Execute the given inlined task.
 * Tha task action will be called directly and no context will be created for it.
 */
static
void handle_inline_task(dart_task_t *task, dart_thread_t *thread)
{
  handle_task(task, thread);
}


void
dart__tasking__handle_task(dart_task_t *task)
{
  dart_thread_t *thread = dart__tasking__current_thread();
  if (DART_TASK_HAS_FLAG(task, DART_TASK_INLINE)) {
    handle_inline_task(task, thread);
  } else {
    handle_task(task, thread);
  }
}


static
void dart_thread_init(dart_thread_t *thread, int threadnum)
{
  thread->current_task = dart__tasking__root_task();
  dart__base__stack_init(&thread->ctxlist);
  thread->ctx_to_enter      = NULL;
  thread->last_progress_ts = 0;
  thread->taskcntr     = 0;
  thread->thread_id    = threadnum;
  thread->is_utility_thread = false;
}

struct thread_init_data {
  pthread_t pthread;
  int       threadid;
};

static
void dart_thread_finalize(dart_thread_t *thread)
{
  if (thread != NULL) {
    thread->thread_id = -1;
    thread->current_task = NULL;

#ifdef DART_TASK_THREADLOCAL_Q
    dart_tasking_taskqueue_finalize(&thread->queue);
#endif // DART_TASK_THREADLOCAL_Q
  }
}

static void
init_threadpool(int num_threads)
{
  thread_pool = calloc(num_threads, sizeof(dart_thread_t*));
  dart_thread_t *master_thread = malloc(sizeof(dart_thread_t));
  // initialize master thread data, the other threads will do it themselves
  dart_thread_init(master_thread, omp_get_thread_num());
  thread_pool[0] = master_thread;
}

dart_ret_t
dart__tasking__init()
{
  if (initialized) {
    DART_LOG_ERROR("DART tasking subsystem can only be initialized once!");
    return DART_ERR_INVAL;
  }

  respect_numa = dart__base__env__bool(DART_THREAD_PLACE_NUMA_ENVSTR, false);

  //printf("respect_numa: %d\n", respect_numa);

  num_threads = omp_get_max_threads();
  DART_LOG_INFO("Using %i threads", num_threads);

  DART_LOG_TRACE("root_task: %p", dart__tasking__root_task());

  dart__tasking__context_init();

  // initialize thread affinity
  dart__tasking__affinity_init();

#ifndef DART_TASK_THREADLOCAL_Q
  if (respect_numa) {
    num_numa_nodes = dart__tasking__affinity_num_numa_nodes();
  }
  task_queue = malloc(num_numa_nodes * sizeof(task_queue[0]));
  for (int i = 0; i < num_numa_nodes; ++i) {
    dart_tasking_taskqueue_init(&task_queue[i]);
  }
#endif // DART_TASK_THREADLOCAL_Q

  // set up the active message queue
  dart_tasking_datadeps_init();

  bind_threads = dart__base__env__bool(DART_THREAD_AFFINITY_ENVSTR, false);

  // initialize all task threads before creating them
  init_threadpool(num_threads);

  // set master thread private data
  __tpd = thread_pool[0];

  set_current_task(dart__tasking__root_task());

#ifdef DART_ENABLE_AYUDAME
  dart__tasking__ayudame_init();
#endif // DART_ENABLE_AYUDAME

  dart_team_size(DART_TEAM_ALL, &num_units);

  dart__task__wait_init();

  dart_tasking_copyin_init();

  dart__tasking__cancellation_init();

  initialized = true;

  return DART_OK;
}

int
dart__tasking__thread_num()
{
  return omp_get_thread_num();
}

int
dart__tasking__num_threads()
{
  return omp_get_max_threads();
}

int
dart__tasking__num_tasks()
{
  return root_task.num_children;
}


dart_taskqueue_t *
dart__tasking__get_taskqueue()
{
  return task_queue;
}

void
dart__tasking__enqueue_runnable(dart_task_t *task)
{
#if DART_OPENMP_HAVE_DETACH
#pragma omp task firstprivate(task) detach(task->detach_handle) untied
#else
#pragma omp task firstprivate(task) untied
#endif // DART_OPENMP_HAVE_DETACH
{
  handle_task(task, get_current_thread());
}
}

#ifdef DART_OPENMP_HAVE_DEPOBJS
static
void dispatch_to_openmp(
          void           (*fn) (void *),
          void            *data,
          size_t           data_size,
    const dart_task_dep_t *deps,
          size_t           ndeps,
          dart_task_prio_t prio,
    const char            *descr,
          dart_taskref_t  *ref)
{

  // collect dependencies into depobj
  omp_depend_t *depobjs = malloc(ndeps*sizeof(omp_depend_t));
  for (int i = 0; i < ndeps; ++i) {
    void *depptr = dart_tasking_datadeps_localize_gptr(deps[i].gptr).addr_or_offs.addr;
    if (deps[i].type == DART_DEP_IN) {
#pragma omp debobj(depobjs[i]) depend(in:depptr[0])
    } else {
#pragma omp debobj(depobjs[i]) depend(out:depptr[0])
    }
  }

  dart_task_t *task = create_task(fn, data, data_size, prio, descr);
  if (ref != NULL) {
    DART_TASK_SET_FLAG(task, DART_TASK_HAS_REF);
    *ref = task;
  }
#if DART_OPENMP_HAVE_DETACH
#pragma omp task depend(i=0:ndeps, depobj:depobjs[i]) \
                 firstprivate(task)                   \
                 detach(task->detach_handle)          \
                 untied
#else
#pragma omp task depend(i=0:ndeps, depobj:depobjs[i]) \
                 firstprivate(task)                   \
                 untied
#endif
{
  handle_task(task, get_current_thread());
}
  free(depobjs);

  int32_t nc = DART_INC_AND_FETCH32(&task->parent->num_children);
  DART_LOG_DEBUG("Parent %p now has %i children", task->parent, nc);
}
#endif // DART_OPENMP_HAVE_DEPOBJS

dart_ret_t
dart__tasking__create_task(
          void           (*fn) (void *),
          void            *data,
          size_t           data_size,
          dart_task_dep_t *deps,
          size_t           ndeps,
          dart_task_prio_t prio,
          int              flags,
    const char            *descr,
          dart_taskref_t  *ref)
{
  if (dart__tasking__cancellation_requested()) {
    DART_LOG_WARN("dart__tasking__create_task: Ignoring task creation while "
                  "canceling tasks!");
    return DART_OK;
  }

#ifdef DART_OPENMP_HAVE_DEPOBJS
  // direct the task directly to OpenMP if we're nested
  if (get_current_task() != &root_task) {
    dispatch_to_openmp(fn, data, data_size, deps, ndeps, prio, descr, ref);
    return DART_OK;
  }
#endif

  // we cannot dispatch to OpenMP directly so we have to handle the task ourselves

  dart_task_t *task = create_task(fn, data, data_size, prio, descr);

  if (flags & DART_TASK_NOYIELD) {
    DART_TASK_SET_FLAG(task, DART_TASK_INLINE);
  }

  if (ref != NULL) {
    DART_TASK_SET_FLAG(task, DART_TASK_HAS_REF);
    *ref = task;
  }

  int32_t nc = DART_INC_AND_FETCH32(&task->parent->num_children);
  DART_LOG_DEBUG("Parent %p now has %i children", task->parent, nc);

  dart_tasking_datadeps_handle_task(task, deps, ndeps);

  LOCK_TASK(task);
  task->state = DART_TASK_CREATED;
  bool is_runnable = dart_tasking_datadeps_is_runnable(task);
  UNLOCK_TASK(task);
  DART_LOG_TRACE("  Task %p ('%s') created: runnable %i, prio %d",
                 task, task->descr, is_runnable, task->prio);
  if (is_runnable) {
    dart__tasking__enqueue_runnable(task);
  }

  return DART_OK;
}

void
dart__tasking__perform_matching(dart_taskphase_t phase)
{
  if (num_units == 1) {
    // nothing to be done for one unit
    return;
  }
  //printf("Performing matching at phase %d\n", phase);
  // make sure all incoming requests are served
  dart_tasking_remote_progress_blocking(DART_TEAM_ALL);
  // release unhandled remote dependencies
  dart_tasking_datadeps_handle_defered_remote(phase);
  DART_LOG_DEBUG("task_complete: releasing deferred tasks of all threads");
  // make sure all newly incoming requests are served
  dart_tasking_remote_progress_blocking(DART_TEAM_ALL);
  // reset the active epoch
  dart__tasking__phase_set_runnable(phase);
  // release the deferred queue
  dart_tasking_datadeps_handle_defered_local();
}


dart_ret_t
dart__tasking__task_complete(bool local_only)
{
  dart_thread_t *thread = get_current_thread();

  DART_ASSERT_MSG(
    !(thread->current_task == &(root_task) && thread->thread_id != 0),
    "Calling dart__tasking__task_complete() on ROOT task "
    "only valid on MASTER thread!");

  DART_LOG_TRACE("Waiting for child tasks of %p to complete", thread->current_task);

  dart_taskphase_t entry_phase;

  bool is_root_task = thread->current_task == &(root_task);

  if (is_root_task && !local_only) {
    entry_phase = dart__tasking__phase_current();
    if (entry_phase > DART_PHASE_FIRST) {
      dart__tasking__perform_matching(DART_PHASE_ANY);
    }
  } else {
    EXTRAE_EXIT(EVENT_TASK);
  }


  // 2) start processing ourselves
  dart_task_t *task = get_current_task();

  DART_LOG_DEBUG("dart__tasking__task_complete: waiting for children of task %p", task);

  // main task processing routine
  while (task->num_children > 0) {
    // a) look for incoming remote tasks and responses
    // always trigger remote progress
    remote_progress(thread, (thread->thread_id == 0));
    // b) check cancellation
    dart__tasking__check_cancellation(thread);
    // d) process our tasks
    // for now we only yield, can we do something better?
    dart__tasking__yield(-1);
  }


  // 3) clean up if this was the root task and thus no other tasks are running
  if (is_root_task) {
    if (entry_phase > DART_PHASE_FIRST && !local_only) {
      // wait for all units to finish their tasks
      dart_tasking_remote_progress_blocking(DART_TEAM_ALL);
    }
    // reset the runnable phase
    dart__tasking__phase_set_runnable(DART_PHASE_FIRST);
    // reset the phase counter
    dart__tasking__phase_reset();
  } else {
    EXTRAE_ENTER(EVENT_TASK);
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
  LOCK_TASK(*tr);
  DART_TASK_UNSET_FLAG((*tr), DART_TASK_HAS_REF);
  if ((*tr)->state == DART_TASK_FINISHED) {
    UNLOCK_TASK(*tr);
    dart__tasking__destroy_task(*tr);
    *tr = DART_TASK_NULL;
    return DART_OK;
  }

  UNLOCK_TASK(*tr);

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
  LOCK_TASK(reftask);

  // the thread just contributes to the execution
  // of available tasks until the task waited on finishes
  while (reftask->state != DART_TASK_FINISHED) {
    UNLOCK_TASK(reftask);

    dart_thread_t *thread = get_current_thread();

    // look for incoming tasks
    remote_progress(thread, true);

    // schedule newly available tasks
    schedule_runnable_tasks();

    // work on tasks
    dart__tasking__yield(-1);

    // lock the task for the check in the while header
    LOCK_TASK(reftask);
  }

  // finally we have to destroy the task
  UNLOCK_TASK(reftask);
  DART_TASK_UNSET_FLAG(reftask, DART_TASK_HAS_REF);
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
  // TODO: is the lock really needed for reading?
  LOCK_TASK(reftask);
  dart_task_state_t state = reftask->state;
  UNLOCK_TASK(reftask);

  // if this is the only available thread we have to execute at least one task
  if (num_threads == 1 && state != DART_TASK_FINISHED) {
    dart_thread_t *thread = get_current_thread();
    // look for incoming tasks
    remote_progress(thread, true);

    // schedule newly available tasks
    schedule_runnable_tasks();

    // work on tasks
    dart__tasking__yield(-1);

    // check if this was our task
    LOCK_TASK(reftask);
    state = reftask->state;
    UNLOCK_TASK(reftask);
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
destroy_threadpool(bool print_stats)
{
  for (int i = 1; i < num_threads; i++) {
    dart_thread_finalize(thread_pool[i]);
  }

  // unset thread-private data
  __tpd = NULL;

  for (int i = 0; i < num_threads; ++i) {
    free(thread_pool[i]);
    thread_pool[i] = NULL;
  }

  free(thread_pool);
  thread_pool = NULL;
}

static void
free_tasklist(dart_stack_t *tasklist)
{
  dart_task_t *task;
  while (NULL != (task = DART_TASKLIST_ELEM_POP(*tasklist))) {
    free(task);
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

  free_tasklist(&task_free_list);

  dart_tasking_datadeps_fini();
  dart__tasking__context_cleanup();
  destroy_threadpool(true);

#ifndef DART_TASK_THREADLOCAL_Q
  for (int i = 0; i < num_numa_nodes; ++i) {
    dart_tasking_taskqueue_finalize(&task_queue[i]);
  }
#endif

  dart__tasking__affinity_fini();

  dart__task__wait_fini();

  dart_tasking_copyin_fini();

  dart_tasking_tasklist_fini();

  dart__tasking__cancellation_fini();

  initialized = false;
  DART_LOG_DEBUG("dart__tasking__fini(): Finished with tear-down");

  return DART_OK;
}

/**
 * Utility thread functions
 */

typedef struct utility_thread {
  void     (*fn) (void *);
  void      *data;
  pthread_t  pthread;
} utility_thread_t;

static void* utility_thread_main(void *data)
{
  utility_thread_t *ut = (utility_thread_t*)data;
  void     (*fn) (void *) = ut->fn;
  void      *fn_data      = ut->data;

  if (bind_threads) {
    dart__tasking__affinity_set_utility(ut->pthread, -1);
  }

  dart_thread_t *thread = calloc(1, sizeof(*thread));
  dart_thread_init(thread, -1);
  thread->is_utility_thread = true;

  __tpd = thread;

  free(ut);
  ut = NULL;

  //printf("Launching utility thread\n");
  // invoke the utility function
  fn(fn_data);

  free(thread);
  __tpd = NULL;

  // at some point we get back here and exit the thread
  return NULL;
}

void dart__tasking__utility_thread(
  void (*fn) (void *),
  void  *data)
{
  // will be free'd by the thread
  utility_thread_t *ut = malloc(sizeof(*ut));
  ut->fn = fn;
  ut->data = data;
  int ret = pthread_create(&ut->pthread, NULL, &utility_thread_main, ut);
  if (ret != 0) {
    DART_LOG_ERROR("Failed to create utility thread!");
  }
}

#endif // DART_TASKING_USE_OPENMP
