#include <dash/dart/tasking/dart_tasking_instrumentation.h>
#include <stdio.h>

struct dart_tool_task_create_cb {
    dart_tool_task_create_cb_t cb;
    void *userdata;
};
struct dart_tool_task_begin_cb {
    dart_tool_task_begin_cb_t cb;
    void *userdata;
};
struct dart_tool_task_end_cb {
    dart_tool_task_end_cb_t cb;
    void *userdata;
};
struct dart_tool_task_cancel_cb {
    dart_tool_task_cancel_cb_t cb;
    void *userdata;
};
struct dart_tool_task_yield_leave_cb {
    dart_tool_task_yield_leave_cb_t cb;
    void *userdata;
};
struct dart_tool_task_yield_resume_cb {
    dart_tool_task_yield_resume_cb_t cb;
    void *userdata;
};

struct dart_tool_task_all_end_cb {
    dart_tool_task_all_end_cb_t cb;
    void *userdata;
};
typedef struct dart_tool_task_create_cb dart_tool_task_create_cb;
typedef struct dart_tool_task_begin_cb dart_tool_task_begin_cb;
typedef struct dart_tool_task_cancel_cb dart_tool_task_cancel_cb;
typedef struct dart_tool_task_yield_leave_cb dart_task_yield_leave_cb;
typedef struct dart_tool_task_yield_resume_cb dart_task_yield_resume_cb;
typedef struct dart_tool_task_all_end_cb dart_tool_task_all_end_cb;
/* data structures to save the function pointer and user data from the callback */
struct dart_tool_task_create_cb dart_tool_task_create_cb_data;
struct dart_tool_task_begin_cb dart_tool_task_begin_cb_data;
struct dart_tool_task_end_cb dart_tool_task_end_cb_data;
struct dart_tool_task_all_end_cb dart_tool_task_all_end_cb_data;



/* implementation of the register function of the callback */
int dart_tool_register_task_create (dart_tool_task_create_cb_t cb, void *userdata_task_create) {
   dart_tool_task_create_cb_data.cb = cb;
   dart_tool_task_create_cb_data.userdata = userdata_task_create;
   printf("dart_tool_register_task_create was called\nPointer: %p and userdata %d\n", cb, *(int *) userdata_task_create);
   return 0;
} 

int dart_tool_register_task_begin(dart_tool_task_begin_cb_t cb, void *userdata_task_begin) {
    dart_tool_task_begin_cb_data.cb = cb;
    dart_tool_task_begin_cb_data.userdata = userdata_task_begin;
    printf("dart_tool_register_task_begin was called\nPointer: %p and userdata %d\n", cb, *(int *) userdata_task_begin);
    return 0;
}

int dart_tool_register_task_end (dart_tool_task_end_cb_t cb, void *userdata_task_end) {
    dart_tool_task_end_cb_data.cb = cb;
    dart_tool_task_end_cb_data.userdata = userdata_task_end;
    printf("dart_tool_register_task_end was called\nPointer: %p and userdata %d\n", cb, *(int *) userdata_task_end);
    return 0;
}

int dart_tool_register_task_all_end(dart_tool_task_all_end_cb_t cb, void *userdata_task_all_end) {
    dart_tool_task_all_end_cb_data.cb = cb;
    dart_tool_task_all_end_cb_data.userdata = userdata_task_all_end;
    printf("dart_tool_register_task_all_end was called\nPointer: %p and userdata %d\n", cb, *(int*) userdata_task_all_end);
    return 0;
}



void dart__tasking__instrument_task_create(
  dart_task_t      *task,
  dart_task_prio_t  prio,
  const char       *name)
{
    if (dart_tool_task_create_cb_data.cb) {
          dart_tool_task_create_cb_data.cb((uint64_t) task, prio, name, dart_tool_task_create_cb_data.userdata);

    }
}

void dart__tasking__instrument_task_begin(
  dart_task_t   *task,
  dart_thread_t *thread)
{
    if (dart_tool_task_begin_cb_data.cb) {
        dart_tool_task_begin_cb_data.cb((uint64_t) task, (uint64_t) thread, dart_tool_task_begin_cb_data.userdata);
    }
  // TODO
}

void dart__tasking__instrument_task_end(
  dart_task_t   *task,
  dart_thread_t *thread)
{
    if (dart_tool_task_end_cb_data.cb) {
        dart_tool_task_end_cb_data.cb((uint64_t) task, (uint64_t) thread, dart_tool_task_end_cb_data.userdata);
    }
  // TODO
}

void dart__tasking__instrument_task_cancel(
  dart_task_t   *task,
  dart_thread_t *thread)
{
  //printf("DASH: task cancel\n");
  // TODO
}

void dart__tasking__instrument_task_yield_leave(
  dart_task_t   *task,
  dart_thread_t *thread)
{
  //printf("DASH: task yield leave\n");
  // TODO
}


void dart__tasking__instrument_task_yield_resume(
  dart_task_t   *task,
  dart_thread_t *thread)
{
  //printf("DASH: task yield resume\n");
  // TODO
}

void dart__tasking__instrument_task_all_end ()
{
    if (dart_tool_task_all_end_cb_data.cb) {
        dart_tool_task_all_end_cb_data.cb(dart_tool_task_end_cb_data.userdata);
    }
}
