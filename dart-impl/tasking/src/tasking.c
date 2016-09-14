
#include <dash/dart/if/dart_tasking.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/mpi/dart_initialization.h>
#include <dash/dart/tasking/dart_tasking_priv.h>


/**
 * \brief Initialize the tasking environment, i.e., create the a thread-pool waiting for tasks.
 */
dart_ret_t
dart_tasking_init()
{
  return dart__base__tasking__init();
}

/**
 * \brief Finalize and release all resource acquired during initialization.
 */
dart_ret_t
dart_tasking_fini()
{
  return dart__base__tasking__fini();
}

int
dart_tasking_thread_num()
{
  return dart__base__tasking__thread_num();
}

int
dart_tasking_num_threads()
{
  return dart__base__tasking__num_threads();
}

/**
 * \brief Add a task the local task graph with dependencies. Tasks may define new tasks if necessary.
 */
dart_ret_t
dart_task_create(void (*fn) (void *), void *data, dart_task_dep_t *deps, size_t ndeps)
{
  return dart__base__tasking__create_task(fn, data, deps, ndeps);
}

/**
 * \brief Wait for all defined tasks to complete.
 */
dart_ret_t
dart_task_complete()
{
  return dart__base__tasking__task_complete();
}
