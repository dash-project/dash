#include <libdash.h>

#include <stdint.h>

#include "dart_tools.h"

unsigned long long myglobalid;
int userdata = 42;

extern "C" void callback_on_task_create(uint64_t task, dart_task_prio_t prio, const char *name, void *userdata) {
}

extern "C" void callback_on_task_begin(uint64_t task, uint64_t thread, void *userdata) {
  
}

extern "C" void callback_on_task_end(uint64_t task, uint64_t thread, void *userdata) {
  
}

extern "C" void callback_on_task_cancel(uint64_t task, uint64_t thread, void *userdata) {
}

extern "C" void callback_on_task_yield_leave(uint64_t task, uint64_t thread,  void *userdata) {

}

extern "C" void callback_on_task_yield_resume(uint64_t task, uint64_t thread,  void *userdata) {

}

extern "C" void callback_on_task_finalize(void *userdata) {
}

extern "C" void callback_on_local_dep(uint64_t task1, uint64_t task2, uint64_t memaddr, int32_t unitid,int edge_type, void *userdata) {
}

extern "C" void callback_on_task_add_to_queue(uint64_t task, uint64_t thread, void *userdata) {
}

extern "C" void callback_on_remote_dep (uint64_t to_task, uint64_t from_task, int to_dep_type, int from_dep_type, uint64_t memaddr, int32_t to_unitid, int32_t from_unitid, int edge_type, void *userdata) {

}


void call_register_functions () {
  dart_tool_register_task_create (&callback_on_task_create, &userdata);
  dart_tool_register_task_begin (&callback_on_task_begin, &userdata);
  dart_tool_register_task_end (&callback_on_task_end, &userdata);
  dart_tool_register_task_cancel (&callback_on_task_cancel, &userdata);
  dart_tool_register_task_yield_leave (&callback_on_task_yield_leave, &userdata);
  dart_tool_register_task_yield_resume (&callback_on_task_yield_resume, &userdata);
  
  dart_tool_register_task_finalize (&callback_on_task_finalize, &userdata);
  
  dart_tool_register_local_dep (&callback_on_local_dep, &userdata);
  dart_tool_register_task_add_to_queue (&callback_on_task_add_to_queue, &userdata);
  dart_tool_register_remote_dep(&callback_on_remote_dep, &userdata);

}

extern "C" int init_ext_tool(int num_threads, int num_units, int32_t myguid) {
  myglobalid = (unsigned long long) myguid;
  call_register_functions();
  return 0;
}
