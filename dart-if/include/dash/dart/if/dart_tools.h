#ifndef DART__TOOLS_H_
#define DART__TOOLS_H_

#include <dash/dart/if/dart_tasking.h>

#define DART__TOOLS_TOOL_ENV_VAR_PATH "TOOL_PATH"
#define DART__TOOLS_TOOL_INIT_FUNCTION_NAME "init_ext_tool"
#ifdef __cplusplus
extern "C" {
#endif

typedef void (*dart_tool_task_create_cb_t) (uint64_t task, dart_task_prio_t prio, const char *name, void *userdata);
typedef void (*dart_tool_task_begin_cb_t) (uint64_t task, uint64_t thread, void *userdata);
typedef void (*dart_tool_task_end_cb_t) (uint64_t task, uint64_t thread, void *userdata);
typedef void (*dart_tool_task_cancel_cb_t) (uint64_t task, uint64_t thread, void *userdata);
typedef void (*dart_tool_task_yield_leave_cb_t) (uint64_t task, uint64_t thread, void *userdata);
typedef void (*dart_tool_task_yield_resume_cb_t) (uint64_t task, uint64_t thread, void *userdata);
typedef void (*dart_tool_task_finalize_cb_t) (void *userdata);


typedef void (*dart_tool_local_dep_raw_cb_t) (uint64_t task1, uint64_t task2, uint64_t memaddr, uint64_t orig_memaddr, void *userdata);
typedef void (*dart_tool_local_dep_waw_cb_t) (uint64_t task1, uint64_t task2, uint64_t memaddr, uint64_t orig_memaddr, void *userdata);
typedef void (*dart_tool_local_dep_war_cb_t) (uint64_t task1, uint64_t task2, uint64_t memaddr, uint64_t orig_memaddr, void *userdata);




//register function for every event
int dart_tool_register_task_create (dart_tool_task_create_cb_t cb, void *userdata);
int dart_tool_register_task_begin (dart_tool_task_begin_cb_t cb, void *userdata);
int dart_tool_register_task_end(dart_tool_task_end_cb_t cb, void* userdata);
int dart_tool_register_task_cancel (dart_tool_task_cancel_cb_t cb, void *userdata);
int dart_tool_register_yield_leave (dart_tool_task_yield_leave_cb_t cb, void *userdata);
int dart_tool_register_yield_resume (dart_tool_task_yield_resume_cb_t cb, void *userdata);
int dart_tool_register_task_finalize (dart_tool_task_finalize_cb_t cb, void *userdata);

int dart_tool_register_local_dep_raw (dart_tool_local_dep_raw_cb_t cb, void *userdata);
int dart_tool_register_local_dep_waw (dart_tool_local_dep_waw_cb_t cb, void *userdata);
int dart_tool_register_local_dep_war (dart_tool_local_dep_war_cb_t cb, void *userdata);


#ifdef __cplusplus
}
#endif

#endif /* DART__TOOLS_H_*/
