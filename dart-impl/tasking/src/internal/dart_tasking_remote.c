
#include <dash/dart/base/logging.h>
#include <dash/dart/base/atomic.h>
#include <dash/dart/base/assert.h>
#include <dash/dart/if/dart_tasking.h>
#include <dash/dart/if/dart_active_messages.h>
#include <dash/dart/if/dart_team_group.h>
#include <dash/dart/tasking/dart_tasking_remote.h>
#include <dash/dart/tasking/dart_tasking_priv.h>
#include <dash/dart/tasking/dart_tasking_taskqueue.h>
#include <dash/dart/tasking/dart_tasking_cancellation.h>

#include <pthread.h>
#include <stdbool.h>


static dart_amsgq_t amsgq;
#define DART_RTASK_QLEN 1024

static bool initialized = false;


struct remote_data_dep {
  /** Global pointer to the data \c rtask depends on */
  dart_gptr_t        gptr;
  /** The remote (origin) unit ID */
  dart_global_unit_t runit;
  /** pointer to a task on the origin unit. Only valid at the origin! */
  taskref            rtask;
  uint64_t           magic;
  dart_taskphase_t   phase;
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


struct remote_task_dep_cancelation {
  // local task at the target
  taskref task;
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
static void
request_cancellation(void *data);

static inline
size_t
max_size(size_t lhs, size_t rhs)
{
  return (lhs > rhs) ? lhs : rhs;
}


dart_ret_t dart_tasking_remote_init()
{
  if (!initialized) {
    DART_ASSERT_RETURNS(
      dart_amsg_openq(
        max_size(sizeof(struct remote_data_dep),
                 sizeof(struct remote_task_dep)),
        DART_RTASK_QLEN, DART_TEAM_ALL, &amsgq),
      DART_OK);
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
 * Send a remote data dependency request for dependency \c dep of
 * the local \c task
 */
dart_ret_t dart_tasking_remote_datadep(dart_task_dep_t *dep, dart_task_t *task)
{
  dart_ret_t ret;
  struct remote_data_dep rdep;
  rdep.gptr        = dep->gptr;
  rdep.rtask.local = task;
  rdep.magic       = 0xDEADBEEF;
  rdep.phase       = dep->phase;
  dart_myid(&rdep.runit);
  // the amsgq is opened on DART_TEAM_ALL and deps container global IDs
  dart_team_unit_t team_unit = DART_TEAM_UNIT_ID(dep->gptr.unitid);

  DART_ASSERT(task != NULL);

  while (1) {
    ret = dart_amsg_trysend(
            team_unit,
            amsgq,
            &enqueue_from_remote,
            &rdep,
            sizeof(rdep));
    if (ret == DART_OK) {
      // the message was successfully sent
      break;
    } else  if (ret == DART_ERR_AGAIN) {
      // cannot be sent at the moment, just try again
      dart_amsg_process(amsgq);
      continue;
    } else {
      DART_LOG_ERROR(
          "Failed to send active message to unit %i", dep->gptr.unitid);
      return DART_ERR_OTHER;
    }
  }
  return DART_OK;
}

/**
 * Send a release for the remote task \c rtask to \c unit,
 * potentially enqueuing rtask into the
 * runnable list on the remote side.
 */
dart_ret_t dart_tasking_remote_release(
  dart_global_unit_t      unit,
  taskref                 rtask,
  const dart_task_dep_t * dep)
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
    ret = dart_amsg_trysend(
        team_unit,
        amsgq,
        &release_remote_dependency,
        &response,
        sizeof(response));
    if (ret == DART_OK) {
      // the message was successfully sent
      DART_LOG_INFO("Sent remote dependency release to unit t:%i "
          "(segid=%i, offset=%p, fn=%p, rtask=%p)",
          team_unit.id, response.gptr.segid,
          response.gptr.addr_or_offs.offset,
          &release_remote_dependency, rtask.local);
      break;
    } else  if (ret == DART_ERR_AGAIN) {
      // cannot be sent at the moment, just try again
      dart_amsg_process(amsgq);
      continue;
    } else {
      DART_LOG_ERROR("Failed to send active message to unit %i", unit.id);
      return DART_ERR_OTHER;
    }
  }

  return DART_OK;
}


/**
 * Send a direct task dependency request to \c unit to make sure
 * that \c local_task is only executed after \c remote_task has
 * finished and sent a release.
 */
dart_ret_t dart_tasking_remote_direct_taskdep(
  dart_global_unit_t   unit,
  dart_task_t        * local_task,
  taskref              remote_task)
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
    ret = dart_amsg_trysend(
        team_unit,
        amsgq,
        &request_direct_taskdep,
        &taskdep,
        sizeof(taskdep));
    if (ret == DART_OK) {
      // the message was successfully sent
      DART_LOG_INFO("Sent direct remote task dependency to unit %i "
                    "(local task %p depdends on remote task %p)",
                    unit.id, local_task, remote_task.local);
      break;
    } else  if (ret == DART_ERR_AGAIN) {
      // cannot be sent at the moment, just try again
      dart_amsg_process(amsgq);
      continue;
    } else {
      DART_LOG_ERROR("Failed to send active message to unit %i", unit.id);
      return DART_ERR_OTHER;
    }
  }

  return DART_OK;
}


dart_ret_t dart_tasking_remote_bcast_cancel(dart_team_t team)
{
  DART_LOG_DEBUG("Broadcasting cancellation request across team %d", team);
  return dart_amsg_bcast(team, amsgq, &request_cancellation, NULL, 0);

}

/**
 * Check for new remote task dependency requests coming in.
 */
dart_ret_t dart_tasking_remote_progress()
{
  return dart_amsg_process(amsgq);
}

/**
 * Check for new remote task dependency requests coming in.
 * This is similar to \ref dart_tasking_remote_progress
 * but blocks if another process is currently processing the
 * message queue. The call will block until no further incoming
 * messages are received.
 */
dart_ret_t dart_tasking_remote_progress_blocking(dart_team_t team)
{
  return dart_amsg_process_blocking(amsgq, team);
}


/***********************************************
 * Remote tasking actions called on the target *
 ***********************************************/


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
  dart_task_dep_t dep;
  dep.gptr      = dart_tasking_datadeps_localize_gptr(rdep->gptr);
  dep.type      = DART_DEP_IN;
  dep.phase     = rdep->phase;
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
                "release from unit %i for task %p (segid=%i, offset=%p)",
                response->runit.id, task, response->gptr.segid,
                response->gptr.addr_or_offs.offset);

  dart_tasking_datadeps_release_remote_dep(response->rtask.local);
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
  dart_tasking_datadeps_handle_remote_direct(taskdep->task,
                                             successor,
                                             taskdep->runit);
}

/**
 * An action called from the remote unit to signal the cancellation of task
 * processing.
 */
static void
request_cancellation(void *data)
{
  // data is NULL
  (void)data;
  // we cannot call dart_task_cancel() here as it does not return
  // just signal cancellation instead and let the threads detect it
  dart__tasking__cancel_start();
}
