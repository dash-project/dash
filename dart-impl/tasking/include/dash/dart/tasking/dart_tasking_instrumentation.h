#ifndef DART__TASKING__INTERNAL_AYURDAME_H__
#define DART__TASKING__INTERNAL_AYURDAME_H__


#include <dash/dart/if/dart_active_messages.h>


void dart__tasking__ayudame__pre_init(uint64_t) {}

void dart__tasking__ayudame_init(uint64_t) {}

void dart__tasking__ayudame_create_task(uint64_t,uint64_t, uint64_t, uint64_t) {}

void dart__tasking__ayudame_add_dependency(uint64_t,uint64_t, uint64_t, uint64_t) {}

void dart__tasking__ayudame_register_function(uint64_t, char *) {}

void dart__tasking__ayudame_begin_task(uint64_t) {}

void dart__tasking__ayudame_end_task(uint64_t) {}

void dart__tasking__ayudame_destroy_task(uint64_t) {}

void dart__tasking__ayudame_finalize() {}

#endif

