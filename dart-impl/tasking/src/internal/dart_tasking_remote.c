
#include <dash/dart/base/logging.h>
#include <dash/dart/base/atomic.h>
#include <dash/dart/base/assert.h>
#include <dash/dart/base/env.h>
#include <dash/dart/base/stack.h>
#include <dash/dart/if/dart_tasking.h>
#include <dash/dart/if/dart_active_messages.h>
#include <dash/dart/if/dart_team_group.h>
#include <dash/dart/tasking/dart_tasking_remote.h>
#include <dash/dart/tasking/dart_tasking_priv.h>
#include <dash/dart/tasking/dart_tasking_taskqueue.h>
#include <dash/dart/tasking/dart_tasking_cancellation.h>
#include <dash/dart/tasking/dart_tasking_copyin.h>
#include <dash/dart/tasking/dart_tasking_wait.h>

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>


static dart_amsgq_t amsgq;
//#define DART_RTASK_QLEN 1024*1024
#define DART_RTASK_QLEN 1024

static bool initialized = false;


struct remote_data_dep {
  /** Global pointer to the data \c rtask depends on */
  dart_gptr_t         gptr;
  /** pointer to a task on the origin unit. Only valid at the origin! */
  taskref             rtask;
  /** The remote (origin) unit ID */
  dart_global_unit_t  runit;
  dart_taskphase_t    phase;
  dart_task_deptype_t type;
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
};

struct remote_task_dep_cancelation {
  // local task at the target
  taskref task;
};

struct remote_task_release {
  // local task at the target
  taskref task;
  // reference to the corresponding hash elem on the reporting side
  uintptr_t dep;
  // the unit the dependency is located at
  dart_global_unit_t unit;
};

struct remote_sendrequest {
  dart_gptr_t            src_gptr;
  int32_t                num_bytes;
  dart_taskphase_t       phase;
  int                    tag;
  dart_global_unit_t     unit;
};

union remote_operation_u {
  struct remote_data_dep             data_dep;
  struct remote_task_dep             task_dep;
  struct remote_task_dep_cancelation dep_cancel;
  struct remote_task_release         release;
  struct remote_sendrequest          sendreq;
};

enum remote_op_type {
  REMOTE_OP_DATADEP,
  REMOTE_OP_TASKDEP,
  REMOTE_OP_DEPCANCEL,
  REMOTE_OP_RELEASE,
  REMOTE_OP_SENDREQ
};

typedef
struct remote_operation_t {
  DART_STACK_MEMBER_DEF;
  union remote_operation_u op;
  dart_task_action_t       fn;
  dart_team_unit_t         team_unit;
  uint32_t                 size; // the size of op
} remote_operation_t;

#define DART_OPLIST_ELEM_POP(__list) \
  (remote_operation_t*)((void*)dart__base__stack_pop(&__list))

#define DART_OPLIST_ELEM_PUSH(__list, __elem) \
  dart__base__stack_push(&__list, &DART_STACK_MEMBER_GET(__elem))

static dart_stack_t operation_freelist = DART_STACK_INITIALIZER;
static dart_stack_t operation_list     = DART_STACK_INITIALIZER;

static dart_taskqueue_t comm_tasks;

/**
 * Forward declarations of remote tasking actions
 */
static void
enqueue_from_remote(void *data);
static void
release_remote_dependency(void *data);
static void
release_remote_task(void *data);
static void
request_direct_taskdep(void *data);
static void
request_cancellation(void *data);
static void
request_send(void *data);

static
remote_operation_t* allocate_op()
{
  remote_operation_t *op = DART_OPLIST_ELEM_POP(operation_freelist);
  if (op == NULL) {
    op = malloc(sizeof(*op));
  }
  return op;
}

static void
process_operation_list()
{
  remote_operation_t *op;
  // read operations until the list is empty
  while (NULL != (op = DART_OPLIST_ELEM_POP(operation_list))) {
    while (1) {
      int ret;
      DART_LOG_TRACE("Sending op %p to unit %d to buffer", op, op->team_unit.id);
      ret = dart_amsg_buffered_send(
              op->team_unit,
              amsgq,
              op->fn,
              &op->op,
              op->size);
      if (ret == DART_OK) {
        // the message was successfully sent
        break;
      } else  if (ret == DART_ERR_AGAIN) {
        // cannot be sent at the moment, just try again
        dart_amsg_process(amsgq);
        continue;
      } else {
        // at this point wen can only abort!
        DART_ASSERT_MSG(ret != DART_ERR_AGAIN,
                        "Failed to send active message to unit %i",
                        op->team_unit.id);
      }
    }
    DART_OPLIST_ELEM_PUSH(operation_freelist, op);
  }
}

