#ifndef DART__BASE__INTERNAL__TASKING_H__
#define DART__BASE__INTERNAL__TASKING_H__

#include <stdbool.h>
#include <pthread.h>
#include <dash/dart/if/dart_active_messages.h>
#include <dash/dart/if/dart_communication.h>
#include <dash/dart/if/dart_tasking.h>
#include <dash/dart/base/mutex.h>
#include <dash/dart/base/macro.h>
#include <dash/dart/tasking/dart_tasking_context.h>
#include <dash/dart/tasking/dart_tasking_phase.h>
#include <dash/dart/tasking/dart_tasking_envstr.h>

// forward declaration, defined in dart_tasking_datadeps.c
struct dart_dephash_elem;
struct task_list;

// whether to use thread-local task queues or a single queue
// (can be set from the command line or enforced here)
//#define DART_TASK_THREADLOCAL_Q

#ifdef USE_UCONTEXT
#define HAVE_RESCHEDULING_YIELD 1
#endif // USE_UCONTEXT

typedef enum {
  DART_TASK_ROOT     = -1, // special state assigned to the root task
  DART_TASK_FINISHED =  0, // comparison with 0
  DART_TASK_NASCENT,
  // active task states begin here
  // NOTE: check IS_ACTIVE_TASK macro if you make changes here!!
  DART_TASK_CREATED,
  DART_TASK_QUEUED,
  DART_TASK_DUMMY,         // the task is a dummy for a remote task
  DART_TASK_RUNNING,
  DART_TASK_SUSPENDED,     // the task is suspended but runnable
  DART_TASK_BLOCKED,       // the task is blocked waiting for a handle
  // active task states end here
  DART_TASK_DESTROYED,
  DART_TASK_CANCELLED
} dart_task_state_t;

typedef enum {
  DART_YIELD_TARGET_ROOT  = 0,  // upon yield, return to the root_task
  DART_YIELD_TARGET_YIELD = 1, // upon yield, yield to another task
} dart_yield_target_t;

#define IS_ACTIVE_TASK(task) \
  ((task)->state >= DART_TASK_CREATED   && \
   (task)->state <= DART_TASK_BLOCKED)


typedef
struct dart_wait_handle_s dart_wait_handle_t;

//#define USE_DART_MUTEX

#ifdef USE_DART_MUTEX
typedef dart_mutex_t tasklock_t;
#define TASKLOCK_INITIALIZER DART_MUTEX_INITIALIZER
#define TASKLOCK_INIT(__task) do { \
 static dart_mutex_t tmp = DART_MUTEX_INITIALIZER; \
 (__task)->lock = tmp; \
} while (0)
#define LOCK_TASK(__task) dart__base__mutex_lock(&(__task)->lock)
#define UNLOCK_TASK(__task) dart__base__mutex_unlock(&(__task)->lock)
# else
typedef int32_t tasklock_t;
#define TASKLOCK_INITIALIZER ((int32_t)0)
#define TASKLOCK_INIT(__task) do { \
  __task->lock = TASKLOCK_INITIALIZER;\
} while (0)
#define LOCK_TASK(__task) do {\
  int cnt = 0; \
  while (DART_COMPARE_AND_SWAP32(&(__task)->lock, 0, 1) != 0) \
  { if (++cnt == 1000) { sched_yield(); cnt = 0; } } \
} while(0)
#define UNLOCK_TASK(__task) do {\
  DART_ASSERT(DART_FETCH_AND_DEC32(&(__task)->lock) == 1); \
} while(0)
#endif // USE_DART_MUTEX

typedef struct task_exec_state {
  context_t                 *taskctx;         // context to start/resume task
  struct dart_dephash_elem **local_deps;      // hashmap containing dependencies of child tasks
  dart_wait_handle_t        *wait_handle;
  struct dart_task_data     *recycle_tasks;   // list of destroyed child tasks
  dart_task_t               *task;            // reference to the task
  tasklock_t                 lock;            // lock to lock the execution state
  int                        delay;           // delay in case this task yields
  int                        num_children;
} task_exec_state_t;


/**
 * Structure describing a task. This is all the data that is required to describe
 * the static properties of the task, i.e., it's action and dependencies.
 * The dynamic part is outsourced to the \c task_exec_state.
 *
 * NOTE: the size has been carefully reduced to 128B (2 cache lines).
 *       Be careful when you add fields!
 *
 * TODO: Can we get rid of of the mutex (>50B), e.g., using an atomic flag?
 *
 */
