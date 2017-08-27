#ifndef DART_TASKING_REMOTE_H_
#define DART_TASKING_REMOTE_H_

#include <dash/dart/tasking/dart_tasking_priv.h>
#include <dash/dart/tasking/dart_tasking_datadeps.h>

#include <stdbool.h>

/**
 * @brief Initialize the remote task dependency system.
 */
dart_ret_t dart_tasking_remote_init();

dart_ret_t dart_tasking_remote_fini();

/**
 * Send a remote data dependency request for dependency \c dep of \c task
 */
dart_ret_t
dart_tasking_remote_datadep(
  dart_task_dep_t *dep,
  dart_task_t     *task);

/**
 * Send a direct task dependency request \c unit to make sure
 * that \c local_task is only executed after \c remote_task has
 * finished and sent a release.
 */
dart_ret_t
dart_tasking_remote_direct_taskdep(
  dart_global_unit_t  unit,
  dart_task_t        *local_task,
  taskref             remote_task);

/**
 * Send a release for the remote task \c rtask to \c unit, potentially
 * enqueuing rtask into the runnable list on the remote side.
 */
dart_ret_t dart_tasking_remote_release(
  dart_global_unit_t     unit,
  taskref                rtask,
  const dart_task_dep_t *dep);

/**
 * Cancel a remote dependency request, potentially cancelling the remote task.
 */
dart_ret_t dart_tasking_remote_cancel_taskdep(
  dart_global_unit_t   unit,
  taskref              remote_task);

/**
 * Check for new remote task dependency requests coming in.
 * The call immediately returns if another thread is currently
 * processing the queue and will only process one chunk of data.
 */
dart_ret_t dart_tasking_remote_progress();


/**
 * Check for new remote task dependency requests coming in.
 *
 * This call is similar to \ref dart_tasking_remote_progress
 * but blocks if another process is currently processing the
 * message queue. The call will block until no further incoming
 * messages are received.
 */
dart_ret_t dart_tasking_remote_progress_blocking();

#endif /* DART_TASKING_REMOTE_H_ */
