/*
 * dart_tasking_datadeps.h
 *
 *  Created on: Nov 8, 2016
 *      Author: joseph
 */

#ifndef DART_TASKING_DATADEPS_H_
#define DART_TASKING_DATADEPS_H_

#include <dash/dart/tasking/dart_tasking_priv.h>

typedef void* remote_task_t;

typedef union taskref {
  dart_task_t              *local;
  remote_task_t        remote;
} taskref;


/**
 * @brief Initialize the data dependency management system.
 */
dart_ret_t dart_tasking_datadeps_init();

dart_ret_t dart_tasking_datadeps_fini();

/**
 * @brief Search for tasks that satisfy the data dependencies of this task.
 */
dart_ret_t dart_tasking_datadeps_handle_task(dart_task_t *task, dart_task_dep_t *deps, size_t ndeps);

/**
 * @brief Look for the latest task that satisfies \c dep and add \rtask to the remote successor list.
 * Note that \c dep has to be a IN dependency.
 */
dart_ret_t dart_tasking_datadeps_handle_remote_task(const dart_task_dep_t *dep, const taskref remote_task, dart_global_unit_t origin);

/**
 * @brief Handle the direct task dependency between a local task and it's remote successor
 */
dart_ret_t dart_tasking_datadeps_handle_remote_direct(dart_task_t *local_task, taskref remote_task, dart_global_unit_t origin);

/**
 * Release the dependencies of \c task, potentially enqueuing tasks into the runnable
 * queue of \c thread.
 */
dart_ret_t
dart_tasking_datadeps_release_local_task(dart_thread_t *thread, dart_task_t *task);

/**
 * @brief Check for new remote task dependency requests coming in
 */
dart_ret_t dart_tasking_datadeps_progress();

#endif /* DART_TASKING_DATADEPS_H_ */
