#ifndef DART__TOOLS_H_
#define DART__TOOLS_H_

#include <dash/dart/if/dart_tasking.h>

#define DART__TOOLS_TOOL_ENV_VAR_PATH "DART_TOOL_PATH"
#define DART__TOOLS_TOOL_INIT_FUNCTION_NAME "init_ext_tool"
#ifdef __cplusplus
extern "C" {
#endif

typedef void (*dart_tool_task_create_cb_t) (uint64_t task, dart_task_prio_t prio, const char *name, void *userdata);
typedef void (*dart_tool_task_begin_cb_t) (uint64_t task, uint64_t thread, void *userdata);
typedef void (*dart_tool_task_end_cb_t) (uint64_t task, uint64_t thread, void *userdata);
typedef void (*dart_tool_task_cancel_cb_t) (uint64_t task, uint64_t thread, void *userdata);
typedef void (*dart_tool_task_yield_leave_cb_t) (uint64_t task, uint64_t thread,  void *userdata);
typedef void (*dart_tool_task_yield_resume_cb_t) (uint64_t task, uint64_t thread, void *userdata);
typedef void (*dart_tool_task_finalize_cb_t) (void *userdata);

typedef void (*dart_tool_task_add_to_queue_cb_t) (uint64_t task, uint64_t thread, void *userdata);

typedef void (*dart_tool_local_dep_cb_t) (uint64_t task1, uint64_t task2, uint64_t memaddr, uint64_t orig_memaddr, int32_t task1_unitid, int32_t task2_unitid, int edge_type,  void *userdata);

typedef void (*dart_tool_dummy_dep_create_cb_t) (uint64_t task, uint64_t dummy_dep, uint64_t in_dep, int phase, int32_t task_unitid, void *userdata);
typedef void (*dart_tool_dummy_dep_capture_cb_t) (uint64_t task, uint64_t dummy_dep, uint64_t remote_dep, int32_t task_unitid, void *userdata);

typedef void (*dart_tool_remote_dep_cb_t) (uint64_t local_task, uint64_t remote_task, int local_dep_type, int remote_dep_type , uint64_t memaddr, uint64_t orig_memaddr, int32_t local_unitid, int32_t remote_unitid, int edge_type, void *userdata);



//register function for every event
int dart_tool_register_task_create (dart_tool_task_create_cb_t cb, void *userdata);
int dart_tool_register_task_begin (dart_tool_task_begin_cb_t cb, void *userdata);
int dart_tool_register_task_end(dart_tool_task_end_cb_t cb, void* userdata);
int dart_tool_register_task_cancel (dart_tool_task_cancel_cb_t cb, void *userdata);
int dart_tool_register_task_yield_leave (dart_tool_task_yield_leave_cb_t cb, void *userdata);
int dart_tool_register_task_yield_resume (dart_tool_task_yield_resume_cb_t cb, void *userdata);
int dart_tool_register_task_finalize (dart_tool_task_finalize_cb_t cb, void *userdata);

int dart_tool_register_task_add_to_queue (dart_tool_task_add_to_queue_cb_t cb, void *userdata);

int dart_tool_register_local_dep (dart_tool_local_dep_cb_t cb, void *userdata);

int dart_tool_register_dummy_dep_create (dart_tool_dummy_dep_create_cb_t cb, void *userdata);
int dart_tool_register_dummy_dep_capture (dart_tool_dummy_dep_capture_cb_t cb, void *userdata);

int dart_tool_register_remote_dep (dart_tool_remote_dep_cb_t cb, void *userdata);


#ifdef __cplusplus
}
#endif

#endif /* DART__TOOLS_H_*/
