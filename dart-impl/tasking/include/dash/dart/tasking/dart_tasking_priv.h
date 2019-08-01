#ifndef DART__INTERNAL__TASKING_H__
#define DART__INTERNAL__TASKING_H__

#include <stdbool.h>
#include <pthread.h>
#include <dash/dart/if/dart_active_messages.h>
#include <dash/dart/if/dart_communication.h>
#include <dash/dart/if/dart_tasking.h>
#include <dash/dart/base/mutex.h>
#include <dash/dart/base/stack.h>
#include <dash/dart/base/macro.h>
#include <dash/dart/tasking/dart_tasking_context.h>
#include <dash/dart/tasking/dart_tasking_phase.h>
#include <dash/dart/tasking/dart_tasking_envstr.h>
#include <dash/dart/tasking/dart_tasking_tasklock.h>

// forward declaration, defined in dart_tasking_datadeps.c
typedef struct dart_dephash_elem dart_dephash_elem_t;
typedef struct dart_dephash_head dart_dephash_head_t;
struct task_list;

#ifdef USE_UCONTEXT
#define HAVE_RESCHEDULING_YIELD 1
#endif // USE_UCONTEXT

// disable re-use of objects, can help in debugging
//#define DART_TASKING_DONOT_REUSE

typedef enum {
  DART_TASK_ROOT     = -1, // special state assigned to the root task
  DART_TASK_FINISHED =  0, // comparison with 0
  DART_TASK_NASCENT,
  // active task states begin here
  // NOTE: check IS_ACTIVE_TASK macro if you make changes here!!
  DART_TASK_CREATED,
  DART_TASK_DEFERRED,      // the task is deferred because its phase is not release yet
  DART_TASK_QUEUED,
  DART_TASK_DUMMY,         // the task is a dummy for a remote task
  DART_TASK_RUNNING,
  DART_TASK_SUSPENDED,     // the task is suspended but runnable
  DART_TASK_BLOCKED,       // the task is blocked waiting for a handle
  DART_TASK_DETACHED,      // the task has been detached, will not run again
  // active task states end here
  DART_TASK_DESTROYED,
  DART_TASK_CANCELLED
} dart_task_state_t;

#define IS_ACTIVE_TASK(task) \
  ((task)->state >= DART_TASK_NASCENT   && \
   (task)->state <= DART_TASK_DETACHED)


typedef
struct dart_wait_handle_s dart_wait_handle_t;

enum dart_taskflags_t {
  DART_TASK_HAS_REF        = 1 << 0,
  DART_TASK_DATA_ALLOCATED = 1 << 1,
  DART_TASK_IS_INLINED     = 1 << 2,
  DART_TASK_IS_COMMTASK    = 1 << 3
};

#define DART_TASK_SET_FLAG(_task, _flag)    (_task)->flags |=  (_flag)
#define DART_TASK_UNSET_FLAG(_task, _flag)  (_task)->flags &= ~(_flag)
#define DART_TASK_HAS_FLAG(_task, _flag)    ((_task)->flags &   (_flag))

typedef struct task_list {
  struct task_list      *next;
  dart_task_t           *task;
} task_list_t;

struct task_deque{
  dart_task_t * head;
  dart_task_t * tail;
};


struct dart_task_data {
  union {
    // atomic list used for free elements
    DART_STACK_MEMBER_DEF;
    // double linked list used in lists/queues
    struct {
      struct dart_task_data     *next;        // next entry in a task list/queue
      struct dart_task_data     *prev;        // previous entry in a task list/queue
    };
  };
  int                        prio;
  uint16_t                   flags;
  int8_t                     state;           // one of dart_task_state_t, single byte sufficient
  dart_tasklock_t            lock;
  struct task_list          *successor;       // the list of tasks that depend on this task
  dart_dephash_elem_t       *remote_successor;// the list of dependencies from remote tasks directly dependending on this task
  struct dart_task_data     *parent;          // the task that created this task
  // TODO: pack using pahole, move all execution-specific fields into context
  context_t                 *taskctx;         // context to start/resume task
  void                      *numaptr;         // ptr used to determine the NUMA node
  union {
    // only relevant before execution, both will be zero when the task starts execution
    struct {
      int32_t                    unresolved_deps; // the number of unresolved task dependencies
      int32_t                    unresolved_remote_deps; // the number of unresolved remote task dependencies
    };
    // only relevant during execution
    dart_dephash_head_t      *local_deps;      // hashmap containing dependencies of child tasks
  };
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
  dart_dephash_elem_t       *deps_owned;      // list of dependencies owned by this task
  dart_wait_handle_t        *wait_handle;
  const char                *descr;           // the description of the task
  dart_taskphase_t           phase;
  int                        num_children;
  int16_t                    owner;           // the thread owning the task object memory
#ifdef DART_DEBUG
  task_list_t              * children;  // list of child tasks
#endif //DART_DEBUG
};

