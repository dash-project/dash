#ifndef DART__BASE__INTERNAL__TASKING_H__
#define DART__BASE__INTERNAL__TASKING_H__

#include <dash/dart/if/dart_active_messages.h>
#include <dash/dart/if/dart_tasking.h>
#include <dash/dart/base/mutex.h>

// forward declaration, defined in dart_tasking_datadeps.c
struct dart_dephash_elem;
struct task_list;

typedef enum {
  DART_TASK_RUNNING,
  DART_TASK_CREATED,
  DART_TASK_FINISHED
} dart_task_state_t;

typedef struct dart_task_data {
  struct dart_task_data     *next;            // next entry in a task list/queue
  struct dart_task_data     *prev;            // previous entry in a task list/queue
  dart_task_action_t         fn;              // the action to be invoked
  void                      *data;            // the data to be passed to passed to the action
  size_t                     data_size;       // the size of the data; data will be freed if data_size > 0
  int                        unresolved_deps; // the number of unresolved task dependencies
  struct task_list          *successor;       // the list of tasks that have a dependency to this task
  struct dart_task_data     *parent;          // the task that created this task
  struct dart_dephash_elem  *remote_successor;
  int                        num_children;
  dart_mutex_t               mutex;
  dart_task_state_t          state;
} dart_task_t;


typedef struct task_list {
  struct task_list      *next;
  dart_task_t           *task;
} task_list_t;

typedef struct dart_taskqueue {
  dart_task_t          * head;
  dart_task_t          * tail;
  pthread_mutex_t        mutex;
} dart_taskqueue_t;

typedef struct {
  dart_task_t           * current_task;
  struct dart_taskqueue   queue;
  pthread_t               pthread;
  int                     thread_id;
} dart_thread_t;

dart_ret_t
dart__base__tasking__init();

int
dart__base__tasking__thread_num();

int
dart__base__tasking__num_threads();

dart_ret_t
dart__base__tasking__create_task(void (*fn) (void *), void *data, size_t data_size, dart_task_dep_t *deps, size_t ndeps);

dart_ret_t
dart__base__tasking__task_complete();

//void
//dart__base__tasking_print_taskgraph();
//
//dart_ret_t
//dart__base__tasking_sync_taskgraph();

dart_ret_t
dart__base__tasking__fini();

dart_thread_t *
dart__base__tasking_current_thread();


#endif /* DART__BASE__INTERNAL__TASKING_H__ */
