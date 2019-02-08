/*
 * dart_tasking_datadeps.h
 *
 *  Created on: Nov 8, 2016
 *      Author: joseph
 */

#ifndef DART_TASKING_DATADEPS_H_
#define DART_TASKING_DATADEPS_H_

#include <dash/dart/base/assert.h>

#include <dash/dart/tasking/dart_tasking_priv.h>
#include <dash/dart/if/dart_tasking.h>

// segment ID for localized gptr
#define DART_TASKING_DATADEPS_LOCAL_SEGID ((uint16_t)-1)

typedef void* remote_task_t;

typedef union taskref {
  dart_task_t         *local;
  remote_task_t        remote;
} taskref;

#define TASKREF(_ptr) (union taskref){.local = _ptr}

extern dart_taskqueue_t local_deferred_tasks DART_INTERNAL;

/**
 * A dependency type for input dependencies that should be inserted
 * instead of appended, e.g., for a task created remotely.
 * This dependency will cause additional depdencies to be inserted for later
 * (already existing) tasks.
 * For internal use only.
 */
#define DART_DEP_DELAYED_IN (DART_DEP_IGNORE + 1)


/**
 * Initialize the data dependency management system.
 */
dart_ret_t dart_tasking_datadeps_init() DART_INTERNAL;

dart_ret_t dart_tasking_datadeps_reset(dart_task_t *task) DART_INTERNAL;

dart_ret_t dart_tasking_datadeps_fini() DART_INTERNAL;

/**
 * Search for tasks that satisfy the data dependencies of this task.
 */
dart_ret_t dart_tasking_datadeps_handle_task(
    dart_task_t           *task,
    const dart_task_dep_t *deps,
    size_t                 ndeps) DART_INTERNAL;

/**
 * Look for the latest task that satisfies \c dep and add \rtask
 * to the remote successor list.
 * Note that \c dep has to be a IN dependency.
 */
dart_ret_t dart_tasking_datadeps_handle_remote_task(
    const dart_task_dep_t  *dep,
    const taskref           remote_task,
    dart_global_unit_t      origin) DART_INTERNAL;


/**
 * End a phase, e.g., by releasing any unhandled remote dependency of the
 * same phase.
 */
dart_ret_t dart_tasking_datadeps_end_phase(uint64_t phase) DART_INTERNAL;

/**
 * Release the dependencies of \c task, potentially enqueuing t
 * asks into the runnable queue of \c thread.
 */
dart_ret_t
dart_tasking_datadeps_release_local_task(
  dart_task_t   *task,
  dart_thread_t *thread) DART_INTERNAL;

/**
 * Release a remote dependency \c elem after it has finished
 * execution.
 *
 * This is called from the remote side.
 */
dart_ret_t dart_tasking_datadeps_release_remote_dep(
    dart_dephash_elem_t *elem) DART_INTERNAL;

/**
 * Release a local task \c local_task.
 * The dependency reference will be stored and sent back later to release the
 * corresponding dependency on the remote side.
 *
 * This is called from the remote side.
 */
dart_ret_t dart_tasking_datadeps_release_remote_task(
  dart_task_t        *local_task,
  uintptr_t           elem,
  dart_global_unit_t  unit) DART_INTERNAL;


/**
 * Cancel all remaining remote dependencies.
 * All tasks that are still blocked by remote dependencies will be subsequently
 * released if they have no local dependecies.
 */
dart_ret_t dart_tasking_datadeps_cancel_remote_deps() DART_INTERNAL;

/**
 * Release all unhandled remote dependency requests.
 * This should be done before starting to execute local tasks
 * to avoid deadlocks.
 */
dart_ret_t
dart_tasking_datadeps_handle_defered_remote() DART_INTERNAL;

/**
 * Release local tasks whose releases have been deferred.
 * Tasks may have gained remote (direct) dependencies so not all tasks may be
 * released.
 */
dart_ret_t
dart_tasking_datadeps_handle_defered_local() DART_INTERNAL;

DART_INLINE
bool
dart_tasking_datadeps_is_runnable(dart_task_t *task) {
  return (task->unresolved_deps == 0) && (task->unresolved_remote_deps == 0);
}


DART_INLINE
dart_gptr_t
dart_tasking_datadeps_localize_gptr(dart_gptr_t gptr)
{
  void *addr;
  dart_gptr_t res = gptr;
  // segment IDs <0 are reserved for attached memory so tey already contain
  // absolute addresses.
  if (gptr.segid >= 0) {
    dart_ret_t ret = dart_gptr_getaddr(gptr, &addr);
    DART_ASSERT_MSG(ret == DART_OK, "Failed to translate gptr={u.=%d,s=%d,o=%p}",
                    gptr.unitid, gptr.segid, gptr.addr_or_offs.addr);
    DART_ASSERT(addr != NULL);
    res.addr_or_offs.addr = addr;
  }
  res.segid = DART_TASKING_DATADEPS_LOCAL_SEGID;

  if (gptr.teamid != DART_TEAM_ALL) {
    dart_global_unit_t guid;
    dart_myid(&guid);
    res.unitid = guid.id;
    res.teamid = DART_TEAM_ALL;
  }

  DART_LOG_TRACE("Localized gptr: [u:%d,t:%d,s:%d,o:%p] -> [u:%d,t:%d,s:%d,o:%p]",
                 gptr.unitid, gptr.teamid, gptr.segid, gptr.addr_or_offs.addr,
                 res.unitid, res.teamid, res.segid, res.addr_or_offs.addr);

  return res;
}


/**
 * Check for new remote task dependency requests coming in
 */
dart_ret_t dart_tasking_datadeps_progress() DART_INTERNAL;

#endif /* DART_TASKING_DATADEPS_H_ */
