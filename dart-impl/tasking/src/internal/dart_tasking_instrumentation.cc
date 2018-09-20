#include <dash/dart/tasking/dart_tasking_instrumentation.h>
#include <ayu_events.h>

void dart__tasking__ayudame__pre_init(uint64_t rt)
{
    ayu_event_preinit(rt);
}

void dart__tasking__ayudame_init(uint64_t nthreads)
{
    ayu_event_init(nthreads);
}

void dart__tasking__ayudame_create_task(uint64_t task_id, uint64_t func_id, uint64_t priority, uint64_t scope_id)
{
    ayu_event_addtask(task_id, func_id, priority, scope_id);
}

void dart__tasking__ayudame_add_dependency(uint64_t to_id, uint64_t from_id, uint64_t memory_addr, uint64_t orig_memeroyaddr)
{
    ayu_event_adddependency(to_id, from_id, memory_addr, orig_memeroyaddr);
}

void dart__tasking__ayudame_register_function(uint64_t funcid, char *name)
{
    ayu_event_registerfunction(funcid, name);
}

void dart__tasking__ayudame_begin_task(uint64_t taskid)
{
    ayu_event_runtask(taskid);
}

void dart__tasking__ayudame_end_task(uint64_t taskid)
{
    ayu_event_postruntask(taskid);
}

void dart__tasking__ayudame_destroy_task(uint64_t taskid)
{
    ayu_event_removetask(taskid);
}

void dart__tasking__ayudame_finalize()
{
    ayu_event_finish();
}