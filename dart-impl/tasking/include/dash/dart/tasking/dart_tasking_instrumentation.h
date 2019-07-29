#ifndef DART_TASKING_INSTRUMENTATION_H_
#define DART_TASKING_INSTRUMENTATION_H_

#include <dash/dart/base/macro.h>
#include <dash/dart/tasking/dart_tasking_priv.h>
//new
#include <dash/dart/if/dart_tools.h>
/**
 * Instrumentation point of a task creation event. Called as soon as a task is
 * inserted into the scheduler, before dependencies are handled.
 */
void dart__tasking__instrument_task_create(
  dart_task_t      *task,
  dart_task_prio_t  prio,
  const char       *name,
  int32_t task_unitid) DART_INTERNAL;

/**
 * Instrumentation point of a task begin event. Called right before the
 * execution of the user provided task action.
 */
void dart__tasking__instrument_task_begin(
  dart_task_t   *task,
  dart_thread_t *thread,
  int32_t task_unitid) DART_INTERNAL;

/**
 * Instrumentation point of a task end event. Called right after the execution
 * of the user provided task action has completed.
 */
void dart__tasking__instrument_task_end(
  dart_task_t   *task,
  dart_thread_t *thread,
  int32_t task_unitid) DART_INTERNAL;

/**
 * Instrumentation point of a task cancel event. Called before leaving the
 * running task after it has been cancelled.
 */
void dart__tasking__instrument_task_cancel(
  dart_task_t   *task,
  dart_thread_t *thread) DART_INTERNAL;

/**
 * Instrumentation point of a task yielding the thread.
 */
void dart__tasking__instrument_task_yield_leave(
  dart_task_t   *task,
  dart_thread_t *thread) DART_INTERNAL;

/**
 * Instrumentation point of a task returning to execution after a previous
 * yield.
 */
void dart__tasking__instrument_task_yield_resume(
  dart_task_t   *task,
  dart_thread_t *thread) DART_INTERNAL;
  
/** 
 * Instrumentation point of an all task ended (finalized) event.
*/
void dart__tasking__instrument_task_finalize(
  ) DART_INTERNAL;
  
/**
 * Instrumentation point of two tasks in the same local task graph share a
 * read-after-write dependency.
*/
void dart__tasking__instrument_local_dep_raw(
    dart_task_t *task1,
    dart_task_t *task2,
    uint64_t memaddr_raw,
    uint64_t orig_memaddr_raw,
    int32_t task1_unitid,
    int32_t task2_unitid) DART_INTERNAL;
/**
 * Instrumentation point of two tasks in the same local task graph share a
 * write-after-write dependency.
*/
void dart__tasking__instrument_local_dep_waw(
    dart_task_t *task1,
    dart_task_t *task2,
    uint64_t memaddr_waw,
    uint64_t orig_memaddr_waw,
    int32_t task1_unitid,
    int32_t task2_unitid) DART_INTERNAL;
/**
 * Instrumentation point of two tasks in the same local task graph share a
 * write-after-read (anti-)dependency.
*/    
void dart__tasking__instrument_local_dep_war(
    dart_task_t *task1,
    dart_task_t *task2,
    uint64_t memaddr_war,
    uint64_t orig_memaddr_war,
    int32_t task1_unitid,
    int32_t task2_unitid) DART_INTERNAL;
/**
 * Instrumentation point of a task beeing added into the task queue.
 * Called right before inserting the task into the queue.
*/
void dart__tasking__instrument_task_add_to_queue(
    dart_task_t *task,
    dart_thread_t *thread,
    int32_t task_unitid) DART_INTERNAL;
    
/**
 * Instrumentation point of a dummy dependency added into the task graph
 * replacing an input dependency, which cannot be matched yet.
 */
void dart__tasking__instrument_dummy_dep_create(
    dart_task_t *task,
    uint64_t dummy_dep,
    uint64_t in_dep,
    dart_task_dep_t out_dep,
    int32_t task_unitid) DART_INTERNAL;
/**
 * Instrumentation point of a dummy dependency captured, meaning
 * a matching dependency was now found, the dummy dependency is
 * not needed anymore.
*/
void dart__tasking__instrument_dummy_dep_capture (
    dart_task_t *task,
    uint64_t dummy_dep,
    uint64_t remote_dep,
    int32_t task_unitid) DART_INTERNAL;

/**
 * Instrumentation point of a remote input dependency matched with a
 * fitting output dependency from another task.
*/
    
void dart__tasking__instrument_remote_in_dep(
    uint64_t local_task,
    uint64_t remote_task,
    int local_dep_type,
    int remote_dep_type,
    int32_t local_unitid,
    int32_t remote_unitid) DART_INTERNAL;

#endif /* DART_TASKING_INSTRUMENTATION_H_ */
