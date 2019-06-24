//#include <dash/dart/base/macro.h>
//#include <dash/dart/tasking/dart_tasking_priv.h>
#include <dash/dart/if/dart_tasking.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef void (*dart_tool_task_create_cb_t) (uint64_t task, dart_task_prio_t prio, const char *name, void *userdata);
typedef void (*dart_tool_task_begin_cb_t) (uint64_t task, uint64_t thread, void *userdata);

//register function for every event
int dart_tool_register_task_create (dart_tool_task_create_cb_t cb, void *userdata);
int dart_tool_register_task_begin (dart_tool_task_begin_cb_t cb, void *userdata);

#ifdef __cplusplus
}
#endif