static volatile int progress_thread = 0;
static int handle_comm_tasks = 0;

static void thread_progress_main(void *data)
{
  printf("Progress thread starting up...\n");
  dart__unused(data);
  int sleep_us = dart__base__env__us(DART_THREAD_PROGRESS_INTERVAL_ENVSTR, 1000);
  struct timespec ts;
  ts.tv_sec  = sleep_us / 1E6;
  ts.tv_nsec = (sleep_us - (ts.tv_sec*1E6))*1E3;

  printf("Progress thread starting up (sleep_us=%d)\n", sleep_us);

  while (progress_thread) {
    //printf("Remote progress thread polling for new messages\n");
    // process the message list
    process_operation_list();
    // flush the buffer
    dart_amsg_flush_buffer(amsgq);
    // put the buffer on the wire
    dart_amsg_process(amsgq);

    // execute communication tasks we have
    dart_task_t *ct;
    while (NULL != (ct = dart_tasking_taskqueue_pop(&comm_tasks))) {
      // mark as inlined, no need to allocate a context
      DART_TASK_SET_FLAG(ct, DART_TASK_IS_INLINED);
      dart__tasking__handle_task(ct);
    }

    // progress blocked tasks' communication
    dart__task__wait_progress();

    if (sleep_us > 0) {
      nanosleep(&ts, NULL);
    }
  }
  progress_thread = -1;
}

dart_ret_t dart_tasking_remote_init()
{
  if (!initialized) {
    size_t size = sizeof(union remote_operation_u);
    DART_ASSERT_RETURNS(
      dart_amsg_openq(
        size, DART_RTASK_QLEN, DART_TEAM_ALL, &amsgq),
      DART_OK);
    DART_ASSERT(amsgq != NULL);
    DART_LOG_INFO("Created active message queue for remote tasking (%p)", amsgq);
    progress_thread = dart__base__env__us(DART_THREAD_PROGRESS_ENVSTR, false);
    printf("progress_thread=%d\n", progress_thread);
    if (progress_thread) {
      dart__tasking__utility_thread(&thread_progress_main, NULL);
      dart_tasking_taskqueue_init(&comm_tasks);
      handle_comm_tasks = dart__base__env__us(
                                  DART_THREAD_PROGRESS_COMMTASKS_ENVSTR, false);
    }
    initialized = true;
  }
  return DART_OK;
}

