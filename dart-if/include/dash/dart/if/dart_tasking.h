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

typedef struct dart_task_data dart_task_t;
typedef dart_task_t* dart_taskref_t;

typedef enum dart_task_deptype {
  DART_DEP_IN,
  DART_DEP_OUT,
  DART_DEP_INOUT,
  DART_DEP_DIRECT,
  DART_DEP_IGNORE
} dart_task_deptype_t;

typedef struct dart_task_dep {
  union {
    /// use for type INPUT, OUTPUT or INOUT dependencies
    dart_gptr_t       gptr;
    /// use for type DIRECT dependencies
    dart_taskref_t    task;
  };
  /// dependency type, see \ref dart_task_deptype_t
  dart_task_deptype_t type;
  /// the epoch this dependency refers to
  int32_t             epoch;
} dart_task_dep_t;

#define DART_TASK_NULL ((dart_taskref_t)NULL)

#define DART_EPOCH_ANY ((int32_t)-1)

DART_INLINE
dart_task_dep_t dart_task_create_datadep(
  dart_gptr_t         gptr,
  dart_task_deptype_t type,
  int32_t             epoch)
{
  dart_task_dep_t res;
  res.gptr  = gptr;
  res.type  = type;
  res.epoch = epoch;
  return res;
}

DART_INLINE
dart_task_dep_t dart_task_create_local_datadep(
  void                * ptr,
  dart_task_deptype_t   type,
  int32_t               epoch)
{
  dart_task_dep_t res;
  res.gptr = DART_GPTR_NULL;
  res.gptr.addr_or_offs.addr = ptr;
  dart_global_unit_t myid;
  dart_myid(&myid);
  res.gptr.unitid = myid.id;
  res.gptr.teamid = DART_TEAM_ALL;
  res.epoch       = epoch;
  res.type        = type;
  return res;
}

DART_INLINE
dart_task_dep_t dart_task_create_directdep(
  dart_taskref_t      task)
{
  dart_task_dep_t res;
  res.task  = task;
  res.type  = DART_DEP_DIRECT;
  res.epoch = -1;
  return res;
}

/**
 * Returns the current thread's number.
 */
int
dart_task_thread_num() __attribute__((weak));

/**
 * Returns the number of worker threads.
 */
int
dart_task_num_threads();

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
dart_task_yield(int delay);


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
dart_task_current_task();


/**
 * Wait for all child tasks to complete.
 * If the current task is the (implicit) root task, this call will
 * wait for all previously defined tasks to complete.
 * Otherwise, the call will return as soon as all child tasks of the current
 * task have finished.
 */
dart_ret_t
dart_task_complete();

#ifdef __cplusplus
}
#endif


#endif /* DART__TASKING_H_ */
