#include <dash/dart/tasking/dart_tasking_instrumentation.h>
#include <stdio.h>

struct dart_tool_task_create_cb {
    dart_tool_task_create_cb_t cb;
    void *userdata;
};
typedef struct dart_tool_task_create_cb dart_tool_task_create_cb;
struct dart_tool_task_create_cb internal_cb; //data structure to save the function pointer from the callback

/**
struct dart_tool_task_begin_cb {
    dart_tool_task_begin_cb_t cb;
    void *userdata;
};
typedef struct dart_tool_task_begin_cb dart_tool_task_begin_cb;
*/
int dart_tool_register_task_create (dart_tool_task_create_cb_t cb, void *userdata) {
   internal_cb.cb = cb;
   internal_cb.userdata = userdata;
   printf("dart_tool_register_task_create was called\n");
   return 0;
} 


void dart__tasking__instrument_task_create(
  dart_task_t      *task,
  dart_task_prio_t  prio,
  const char       *name)
{
  printf("[INSTR]: task create: name: %s  with prio %d and dart_task_t: %s with state %d and unresolved deps: %d \n", name, prio, task->descr, task->state, task->unresolved_deps);
  internal_cb.cb((uint64_t) task, prio, name, internal_cb.userdata);
}

void dart__tasking__instrument_task_begin(
  dart_task_t   *task,
  dart_thread_t *thread)
{
  printf("[INSTR]: task %s begin with state %d\n", task->descr, task->state);
  
  //dart_dephash_elem_t *ptr = task->deps_owned;
  //printf("ptr: %s at %s", ptr->descr, &ptr);
  //while (ptr != NULL) {
  //  printf("dep descr: %s\n", ptr->descr);
  //  ptr = ptr->next;
  //}
  printf("[INSTR]: dart_task_t: %s, dart_thread_t.thread_id: %d\n", task->descr, thread->thread_id);
  // TODO
}

void dart__tasking__instrument_task_end(
  dart_task_t   *task,
  dart_thread_t *thread)
{
  printf("[INSTR]: task %s end with state %d\n", task->descr, task->state);
  printf("[INSTR]: dart_task_t: %s, dart_thread_t.thread_id: %d\n", task->descr, thread->thread_id);
  // TODO
}

void dart__tasking__instrument_task_cancel(
  dart_task_t   *task,
  dart_thread_t *thread)
{
  printf("DASH: task cancel\n");
  // TODO
}

void dart__tasking__instrument_task_yield_leave(
  dart_task_t   *task,
  dart_thread_t *thread)
{
  printf("DASH: task yield leave\n");
  // TODO
}


void dart__tasking__instrument_task_yield_resume(
  dart_task_t   *task,
  dart_thread_t *thread)
{
  printf("DASH: task yield resume\n");
  // TODO
}
