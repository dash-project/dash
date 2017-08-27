#ifndef DART__BASE__INTERNAL__TASKING_H__
#define DART__BASE__INTERNAL__TASKING_H__

#include <stdbool.h>
#include <dash/dart/if/dart_active_messages.h>
#include <dash/dart/if/dart_tasking.h>
#include <dash/dart/base/mutex.h>
#include <dash/dart/tasking/dart_tasking_context.h>

// forward declaration, defined in dart_tasking_datadeps.c
struct dart_dephash_elem;
struct task_list;

typedef enum {
  DART_TASK_ROOT     = -1, // special state assigned to the root task
  DART_TASK_FINISHED =  0, // comparison with 0
  DART_TASK_CREATED,
  DART_TASK_RUNNING,
  DART_TASK_SUSPENDED,
  DART_TASK_NASCENT,
  DART_TASK_DESTROYED,
  DART_TASK_CANCELLED
} dart_task_state_t;

struct dart_task_data {
  struct dart_task_data     *next;            // next entry in a task list/queue
  struct dart_task_data     *prev;            // previous entry in a task list/queue
  dart_task_action_t         fn;              // the action to be invoked
  void                      *data;            // the data to be passed to passed to the action
  size_t                     data_size;       // the size of the data; data will be freed if data_size > 0
  int32_t                    unresolved_deps; // the number of unresolved task dependencies
  int32_t                    unresolved_remote_deps; // the number of unresolved remote task dependencies
  struct task_list          *successor;       // the list of tasks that depend on this task
  struct dart_task_data     *parent;          // the task that created this task
  struct dart_dephash_elem  *remote_successor;
  struct dart_dephash_elem **local_deps;      // hashmap containing dependencies of child tasks
  int                        num_children;
  dart_mutex_t               mutex;
  dart_task_state_t          state;
  int32_t                    epoch;
  context_t                 *taskctx;         // context to start/resume task
  int                        delay;           // delay in case this task yields
  dart_task_prio_t           prio;
  bool                       has_ref;
};

#define DART_STACK_PUSH(_head, _elem) \
  do {                                \
    _elem->next = _head;              \
    _head = _elem;                    \
  } while (0)

#define DART_STACK_POP(_head, _elem) \
  do {                               \
    _elem = _head;                   \
    _head = _elem->next;             \
    _elem->next = NULL;              \
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
  struct task_deque   lowprio;
  struct task_deque   highprio;
  dart_mutex_t        mutex;
} dart_taskqueue_t;

typedef struct {
  dart_task_t           * current_task;
  struct dart_taskqueue   queue;
  struct dart_taskqueue   defered_queue;
  uint64_t                taskcntr;
  pthread_t               pthread;
  context_t               retctx;            // the thread-specific context to return to eventually
  context_list_t        * ctxlist;
  int                     thread_id;
  int                     last_steal_thread;
} dart_thread_t;

dart_ret_t
dart__tasking__init();

int
dart__tasking__thread_num();

int
dart__tasking__num_threads();

int32_t
dart__tasking__epoch_bound();

dart_ret_t
dart__tasking__create_task(
        void           (*fn) (void *),
        void            *data,
        size_t           data_size,
  const dart_task_dep_t *deps,
        size_t           ndeps,
        dart_task_prio_t prio);


dart_ret_t
dart__tasking__create_task_handle(
        void           (*fn) (void *),
        void            *data,
        size_t           data_size,
  const dart_task_dep_t *deps,
        size_t           ndeps,
        dart_task_prio_t prio,
        dart_taskref_t  *ref);

dart_ret_t
dart__tasking__taskref_free(dart_taskref_t *tr);

dart_ret_t
dart__tasking__task_wait(dart_taskref_t *tr);

dart_ret_t
dart__tasking__task_complete();

dart_ret_t
dart__tasking__phase();

dart_taskref_t
dart__tasking__current_task();

void
dart__tasking__enqueue_runnable(dart_task_t *task);

void
dart__tasking__cancel_task(dart_task_t *task);
//void
//dart__base__tasking_print_taskgraph();
//
//dart_ret_t
//dart__base__tasking_sync_taskgraph();

dart_ret_t
dart__tasking__fini();

dart_thread_t *
dart__tasking_current_thread();

dart_ret_t
dart__tasking__yield(int delay);

#endif /* DART__BASE__INTERNAL__TASKING_H__ */