#define DART_STACK_PUSH(_head, _elem) \
  DART_STACK_PUSH_MEMB(_head, _elem, next)

#define DART_STACK_POP(_head, _elem) \
  DART_STACK_POP_MEMB(_head, _elem, next)

#define DART_STACK_PUSH_MEMB(_head, _elem, _memb) \
  do {                                \
    _elem->_memb = _head;           \
    _head = _elem;                    \
  } while (0)

#define DART_STACK_POP_MEMB(_head, _elem, _memb) \
  do {                               \
    _elem = _head;                   \
    if (_elem != NULL) {             \
      _head = _elem->_memb;        \
      _elem->_memb = NULL;         \
    }                                \
  } while (0)

/**
 * The maximum number of utility threads allowed. Adjust if adding potential new
 * utlity thread!
 */
#define DART_TASKING_MAX_UTILITY_THREADS 1

/*
 * Special priority signalling to immediately execute the task when ready.
 * The task's action will be called directly in the context of the current task.
 * The task should not be cancelled. Currently this is used internally for
 * copyin tasks.
 * TODO: expose this to the user?
 */
#define DART_PRIO_INLINE (__DART_PRIO_COUNT)

typedef struct dart_taskqueue {
  size_t              num_elem;
  struct task_deque   queues[__DART_PRIO_COUNT];
  dart_mutex_t        mutex;
} dart_taskqueue_t;

/* Number of tasks queued inside a thread */
#define THREAD_QUEUE_SIZE 16

typedef struct {
  dart_task_t           * current_task;
  dart_task_t           * queue[THREAD_QUEUE_SIZE]; // array of tasks short-cut
  dart_task_t           * next_task;                // pointer to the next task executed in a yield
  uint64_t                taskcntr;
  pthread_t               pthread;
  context_t               retctx;            // the thread-specific context to return to eventually
  dart_stack_t            ctxlist;           // a free-list of contexts, written by all threads
  context_list_t        * ctx_to_enter;      // the context to enter next
  int                     thread_id;
  int                     delay;             // delay in case this task yields
  int16_t                 core_id;
  int8_t                  numa_id;
  bool                    is_utility_thread; // whether the thread is a worker or utility thread
  double                  last_progress_ts;  // the timestamp of the last remote progress call
  int                     last_steal_thread_id;  // the timestamp of the last remote progress call
};

dart_ret_t
dart__tasking__init() DART_INTERNAL;

int
dart__tasking__thread_num() DART_INTERNAL;

int
dart__tasking__num_threads() DART_INTERNAL;

int
dart__tasking__num_tasks()   DART_INTERNAL;

int32_t
dart__tasking__epoch_bound() DART_INTERNAL;

dart_ret_t
dart__tasking__create_task(
        void             (*fn) (void *),
        void              *data,
        size_t             data_size,
  const dart_task_dep_t   *deps,
        size_t             ndeps,
        dart_task_prio_t   prio,
  const char              *descr,
        dart_taskref_t    *taskref) DART_INTERNAL;

dart_ret_t
dart__tasking__taskref_free(dart_taskref_t *tr) DART_INTERNAL;

dart_ret_t
dart__tasking__task_wait(dart_taskref_t *tr) DART_INTERNAL;

dart_ret_t
dart__tasking__task_test(dart_taskref_t *tr, int *flag) DART_INTERNAL;

dart_ret_t
dart__tasking__task_complete(bool local_only) DART_INTERNAL;

dart_taskref_t
dart__tasking__current_task() DART_INTERNAL;

dart_task_t*
dart__tasking__root_task() DART_INTERNAL;

void
dart__tasking__mark_detached(dart_taskref_t task) DART_INTERNAL;

void
dart__tasking__release_detached(dart_taskref_t task) DART_INTERNAL;

void
dart__tasking__handle_task(dart_task_t *task) DART_INTERNAL;

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

dart_task_t *
dart__tasking__allocate_dummytask() DART_INTERNAL;

void
dart__tasking__perform_matching(
  dart_taskphase_t   phase) DART_INTERNAL;

DART_INLINE
bool
dart__tasking__is_root_task(dart_task_t *task)
{
  return task->state == DART_TASK_ROOT;
}


dart_taskqueue_t *
dart__tasking__get_taskqueue() DART_INTERNAL;

void dart__tasking__utility_thread(
  void (*fn) (void *),
  void  *data
) DART_INTERNAL;


#define CLOCK_TIME_USEC(ts) \
  (((uint64_t)(ts).tv_sec)*1000*1000 + (ts).tv_nsec/1000)

static inline
uint64_t current_time_us() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return CLOCK_TIME_USEC(ts);
}

#endif /* DART__INTERNAL__TASKING_H__ */
