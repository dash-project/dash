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

#include <stdbool.h>
#include <limits.h>
#include <stdint.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_globmem.h>
#include <dash/dart/if/dart_communication.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dart_task_data dart_task_t;
typedef dart_task_t* dart_taskref_t;

typedef int32_t dart_taskphase_t;

#define DART_TASK_NULL ((dart_taskref_t)NULL)

/// the dependency can refer to any previous task (used for local dependencies)
#define DART_PHASE_ANY   ((dart_taskphase_t)INT_MAX)
/// the first dependency phase, which can be executed without synchronization
#define DART_PHASE_FIRST ((dart_taskphase_t)-1)
/// the dependency should assume the phase of the task it belongs to
#define DART_PHASE_TASK  ((dart_taskphase_t)-2)

typedef enum {
  DART_PRIO_PARENT = -1, // << inherit the priority from the parent
  DART_PRIO_LOW    =  0, // << low priority
  DART_PRIO_DEFAULT,     // << default (medium) priority
  DART_PRIO_HIGH,        // << high priority
  __DART_PRIO_COUNT      // << the number of priorities, do not use as task priority!
} dart_task_prio_t;

typedef enum dart_task_deptype {
  DART_DEP_IN,
  DART_DEP_OUT,
  DART_DEP_INOUT,
  DART_DEP_COPYIN,
  DART_DEP_COPYIN_R, // like copyin, only copies the data if the target is non-local
  DART_DEP_DIRECT,
  DART_DEP_IGNORE  // should always be the last
} dart_task_deptype_t;

typedef enum dart_task_flags {
  DART_TASK_NOYIELD = 1
} dart_task_flags_t;

typedef struct dart_task_dep {
  union {
    /// use for type INPUT, OUTPUT or INOUT dependencies
    dart_gptr_t       gptr;
    /// use for type DIRECT dependencies
    dart_taskref_t    task;
    /// use for type COPYIN -- TODO: add src/dest data types here and hide behind a pointer
    struct {
      dart_gptr_t  gptr; // the src, has to match an OUTPUT dependency
      void       * dest; // the destination
      size_t       size; // the number of consecutive bytes to copy
    }               copyin;
  };
  /// dependency type, see \ref dart_task_deptype_t
  dart_task_deptype_t type;
  /// the phase this dependency refers to
  /// INPUT dependencies refer to any previous phase
  /// OUTPUT dependencies refer to this phase
  dart_taskphase_t   phase;
} dart_task_dep_t;

/**
 * The maximum size of data a task can store inline, i.e., without additional
 * allocation.
 * The size is chosen to minimize padding while guaranteeing at least 32B space.
 */
#define DART_TASKING_INLINE_DATA_SIZE 96

/**
 * Returns the current thread's number.
 */
int
dart_task_thread_num() __attribute__((weak));

/**
 * Returns the number of worker threads.
 */
int
dart_task_num_threads() __attribute__((weak));


/**
 * Add a task to the local task graph with dependencies.
 * Tasks may define nested tasks. At the moment, tasks wait for all
 * child tasks to finish before finishing their execution.
 *
 * Data dependencies are one of \c DART_DEP_IN, \c DART_DEP_OUT, or
 * \c DART_DEP_INOUT and contain a \c dart_gptr_t that describes
 * the target of the dependency in the global address space.
 * Note that remote OUT and INOUT dependencies are currently not supported.
 */
dart_ret_t
dart_task_create(
        void           (*fn) (void *),
        void            *data,
        size_t           data_size,
        dart_task_dep_t *deps,
        size_t           ndeps,
        dart_task_prio_t prio,
        int              flags,
  const char            *descr);

/**
 * Free a task reference obtained from \c dart_task_create_handle without
 * waiting for the task's completion. The reference is invalidated and cannot
 * be used afterwards.
 */
dart_ret_t
dart_task_freeref(dart_taskref_t *taskref);


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
        dart_task_prio_t prio,
        int              flags,
        dart_taskref_t  *taskref);


