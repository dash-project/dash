#ifndef DART_TASKING_REMOTE_H_
#define DART_TASKING_REMOTE_H_

#include <dash/dart/tasking/dart_tasking_priv.h>
#include <dash/dart/tasking/dart_tasking_datadeps.h>

#include <stdbool.h>

/**
 * @brief Initialize the remote task dependency system.
 */
dart_ret_t dart_tasking_remote_init() DART_INTERNAL;

dart_ret_t dart_tasking_remote_fini() DART_INTERNAL;

/**
 * Send a remote data dependency request for dependency \c dep of \c task
 */
dart_ret_t
dart_tasking_remote_datadep(
  dart_task_dep_t *dep,
  dart_task_t     *task) DART_INTERNAL;

/**
 * Send a release for the remote task \c rtask to \c unit, potentially
 * enqueuing rtask into the runnable list on the remote side.
 */
dart_ret_t dart_tasking_remote_release_dep(
  dart_global_unit_t     unit,
  taskref                rtask,
  uintptr_t              depref) DART_INTERNAL;

/**
 * Send a release for the remote task \c rtask to \c unit, potentially
 * enqueuing rtask into the runnable list on the remote side.
 */
dart_ret_t dart_tasking_remote_release_task(
  dart_global_unit_t unit,
  taskref            rtask,
  uintptr_t          depref) DART_INTERNAL;

/**
 * Send a request to the remote unit to create a task that will send us data
 * as part of a pre-fetch copyin.
 */
dart_ret_t dart_tasking_remote_sendrequest(
  dart_global_unit_t     unit,
  dart_gptr_t            src_gptr,
  size_t                 num_bytes,
  int                    tag,
  dart_taskphase_t       phase) DART_INTERNAL;

/**
 * Broadcast the request to cancel execution of remaining tasks.
 */
dart_ret_t dart_tasking_remote_bcast_cancel(dart_team_t team) DART_INTERNAL;

/**
 * Check for new remote task dependency requests coming in.
 * The call immediately returns if another thread is currently
 * processing the queue and will only process one chunk of data.
 */
dart_ret_t dart_tasking_remote_progress() DART_INTERNAL;


/**
 * Check for new remote task dependency requests coming in.
 *
 * This call is similar to \ref dart_tasking_remote_progress
 * but blocks if another process is currently processing the
 * message queue. The call will block until no further incoming
 * messages are received.
 */
dart_ret_t dart_tasking_remote_progress_blocking(dart_team_t team) DART_INTERNAL;

/**
 * Pass a task to the remote communication engine. If the task is not handled
 * by the engine (i.e., no progress thread exists) \c enqueued will be set
 * to \c false, otherwise it's set to \c true in which case the task
 * should not be enqueued anywhere else.
 */
void dart_tasking_remote_handle_comm_task(
  dart_task_t *task,
  bool        *enqueued) DART_INTERNAL;

#endif /* DART_TASKING_REMOTE_H_ */
