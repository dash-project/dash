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
  DART_DEP_INOUT
} dart_task_deptype_t;

typedef struct dart_task_dep {
  dart_gptr_t         gptr;
  dart_task_deptype_t type;
} dart_task_dep_t;


/**
 * \brief Initialize the tasking environment, i.e., create the a thread-pool waiting for tasks.
 */
dart_ret_t
dart_tasking_init();

/**
 * \brief Returns the current thread's number.
 */
int
dart_tasking_thread_num();

/**
 * \brief Returns the number of worker threads.
 */
int
dart_tasking_num_threads();


/**
 * \brief Add a task the local task graph with dependencies. Tasks may define new tasks if necessary.
 */
dart_ret_t
dart_task_create(void (*fn) (void *), void *data, size_t data_size, dart_task_dep_t *deps, size_t ndeps);


/**
 * \brief Wait for all defined tasks to complete.
 */
dart_ret_t
dart_task_complete();


/**
 * \brief Finalize and release all resource acquired during initialization.
 */
dart_ret_t
dart_tasking_fini();

#ifdef __cplusplus
}
#endif


#endif /* DART__TASKING_H_ */