/**
 * Wait for the completion of a task created through
 * \c dart_task_create_handle.
 * A task can only be waited on once, passing the same
 * task reference to \c dart_task_wait twice is erroneous.
 */
dart_ret_t
dart_task_wait(dart_taskref_t *taskref);

/**
 * Test for the completion of a task created through
 * \c dart_task_create_handle.
 * If the task has finished execution, the parameter \c flag will be set to 1
 * and the handle should not be waited or tested on later, i.e., do not call
 * \ref dart_task_wait on the same handle if the test was successful.
 */
dart_ret_t
dart_task_test(dart_taskref_t *taskref, int *done);

dart_taskref_t
dart_task_current_task();

/**
 * Wait for all child tasks to complete.
 * If the current task is the (implicit) root task, this call will
 * wait for all previously defined tasks to complete.
 * Otherwise, the call will return as soon as all child tasks of the current
 * task have finished.
 */
dart_ret_t
dart_task_complete(bool local_only);

/**
 * Cancel the current task and start the cancellation on a global scale, i.e.,
 * signal cancellation to all local and remote tasks in units in DART_TEAM_ALL.
 * This method should be called by a single global task. For collective
 * cancellation use \ref dart_task_cancel_global.
 * There may not be two cancellation requests in flight at the
 * same time.
 * This method does not return.
 */
void
dart_task_cancel_bcast() DART_NORETURN;

/**
 * Cancel the current task and start the cancellation locally, i.e., signalling
 * cancellation to all local threads.
 * This method has to be called from all units in DART_TEAM_ALL.
 * There may not be two cancellation requests in flight at the
 * same time.
 * This method does not return.
 */
void
dart_task_cancel_barrier() DART_NORETURN;

/**
 * Abort the execution of the current task and continue with the next task
 * (unless there has been a previous cancellation request).
 */
void
dart_task_abort() DART_NORETURN;

/**
 * Returns true if cancellation has previously been requested.
 * This can be used in combination with \ref dart_task_abort to cleanup
 * before aborting the current task to join the cancellation process.
 */
bool
dart_task_should_abort();

/**
 * Yield the execution thread to execute another task.
 *
 * The current task will be re-inserted into the current thread's
 * task queue. The parameter \c delay determines the position at which the
 * task is enqueued, with non-negative integers denoting the reinsertion
 * position, starting from 0 as being the new head of the queue after dequeing
 * a replacement.
 * A value of -1 enforces this task to be placed at the end of the queue.
 * Note that yielded tasks are subject to work-stealing as well, allowing
 * other threads to pick up the current task later.
 */
dart_ret_t
dart_task_yield(int delay) __attribute__((weak));


/**
 * Yield the execution thread until all \c num_handle operations in \c handle
 * have completed.
 */
dart_ret_t
dart_task_wait_handle(dart_handle_t *handle, size_t num_handle) __attribute__((weak));

/**
 * End the execution of the task but do not release it's dependencies until
 * all \c num_handle operations in \c handle have completed.
 */
dart_ret_t
dart_task_detach_handle(dart_handle_t *handle, size_t num_handle);

/**
 * Advance to the next task execution phase.
 * This function is not blocking and only increments a counter that
 * represents the current phase. It is the user's responsibility to keep
 * the phase counter in sync.
 */
void
dart_task_phase_advance();

/**
 * Returns the current value of the phase counter.
 */
dart_taskphase_t
dart_task_phase_current();

/**
 * Resyncs the phases among all units to the highest phase and avances it.
 * This is useful if at some pointer phases may have diverged, i.e., due to
 * task creation on subteams.
 * \note This involves a blocking collective operation on all units in \c team.
 */
dart_ret_t
dart_task_phase_resync(dart_team_t team);

void *
dart_task_copyin_info(dart_taskref_t task, int depnum);


#ifdef __cplusplus
}
#endif


#endif /* DART__TASKING_H_ */
