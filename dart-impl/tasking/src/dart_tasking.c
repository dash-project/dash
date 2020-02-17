
#include <dash/dart/if/dart_tasking.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/tasking/dart_tasking_priv.h>
#include <dash/dart/tasking/dart_tasking_cancellation.h>
#include <dash/dart/tasking/dart_tasking_wait.h>


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
  const dart_task_dep_t *deps,
        size_t           ndeps,
        dart_task_prio_t prio,
  const char            *descr)
{
  return dart__tasking__create_task(
                  fn, data,
                  data_size,
                  deps, ndeps,
                  prio, descr, NULL);
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
  const dart_task_dep_t *deps,
        size_t           ndeps,
        dart_task_prio_t prio,
        dart_taskref_t  *taskref)
{
  return dart__tasking__create_task(
                 fn, data,
                 data_size,
                 deps, ndeps,
                 prio, NULL, taskref);
}



dart_taskref_t
dart_task_current_task()
{
  return dart__tasking__current_task();
}

/**
 * Free a task reference obtained from \c dart_task_create_handle without
 * waiting for the task's completion. The reference is invalidated and cannot
 * be used afterwards.
 */
dart_ret_t
dart_task_freeref(dart_taskref_t *taskref)
{
  return dart__tasking__taskref_free(taskref);
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
 * Test for the completion of a task created through
 * \c dart_task_create_handle.
 * If the task has finished execution, the parameter \c flag will be set to 1
 * and the handle should not be waited or tested on later, i.e., do not call
 * \ref dart_task_wait on the same handle if the test was successful.
 */
dart_ret_t
dart_task_test(dart_taskref_t *taskref, int *flag)
{
  return dart__tasking__task_test(taskref, flag);
}

/**
 * Wait for all discovered tasks to complete.
 * If \c local_only is true, then no matching is performed and the call is not
 * collective among all units.
 * Otherwise, the call is collective across all units and matching will be
 * performed.
 */
dart_ret_t
dart_task_complete(bool local_only)
{
  return dart__tasking__task_complete(local_only);
}


/**
 * Cancel the current task and start the cancellation on a global scale, i.e.,
 * signal cancellation to all local and remote threads in units in DART_TEAM_ALL.
 * There may not be two cancellation requests in flight at the
 * same time.
 */
void
dart_task_cancel_bcast() {
  dart__tasking__cancel_bcast();
}

/**
 * Cancel the current task and start the cancellation locally, i.e., signalling
 * cancellation to all local threads.
 * This method has to be called from all units in DART_TEAM_ALL.
 * There may not be two cancellation requests in flight at the
 * same time.
 */
void
dart_task_cancel_barrier()
{
  dart__tasking__cancel_barrier();
}


void
dart_task_abort()
{
  dart__tasking__abort();
}

bool
dart_task_should_abort()
{
  return dart__tasking__should_abort();
}

/**
 * Yield the execution thread to execute another task.
 */
dart_ret_t
dart_task_yield(int delay)
{
  return dart__tasking__yield(delay);
}

/**
 * Yield the execution thread until all \c num_handle operations in \c handle
 * have completed.
 */
dart_ret_t
dart_task_wait_handle(dart_handle_t *handle, size_t num_handle)
{
  return dart__task__wait_handle(handle, num_handle);
}


dart_ret_t
dart_task_detach_handle(dart_handle_t *handle, size_t num_handle)
{
  return dart__task__detach_handle(handle, num_handle);
}

void
dart_task_phase_advance()
{
  dart__tasking__phase_advance();
}

dart_taskphase_t
dart_task_phase_current()
{
  return dart__tasking__phase_current();
}

dart_ret_t
dart_task_phase_resync(dart_team_t team)
{
  return dart__tasking__phase_resync(team);
}

const char *
dart_task_current_task_descr()
{
  return dart__tasking__get_current_task_descr();
}
