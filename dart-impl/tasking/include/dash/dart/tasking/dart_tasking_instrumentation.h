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
  const char       *name) DART_INTERNAL;

/**
 * Instrumentation point of a task begin event. Called right before the
 * execution of the user provided task action.
 */
void dart__tasking__instrument_task_begin(
  dart_task_t   *task,
  dart_thread_t *thread) DART_INTERNAL;

/**
 * Instrumentation point of a task end event. Called right after the execution
 * of the user provided task action has completed.
 */
void dart__tasking__instrument_task_end(
  dart_task_t   *task,
  dart_thread_t *thread) DART_INTERNAL;

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
  
/** Instrumentation point of an all task ended event.
 * 
*/
void dart__tasking__instrument_task_all_end(
  ) DART_INTERNAL;

#endif /* DART_TASKING_INSTRUMENTATION_H_ */
