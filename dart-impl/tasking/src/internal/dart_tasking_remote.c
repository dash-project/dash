
#include <dash/dart/base/logging.h>
#include <dash/dart/base/atomic.h>
#include <dash/dart/if/dart_tasking.h>
#include <dash/dart/if/dart_active_messages.h>
#include <dash/dart/tasking/dart_tasking_remote.h>
#include <dash/dart/tasking/dart_tasking_priv.h>
#include <dash/dart/tasking/dart_tasking_taskqueue.h>

#include <pthread.h>
#include <stdbool.h>


static dart_amsgq_t amsgq;
#define DART_RTASK_QLEN 256

static bool initialized = false;


struct remote_data_dep {
  /** Global pointer to the data \c rtask depends on */
  dart_gptr_t gptr;
  /** The remote (origin) unit ID */
  dart_unit_t runit;
  /** pointer to a task on the origin unit. Only valid at the origin! */
  taskref     rtask;
};

struct remote_task_dep {
  /**
   * Pointer to a task at the target on which \c successor depends on.
   * Only valid on the target!
   */
  dart_task_t     *task;
  /** Pointer to a task that depends on \c task. Only valid on the origin! */
  dart_task_t     *successor;
  /** The origin unit of the request */
  dart_unit_t runit;
};


/**
 * Forward declarations of remote tasking actions
 */
static void
enqueue_from_remote(void *data);
static void
release_remote_dependency(void *data);
static void
request_direct_taskdep(void *data);


dart_ret_t dart_tasking_remote_init()
{
  if (!initialized) {
    amsgq  = dart_amsg_openq(sizeof(struct remote_data_dep) * DART_RTASK_QLEN, DART_TEAM_ALL);
    DART_LOG_INFO("Created active message queue for remote tasking (%p)", amsgq);
    initialized = true;
  }
  return DART_OK;
}

dart_ret_t dart_tasking_remote_fini()
{
  if (initialized)
  {
    dart_amsg_closeq(amsgq);
    initialized = false;
  }
  return DART_OK;
}

/**
 * @brief Send a remote data dependency request for dependency \c dep of the local \c task
 */
dart_ret_t dart_tasking_remote_datadep(dart_task_dep_t *dep, dart_task_t *task)
{
  dart_ret_t ret;
  struct remote_data_dep rdep;
  rdep.gptr = dep->gptr;
  rdep.rtask.local = task;
  dart_myid(&rdep.runit);

  while (1) {
    ret = dart_amsg_trysend(dep->gptr.unitid, amsgq, &enqueue_from_remote, &rdep, sizeof(rdep));
    if (ret == DART_OK) {
      // the message was successfully sent
      DART_LOG_INFO("Sent remote dependency request (unit=%i, segid=%i, offset=%p, fn=%p)", gptr.unitid, gptr.segid, gptr.addr_or_offs.offset, &enqueue_from_remote);
      break;
    } else  if (ret == DART_ERR_AGAIN) {
      // cannot be sent at the moment, just try again
      // TODO: anything more sensible to do here? We could execute a task in between...
      continue;
    } else {
      DART_LOG_ERROR("Failed to send active message to unit %i", dep->gptr.unitid);
      return DART_ERR_OTHER;
    }
  }
  return DART_OK;
}

/**
 * @brief Send a release for the remote task \c rtask to \c unit, potentially enqueuing rtask into the
 *        runnable list on the remote side.
 */
dart_ret_t dart_tasking_remote_release(dart_unit_t unit, taskref rtask, dart_task_dep_t *dep)
{
  struct remote_data_dep response;
  response.rtask = rtask;
  response.gptr = dep->gptr;
  while (1) {
    dart_ret_t ret;
    ret = dart_amsg_trysend(unit, amsgq, &release_remote_dependency, &response, sizeof(response));
    if (ret == DART_OK) {
      // the message was successfully sent
      DART_LOG_INFO("Sent remote dependency release to unit %i (segid=%i, offset=%p, fn=%p)",
          rtask->runit, rtask->gptr.segid, rtask->gptr.addr_or_offs.offset, &enqueue_from_remote);
      break;
    } else  if (ret == DART_ERR_AGAIN) {
      // cannot be sent at the moment, just try again
      // TODO: anything more sensible to do here?
      continue;
    } else {
      DART_LOG_ERROR("Failed to send active message to unit %i", unit);
      return DART_ERR_OTHER;
    }
  }

  return DART_OK;
}