dart_ret_t dart_tasking_remote_fini()
{
  if (initialized)
  {
    // quit progress thread and wait for it to shut down
    if (progress_thread) {
      progress_thread = 0;
      while (progress_thread != -1) {}
    }
    dart_amsg_closeq(amsgq);
    initialized = false;

    if (progress_thread) {
      dart_tasking_taskqueue_finalize(&comm_tasks);
      remote_operation_t *op;
      while (NULL != (op = DART_OPLIST_ELEM_POP(operation_freelist))) {
        free(op);
      }
    }
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
  rdep.phase       = dep->phase;
  rdep.type        = dep->type;
  dart_myid(&rdep.runit);
  // the amsgq is opened on DART_TEAM_ALL and deps container global IDs
  dart_team_unit_t team_unit = DART_TEAM_UNIT_ID(dep->gptr.unitid);

  DART_ASSERT(task != NULL);

  if (progress_thread) {
    remote_operation_t *op = allocate_op();
    op->fn            = &enqueue_from_remote;
    op->size          = sizeof(rdep);
    op->team_unit     = team_unit;
    op->op.data_dep   = rdep;
    DART_OPLIST_ELEM_PUSH(operation_list, op);
    DART_LOG_TRACE("Enqueued remote dependency request to unit %d "
                   "(segid=%i, offset=%p, fn=%p, task=%p), op=%p",
                   team_unit.id, dep->gptr.segid,
                   dep->gptr.addr_or_offs.addr,
                   &release_remote_dependency, task, op);
    return DART_OK;
  }

  while (1) {
    ret = dart_amsg_buffered_send(
            team_unit,
            amsgq,
            &enqueue_from_remote,
            &rdep,
            sizeof(rdep));
    if (ret == DART_OK) {
      // the message was successfully sent
      DART_LOG_TRACE("Sent remote dependency request to unit t:%i "
          "(segid=%i, offset=%p, fn=%p, task=%p)",
          team_unit.id, dep->gptr.segid,
          dep->gptr.addr_or_offs.addr,
          &release_remote_dependency, task);
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
 * runnable list on the remote side. We also send the dependency reference
 * that is later used to report completion.
 */
dart_ret_t dart_tasking_remote_release_task(
  dart_global_unit_t  unit,
  taskref             rtask,
  uintptr_t           depref)
{
  struct remote_task_release response;
  response.task = rtask;
  response.dep  = depref;
  dart_myid(&response.unit);
  dart_team_unit_t team_unit;
  dart_team_unit_g2l(DART_TEAM_ALL, unit, &team_unit);

  DART_ASSERT(rtask.remote != NULL);

  if (progress_thread) {
    remote_operation_t *op = allocate_op();
    op->fn            = &release_remote_task;
    op->size          = sizeof(response);
    op->team_unit     = team_unit;
    op->op.release    = response;
    DART_OPLIST_ELEM_PUSH(operation_list, op);
    DART_LOG_TRACE("Enqueued remote task release to unit %i "
        "(fn=%p, rtask=%p, depref=%p), op %p",
        team_unit.id, &release_remote_task, rtask.local, (void*)depref, op);
    return DART_OK;
  }

  while (1) {
    dart_ret_t ret;
    ret = dart_amsg_trysend(
            team_unit,
            amsgq,
            &release_remote_task,
            &response,
            sizeof(response));
    if (ret == DART_OK) {
      // the message was successfully sent
      DART_LOG_TRACE("Sent remote task release to unit %i "
          "(fn=%p, rtask=%p, depref=%p)",
          team_unit.id, &release_remote_task, rtask.local, (void*)depref);
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
 * Send a release for the remote dependency to the unit managing it.
 * This signals that the task is completed and the dependency of this task can
 * be released.
 */
dart_ret_t dart_tasking_remote_release_dep(
  dart_global_unit_t      unit,
  taskref                 rtask,
  uintptr_t               depref)
{
  struct remote_task_release response;
  response.task.local = NULL;
  response.dep        = depref;
  response.unit.id    = -1;
  dart_team_unit_t team_unit;
  dart_team_unit_g2l(DART_TEAM_ALL, unit, &team_unit);

  if (progress_thread) {
    remote_operation_t *op = allocate_op();
    op->fn            = &release_remote_dependency;
    op->size          = sizeof(response);
    op->team_unit     = team_unit;
    op->op.release    = response;
    DART_OPLIST_ELEM_PUSH(operation_list, op);
    DART_LOG_TRACE("Enqueued remote dependency release to unit %i "
                   "(fn=%p, task=%p, depref=%p), op %p",
                   team_unit.id,
                   &release_remote_dependency, rtask.local, (void*)depref, op);
    return DART_OK;
  }

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
      DART_LOG_TRACE("Sent remote dependency release to unit %i "
                     "(fn=%p, task=%p, depref=%p)",
                     team_unit.id,
                     &release_remote_dependency, rtask.local, (void*)depref);
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


dart_ret_t dart_tasking_remote_sendrequest(
  dart_global_unit_t     unit,
  dart_gptr_t            src_gptr,
  size_t                 num_bytes,
  int                    tag,
  dart_taskphase_t       phase)
{
  DART_ASSERT(num_bytes <= INT_MAX);

  struct remote_sendrequest request;
  dart_myid(&request.unit);
  request.num_bytes = num_bytes;
  request.src_gptr  = src_gptr;
  request.tag       = tag;
  request.phase     = phase;

  if (progress_thread) {
    remote_operation_t *op = allocate_op();
    op->fn            = &request_send;
    op->size          = sizeof(request);
    op->team_unit     = DART_TEAM_UNIT_ID(unit.id);
    op->op.sendreq    = request;
    DART_OPLIST_ELEM_PUSH(operation_list, op);
    return DART_OK;
  }

  while (1) {
    int ret;
    ret = dart_amsg_buffered_send(DART_TEAM_UNIT_ID(unit.id), amsgq,
                                  &request_send, &request, sizeof(request));
    if (ret == DART_OK) {
      // the message was successfully sent
      break;
    } else  if (ret == DART_ERR_AGAIN) {
      // cannot be sent at the moment, just try again
      dart_amsg_process(amsgq);
      continue;
    } else {
      DART_LOG_ERROR(
          "Failed to send active message to unit %i", unit.id);
      return DART_ERR_OTHER;
    }
  }
  return DART_OK;
}

dart_ret_t dart_tasking_remote_bcast_cancel(dart_team_t team)
{
  DART_LOG_DEBUG("Broadcasting cancellation request across team %d", team);
  dart_amsg_bcast(team, amsgq, &request_cancellation, &team, sizeof(team));
  // wait for all units to receive the cancellation signal
  dart_barrier(team);
}

/**
 * Check for new remote task dependency requests coming in.
 */
dart_ret_t dart_tasking_remote_progress()
{
  if (!progress_thread) {
    dart__task__wait_progress();
    return dart_amsg_process(amsgq);
  }
  return DART_OK;
}

/**
 * Check for new remote task dependency requests coming in.
 * This is similar to \ref dart_tasking_remote_progress
 * but blocks if another thread is currently processing the
 * message queue. The call will block until no further incoming
 * messages are received.
 */
dart_ret_t dart_tasking_remote_progress_blocking(dart_team_t team)
{
  // make sure all operations in-flight have been passed to the message queue
  process_operation_list();
  return dart_amsg_process_blocking(amsgq, team);
}

void dart_tasking_remote_handle_comm_task(dart_task_t *task, bool *enqueued)
{
  if (progress_thread && handle_comm_tasks) {
    dart_tasking_taskqueue_push(&comm_tasks, task);
    *enqueued = true;
  } else {
    *enqueued = false;
  }
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
  DART_ASSERT(rdep->rtask.remote != NULL);
  dart_task_dep_t dep;
  dep.gptr      = dart_tasking_datadeps_localize_gptr(rdep->gptr);
  dep.type      = rdep->type;
  dep.phase     = rdep->phase;
  taskref rtask = rdep->rtask;
  DART_LOG_TRACE("Received remote dependency request for task %p "
                "(unit=%i, segid=%i, addr=%p, ph=%i, type=%d)",
                rdep->rtask.remote, rdep->runit.id, rdep->gptr.segid,
                rdep->gptr.addr_or_offs.addr, dep.phase, rdep->type);
  dart_tasking_datadeps_handle_remote_task(&dep, rtask, rdep->runit);
}

/**
 * An action called from the remote unit to signal the release of a remote dependency.
 * The remote unit sends back a pointer to the local task object so we can easily
 * decrement the dependency counter and enqueue the task if possible.
 */
static void release_remote_dependency(void *data)
{
  struct remote_task_release *response = (struct remote_task_release *)data;
  DART_ASSERT(response->task.local == NULL);
  DART_LOG_TRACE("release_remote_dependency : Received remote dependency "
                 "release for dependency object %p",
                 (dart_dephash_elem_t*)response->dep);

  dart_tasking_datadeps_release_remote_dep((dart_dephash_elem_t*)response->dep);
}

/**
 * An action called from the remote unit to signal the release of a local task.
 * The remote unit sends back a pointer to the local task object so we can easily
 * decrement the dependency counter and enqueue the task if possible.
 */
static void release_remote_task(void *data)
{
  struct remote_task_release *response = (struct remote_task_release *)data;
  DART_ASSERT(response->task.local != NULL);
  dart_task_t *task = response->task.local;
  DART_LOG_TRACE("release_remote_dependency : Received remote dependency "
                "release for task %p", task);

  dart_tasking_datadeps_release_remote_task(
    response->task.local, response->dep, response->unit);
}

static void
request_send(void *data)
{
  struct remote_sendrequest *request = (struct remote_sendrequest *) data;

  dart_tasking_copyin_sendrequest(
    request->src_gptr, request->num_bytes,
    request->phase,    request->tag,   request->unit);
}

/**
 * An action called from the remote unit to signal the cancellation of task
 * processing.
 */
static void
request_cancellation(void *data)
{
  dart_team_t team = *(dart_team_t*)data;
  // wait for all units to receive the cancellation signal
  dart_barrier(team);
  // we cannot call dart_task_cancel() here as it does not return
  // just signal cancellation instead and let the threads detect it
  dart__tasking__cancel_start();
}
