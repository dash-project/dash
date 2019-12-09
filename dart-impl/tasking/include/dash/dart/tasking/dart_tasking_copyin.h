#ifndef DART_TASKING_DART_TASKING_COPYIN_H_
#define DART_TASKING_DART_TASKING_COPYIN_H_

#include <dash/dart/base/macro.h>
#include <dash/dart/tasking/dart_tasking_priv.h>
#include <dash/dart/tasking/dart_tasking_datadeps.h>

/**
 * Basic functionality for copying-in a consecutive memory region from either
 * the local or a remote unit.
 *
 * This can be implemented in two ways:
 * 1) Create a task with a remote dependency and have all tasks with the same
 *    copy-in dependency depend on it.
 * 2) Send a request for a send to the remote unit and create a local task
 *    that receives the data and then releases all tasks with the same copin dep.
 *    This re-couples synchronization and communication and is expected to work
 *    better on systems that emulate RMA through two-sided communication as it
 *    reduces the amount of messages to be exchanged.
 *
 * TODO: Do we have to cache the request for a send at the sender side to avoid
 *       race conditions if the active message is invoked before the producing
 *       task has been created.
 * TODO: Add implementation for RMA-based copy-in.
 */

void
dart_tasking_copyin_init();

void
dart_tasking_copyin_fini();

/**
 * Call-back called by a remote unit to create a task that
 * sends data (if required).
 */
void
dart_tasking_copyin_sendrequest(
  dart_gptr_t            src_gptr,
  int32_t                num_bytes,
  dart_taskphase_t       phase,
  int                    tag,
  dart_global_unit_t     unit) DART_INTERNAL;

/**
 * Create delayed tasks created by \c dart_tasking_copyin_create_task
 * from remote unit.
 */
void
dart_tasking_copyin_create_delayed_tasks() DART_INTERNAL;

/**
 * Create a task that performs the copy-in.
 * Potentially creates a task on the remote side as well.
 */
dart_ret_t
dart_tasking_copyin_create_task(
  const dart_task_dep_t * dep,
        taskref           local_task) DART_INTERNAL;

void *
dart__tasking__copyin_info(dart_task_t *task, int depnum) DART_INTERNAL;

#endif // DART_TASKING_DART_TASKING_COPYIN_H_