struct dart_task_data {
  struct dart_task_data     *next;            // next entry in a task list/queue
  struct dart_task_data     *prev;            // previous entry in a task list/queue
  int32_t                    unresolved_deps; // the number of unresolved task dependencies
  int32_t                    unresolved_remote_deps; // the number of unresolved remote task dependencies
  struct task_list          *successor;       // the list of tasks that depend on this task
  struct dart_dephash_elem  *remote_successor;
  const char                *descr;           // the description of the task
  tasklock_t                 lock;
  dart_taskphase_t           phase;
  bool                       has_ref;
  bool                       data_allocated;  // whether the data was allocated and copied
  int8_t                     state;           // one of dart_task_state_t, single byte sufficient
  int8_t                     prio;
  task_exec_state_t         *parent;          // the task that created this task
  task_exec_state_t         *exec;            // the dynamic state of the task
  union {
    // used for dummy tasks
    struct {
      void*                  remote_task;     // the remote task (do not deref!)
      dart_global_unit_t     origin;          // the remote unit
    };
    // used for regular tasks
    struct {
      dart_task_action_t     fn;              // the action to be invoked
      void                  *data;            // the data to be passed to the action
    };
  };
};

#define DART_STACK_PUSH(_head, _elem) \
  do {                                \
    _elem->next = _head;              \
    _head = _elem;                    \
  } while (0)

#define DART_STACK_POP(_head, _elem) \
  do {                               \
    _elem = _head;                   \
    if (_elem != NULL) {             \
      _head = _elem->next;           \
      _elem->next = NULL;            \
    }                                \
  } while (0)


typedef struct task_list {
  struct task_list      *next;
  dart_task_t           *task;
} task_list_t;

struct task_deque{
  dart_task_t * head;
  dart_task_t * tail;
};

typedef struct dart_taskqueue {
  size_t              num_elem;
  struct task_deque   queues[__DART_PRIO_COUNT];
  dart_mutex_t        mutex;
} dart_taskqueue_t;

typedef struct {
  dart_task_t           * current_task;
#ifdef DART_TASK_THREADLOCAL_Q
  struct dart_taskqueue   queue;
#endif // DART_TASK_THREADLOCAL_Q
  uint64_t                taskcntr;
  pthread_t               pthread;
  context_t               retctx;            // the thread-specific context to return to eventually
  context_list_t        * ctxlist;
  int                     thread_id;
  int                     last_steal_thread;
  dart_yield_target_t     yield_target;
  double                  last_progress_ts;  // the timestamp of the last remote progress call
  dart_task_t           * next_task;         // short-cut on the next task to execute
} dart_thread_t;

struct dart_wait_handle_s {
  size_t              num_handle;
  dart_handle_t       handle[];
};

dart_ret_t
dart__tasking__init() DART_INTERNAL;

int
dart__tasking__thread_num() DART_INTERNAL;

int
dart__tasking__num_threads() DART_INTERNAL;

int32_t
dart__tasking__epoch_bound() DART_INTERNAL;

dart_ret_t
dart__tasking__create_task(
        void           (*fn) (void *),
        void            *data,
        size_t           data_size,
  const dart_task_dep_t *deps,
        size_t           ndeps,
        dart_task_prio_t prio,
  const char            *descr) DART_INTERNAL;


dart_ret_t
dart__tasking__create_task_handle(
        void           (*fn) (void *),
        void            *data,
        size_t           data_size,
  const dart_task_dep_t *deps,
        size_t           ndeps,
        dart_task_prio_t prio,
        dart_taskref_t  *ref) DART_INTERNAL;

dart_ret_t
dart__tasking__taskref_free(dart_taskref_t *tr) DART_INTERNAL;

dart_ret_t
dart__tasking__task_wait(dart_taskref_t *tr) DART_INTERNAL;

dart_ret_t
dart__tasking__task_test(dart_taskref_t *tr, int *flag) DART_INTERNAL;

dart_ret_t
dart__tasking__task_complete() /*DART_INTERNAL*/;

dart_taskref_t
dart__tasking__current_task() DART_INTERNAL;

//void
//dart__base__tasking_print_taskgraph();
//
//dart_ret_t
//dart__base__tasking_sync_taskgraph();

dart_ret_t
dart__tasking__fini() DART_INTERNAL;

dart_ret_t
dart__tasking__yield(int delay) DART_INTERNAL;

/**
 * Functions not exposed to the outside
 */

void
dart__tasking__enqueue_runnable(
  dart_task_t   *task) DART_INTERNAL;

void
dart__tasking__destroy_task(dart_task_t *task) DART_INTERNAL;

dart_thread_t *
dart__tasking__current_thread() DART_INTERNAL;

void
dart__tasking__perform_matching(
  dart_thread_t    * thread,
  dart_taskphase_t   phase) DART_INTERNAL;

DART_INLINE
bool
dart__tasking__is_root_task(dart_task_t *task)
{
  return task->state == DART_TASK_ROOT;
}

#endif /* DART__BASE__INTERNAL__TASKING_H__ */
