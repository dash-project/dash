
#include <dash/dart/base/logging.h>
#include <dash/dart/base/atomic.h>
#include <dash/dart/base/assert.h>
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
  dart_gptr_t        gptr;
  /** The remote (origin) unit ID */
  dart_global_unit_t runit;
  /** pointer to a task on the origin unit. Only valid at the origin! */
  taskref            rtask;
  uint64_t           magic;
  uint64_t           phase;
};

struct remote_task_dep {
  /**
   * Pointer to a task at the target on which \c successor depends on.
   * Only valid on the target!
   */
  dart_task_t        *task;
  /** Pointer to a task that depends on \c task. Only valid on the origin! */
  dart_task_t        *successor;
  /** The origin unit of the request */
  dart_global_unit_t  runit;
  uint64_t            magic;
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
    amsgq  = dart_amsg_openq(sizeof(struct remote_data_dep), DART_RTASK_QLEN, DART_TEAM_ALL);
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
  rdep.gptr        = dep->gptr;
  rdep.rtask.local = task;
  rdep.magic       = 0xDEADBEEF;
  rdep.phase       = task->phase;
  dart_myid(&rdep.runit);
  dart_team_unit_t team_unit;
  dart_team_unit_g2l(DART_TEAM_ALL, DART_GLOBAL_UNIT_ID(dep->gptr.unitid), &team_unit);

  DART_ASSERT(task != NULL);

  while (1) {
    // the amsgq is opened on DART_TEAM_ALL
    ret = dart_amsg_trysend(team_unit, amsgq, &enqueue_from_remote, &rdep, sizeof(rdep));
    if (ret == DART_OK) {
      // the message was successfully sent
      int32_t unresolved_deps = DART_INC32_AND_FETCH(&task->unresolved_deps);
      DART_LOG_INFO("Sent remote dependency request for task %p (unit=%i, segid=%i, offset=%p, fn=%p, num_deps=%i)",
          task, dep->gptr.unitid, dep->gptr.segid, dep->gptr.addr_or_offs.offset, &enqueue_from_remote, unresolved_deps);
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
dart_ret_t dart_tasking_remote_release(dart_global_unit_t unit, taskref rtask, const dart_task_dep_t *dep)
{
  struct remote_data_dep response;
  response.rtask = rtask;
  response.gptr  = dep->gptr;
  response.magic = 0xDEADBEEF;
  dart_team_unit_t team_unit;
  dart_myid(&response.runit);
  dart_team_unit_g2l(DART_TEAM_ALL, unit, &team_unit);

  DART_ASSERT(rtask.remote != NULL);

  while (1) {
    dart_ret_t ret;
    ret = dart_amsg_trysend(team_unit, amsgq, &release_remote_dependency, &response, sizeof(response));
    if (ret == DART_OK) {
      // the message was successfully sent
      DART_LOG_INFO("Sent remote dependency release to unit t:%i (segid=%i, offset=%p, fn=%p, rtask=%p)",
          team_unit.id, response.gptr.segid, response.gptr.addr_or_offs.offset, &release_remote_dependency, rtask.local);
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
dart_ret_t dart_tasking_remote_direct_taskdep(dart_global_unit_t unit, dart_task_t *local_task, taskref remote_task)
{
  struct remote_task_dep taskdep;
  taskdep.task      = remote_task.local;
  taskdep.successor = local_task;
  taskdep.magic     = 0xDEADBEEF;
  dart_myid(&taskdep.runit);
  dart_team_unit_t team_unit;
  dart_team_unit_g2l(DART_TEAM_ALL, unit, &team_unit);

  DART_ASSERT(remote_task.local != NULL);
  DART_ASSERT(local_task != NULL);

  while (1) {
    dart_ret_t ret;
    ret = dart_amsg_trysend(team_unit, amsgq, &request_direct_taskdep, &taskdep, sizeof(taskdep));
    if (ret == DART_OK) {
      // the message was successfully sent
      DART_LOG_INFO("Sent direct remote task dependency to unit %i "
                    "(local task %p depdends on remote task %p)",
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
  DART_ASSERT(rdep->magic == 0xDEADBEEF);
  DART_ASSERT(rdep->rtask.remote != NULL);
  dart_phase_dep_t dep;
  dep.dep.gptr = rdep->gptr;
  dep.dep.type = DART_DEP_IN;
  dep.phase    = rdep->phase;
  taskref rtask = rdep->rtask;
  DART_LOG_INFO("Received remote dependency request for task %p "
                "(unit=%i, segid=%i, addr=%p, ph=%li)",
                rdep->rtask.remote, rdep->runit.id, rdep->gptr.segid,
                rdep->gptr.addr_or_offs.addr, dep.phase);
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
  DART_ASSERT(response->magic == 0xDEADBEEF);
  DART_ASSERT(response->rtask.local != NULL);
  dart_task_t *task = response->rtask.local;
  DART_LOG_INFO("release_remote_dependency : Received remote dependency "
                "release from unit %i for task %p (segid=%i, offset=%p, ph=%li)",
                response->runit.id, task, response->gptr.segid,
                response->gptr.addr_or_offs.offset);

  int unresolved_deps = DART_DEC32_AND_FETCH(&task->unresolved_deps);
  DART_LOG_DEBUG("release_remote_dependency : Task with remote dep %p has %i "
                 "unresolved dependencies left", task, unresolved_deps);
  if (unresolved_deps < 0) {
    DART_LOG_ERROR("ERROR: task %p with remote dependency does not seem to "
                   "have unresolved dependencies!", task);
  } else if (unresolved_deps == 0) {
    // enqueue as runnable
    dart_tasking_taskqueue_push(
        &dart__base__tasking_current_thread()->queue, task);
  }
}

/**
 *
 */
static void
request_direct_taskdep(void *data)
{
  struct remote_task_dep *taskdep = (struct remote_task_dep*) data;
  DART_ASSERT(taskdep->magic == 0xDEADBEEF);
  DART_ASSERT(taskdep->task != NULL);
  DART_ASSERT(taskdep->successor != NULL);
  taskref successor = {taskdep->successor};
  dart_tasking_datadeps_handle_remote_direct(taskdep->task, successor, taskdep->runit);
}
