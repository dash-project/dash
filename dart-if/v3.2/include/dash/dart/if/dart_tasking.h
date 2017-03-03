#ifndef DART__TASKING_H_
#define DART__TASKING_H_

/**
 * \file dart_tasking.h
 *
 * An interface for creating (and waiting for completion of) units of work that be either executed
 * independently or have explicitly stated data dependencies.
 * The scheduler will take care of data dependencies, which can be either local or global, i.e., tasks
 * can specify dependencies to data on remote units.
 */

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_globmem.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum dart_task_deptype {
  DART_DEP_IN,
  DART_DEP_OUT,
  DART_DEP_INOUT,
  DART_DEP_RES,
  DART_DEP_DIRECT
} dart_task_deptype_t;

typedef struct dart_task_dep {
  dart_gptr_t         gptr;
  dart_task_deptype_t type;
} dart_task_dep_t;

typedef struct dart_task_data dart_task_t;

typedef dart_task_t* dart_taskref_t;

/**
 * Initialize the tasking environment, i.e., create the a thread-pool waiting for tasks.
 */
dart_ret_t
dart_tasking_init();

/**
 * Returns the current thread's number.
 */
int
dart_tasking_thread_num() __attribute__((weak));

/**
 * Returns the number of worker threads.
 */
int
dart_tasking_num_threads();


/**
 * Add a task to the local task graph with dependencies.
 * Tasks may define nested tasks. At the moment, tasks wait for all
 * child tasks to finish before finishing their execution.
 *
 * Data dependencies are one of \c DART_DEP_IN, \c DART_DEP_OUT, or
 * \c DART_DEP_INOUT and contain a \c dart_gptr_t that describes
 * the target of the dependency in the global address space.
 * Note that remote OUT and INOUT dependencies are currently not supported.
 *
 */
dart_ret_t
dart_task_create(
  void (*fn) (void *),
  void *data,
  size_t data_size,
  dart_task_dep_t *deps,
  size_t ndeps);


/**
 * Add a task to the local task graph with dependencies.
 * This function is similar to \ref dart_task_create but
 * also returns a reference to the created task, which can
 * be used to wait for completion of that task in
 * \c dart_task_wait.
 * The resources allocated for \c taskref are released
 * through a call to \c dart_task_wait.
 */
dart_ret_t
dart_task_create_handle(
  void           (*fn) (void *),
  void            *data,
  size_t           data_size,
  dart_task_dep_t *deps,
  size_t           ndeps,
  dart_taskref_t  *taskref);

/**
 * Wait for the completion of a task created through
 * \c dart_task_create_handle.
 * A task can only be waited on once, passing the same
 * task reference to \c dart_task_wait twice is erroneous.
 */
dart_ret_t
dart_task_wait(dart_taskref_t *taskref);


dart_taskref_t
dart_tasking_current_task();


/**
 * Wait for all child tasks to complete.
 * If the current task is the (implicit) root task, this call will
 * wait for all previously defined tasks to complete.
 * Otherwise, the call will return as soon as all child tasks of the current
 * task have finished.
 */
dart_ret_t
dart_task_complete();


/**
 * Finalize and release all resource acquired during initialization.
 */
dart_ret_t
dart_tasking_fini();

/**
 * Signal the end of an phase (or iteration) and the beginning of a new phase.
 *
 * This should be used to ensure remote dependencies
 */
dart_ret_t
dart_tasking_phase();

#ifdef __cplusplus
}
#endif


#endif /* DART__TASKING_H_ */
