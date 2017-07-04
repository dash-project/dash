
#include <dash/dart/if/dart_tasking.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/tasking/dart_tasking_priv.h>


/**
 * \brief Initialize the tasking environment, i.e., create the a thread-pool waiting for tasks.
 */
dart_ret_t
dart_tasking_init()
{
  return dart__tasking__init();
}

/**
 * \brief Finalize and release all resource acquired during initialization.
 */
dart_ret_t
dart_tasking_fini()
{
  return dart__tasking__fini();
}

int
dart_task_thread_num()
{
  return dart__tasking__thread_num();
}

int
dart_task_num_threads()
{
  return dart__tasking__num_threads();
}

/**
 * Add a task to the local task graph with dependencies.
 * Tasks may define new tasks if necessary.
 *
 * The \c data argument will be passed to the action \c fn.
 * If \c data_size is non-zero, the content of \c data
 * will be copied, the copy will be passed to \c fn
 * and free'd upon completion of \c fn.
 */
dart_ret_t
dart_task_create(
  void           (*fn) (void *),
  void            *data,
  size_t           data_size,
  dart_task_dep_t *deps,
  size_t           ndeps)
{
  return dart__tasking__create_task(
                  fn, data,
                  data_size,
                  deps, ndeps);
}

/**
 * Add a task to the local task graph with dependencies.
 * This function is similar to \ref dart_task_create but
 * also returns a reference to the created task, which can
 * be used to wait for completion of that task in
 * \c dart_task_wait.
 */
dart_ret_t
dart_task_create_handle(
  void           (*fn) (void *),
  void            *data,
  size_t           data_size,
  dart_task_dep_t *deps,
  size_t           ndeps,
  dart_taskref_t  *taskref)
{
  return dart__tasking__create_task_handle(
                 fn, data,
                 data_size,
                 deps, ndeps,
                 taskref);
}



dart_taskref_t
dart_task_current_task()
{
  return dart__tasking__current_task();
}

/**
 * Wait for the completion of a task created through
 * \c dart_task_create_handle.
 * A task can only be waited on once, passing the same
 * task reference to \c dart_task_wait twice is erroneous.
 */
dart_ret_t
dart_task_wait(dart_taskref_t *taskref)
{
  return dart__tasking__task_wait(taskref);
}

/**
 * Wait for all defined tasks to complete.
 */
dart_ret_t
dart_task_complete()
{
  return dart__tasking__task_complete();
}

/**
 * Yield the execution thread to execute another task.
 */
dart_ret_t
dart_task_yield(int delay)
{
  return dart__tasking__yield(delay);
}