/**
 * @brief Send a direct task dependency request to \c unit to make sure
 *        that \c local_task is only executed after \c remote_task has finished and sent a release.
 */
dart_ret_t dart_tasking_remote_direct_taskdep(dart_unit_t unit, dart_task_t *local_task, taskref remote_task)
{
  struct remote_task_dep taskdep;
  taskdep.task = remote_task.local;
  taskdep.successor = local_task;
  dart_myid(&taskdep.runit);

  while (1) {
    dart_ret_t ret;
    ret = dart_amsg_trysend(unit, amsgq, &request_direct_taskdep, &taskdep, sizeof(taskdep));
    if (ret == DART_OK) {
      // the message was successfully sent
      DART_LOG_INFO("Sent direct remote task dependency to unit %i (local task %p depdends on remote task %p)",
          unit, local_task, remote_task);
      break;
    } else  if (ret == DART_ERR_AGAIN) {
      // cannot be sent at the moment, just try again
      // TODO: anything more sensible to do here?
      continue;
    } else {
      DART_LOG_ERROR("Failed to send active message to unit %i", unit);
      return DART_ERR_OTHER;
    }
  }

  return DART_OK;
}


/**
 * @brief Check for new remote task dependency requests coming in
 */
dart_ret_t dart_tasking_remote_progress()
{
  return dart_amsg_process(amsgq);
}



/**************************
 * Remote tasking actions *
 **************************/


/**
 * Action called by a remote unit to register a task dependency.
 * The remote unit provides a (remote) pointer to a task that depends
 * on a gptr local to our unit. We create a task that has this dependence
 * and when executed sends a release back to the origin unit.
 */
static void
enqueue_from_remote(void *data)
{
  struct remote_data_dep *rdep = (struct remote_data_dep *)data;
  dart_task_dep_t dep;
  dep.gptr = rdep->gptr;
  dep.type = DART_DEP_IN;
  taskref rtask = rtask;
  DART_LOG_INFO("Adding task %p from remote dependency request from unit %i (unit=%i, segid=%i, addr=%p)",
                      rdep->rdep, rdep->gptr.unitid, rdep->gptr.segid, rdep->gptr.addr_or_offs.addr);
  dart_tasking_datadeps_handle_remote_task(&dep, rtask, rdep->runit);
}

/**
 * An action called from the remote unit to signal the release of a remote dependency.
 * The remote unit sends back a pointer to the local task object so we can easily
 * decrement the dependency counter and enqueue the task if possible.
 */
static void release_remote_dependency(void *data)
{
  struct remote_data_dep *response = (struct remote_data_dep *)data;
  dart_task_t *task = response->rtask.local;
  DART_LOG_INFO("Received remote dependency release from unit %i for task %p (segid=%i, offset=%p)",
    response->runit, task, response->gptr.segid, response->gptr.addr_or_offs.offset);

  int unresolved_deps = DART_FETCH_AND_DEC32(&task->unresolved_deps);

  if (unresolved_deps < 0) {
    DART_LOG_ERROR("ERROR: task with remote dependency does not seem to have unresolved dependencies!");
  } else if (task->unresolved_deps == 0) {
    // enqueue as runnable
    dart_tasking_taskqueue_push(&dart__base__tasking_current_thread()->queue, task);
  }
}

/**
 *
 */
static void
request_direct_taskdep(void *data)
{
  struct remote_task_dep *taskdep = (struct remote_task_dep*) data;
  taskref successor = {taskdep->successor};
  dart_tasking_datadeps_handle_remote_direct(taskdep->task, successor, taskdep->runit);
}
