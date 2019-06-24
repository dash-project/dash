/*
#include <dash/dart/if/dart_tools.h>
#include <stdio.h>
struct dart_tool_task_create_cb {
    dart_tool_task_create_cb_t cb;
    void *userdata;
};
typedef struct dart_tool_task_create_cb dart_tool_task_create_cb;
struct dart_tool_task_create_cb internal_cb;

struct dart_tool_task_begin_cb {
    dart_tool_task_begin_cb_t cb;
    void *userdata;
};
typedef struct dart_tool_task_begin_cb dart_tool_task_begin_cb;
int dart_tool_register_task_create (dart_tool_task_create_cb_t cb, void *userdata) {
   internal_cb.cb = cb;
   internal_cb.userdata = userdata;
   printf("dart_tool_register_task_create\n");
   return 0;
}
*/
