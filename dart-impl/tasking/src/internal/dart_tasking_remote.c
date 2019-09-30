
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
#include <time.h>

/**
 * Name of the environment variable specifying whether all message types are
 * to be sent coalesced. By default, dependency releases are sent directly.
 * Accumulating them might help increase bandwidth at the cost of some
 * added latency.
 *
 * Type: boolean
 * Default: false
 */
#define DART_THREAD_COALESCE_RELEASE_ENVSTR  "DART_THREAD_COALESCE_RELEASE"

static dart_amsgq_t amsgq;
//#define DART_RTASK_QLEN 1024*1024
#define DART_RTASK_QLEN 1024

static bool initialized = false;

static uint64_t progress_processing_us   = 0;
static uint64_t progress_sending_us      = 0;
static uint64_t progress_waitprogress_us = 0;
static uint64_t progress_commtasks_us    = 0;
static uint64_t progress_iteration_count = 0;

static bool coalesce_release = false;

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

struct remote_dep_release {
  // reference to the corresponding hash elem on the reporting side
  uintptr_t dep;
};

struct remote_sendrequest {
  dart_gptr_t            src_gptr;
  int32_t                num_bytes;
  dart_taskphase_t       phase;
  int                    tag;
  dart_global_unit_t     unit;
};

struct blocking_progress {
  volatile uint32_t *signal; // <- variable to set once blocking progress completed
};

union remote_operation_u {
  struct remote_data_dep             data_dep;
  struct remote_task_dep             task_dep;
  struct remote_task_dep_cancelation dep_cancel;
  struct remote_task_release         task_release;
  struct remote_dep_release          dep_release;
  struct remote_sendrequest          sendreq;
  struct blocking_progress           progress;
};

typedef
struct remote_operation_t {
  union {
    DART_STACK_MEMBER_DEF;
    struct remote_operation_t *next;
  };
  union remote_operation_u op;
  dart_task_action_t       fn;
  dart_team_unit_t         team_unit;
  uint32_t                 size; // the size of op
  bool                     direct_send;
  bool                     blocking_flush;
} remote_operation_t;

#define DART_OPLIST_ELEM_POP(__list) \
  (remote_operation_t*)((void*)dart__base__stack_pop(&__list))

#define DART_OPLIST_ELEM_POP_NOLOCK(__list) \
  (remote_operation_t*)((void*)dart__base__stack_pop_nolock(&__list))

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

static inline
void do_send(
  bool                      direct_send,
  dart_team_unit_t          team_unit,
  dart_task_action_t        fn,
  void                     *data,
  uint32_t                  size)
{
  while (1) {
    int ret;
    if (direct_send) {
      DART_LOG_TRACE("Sending op %p to unit %d directly", data, team_unit.id);
      ret = dart_amsg_trysend(team_unit, amsgq, fn, data, size);
    } else {
      DART_LOG_TRACE("Sending op %p to unit %d to buffer", data, team_unit.id);
      ret = dart_amsg_buffered_send(team_unit, amsgq, fn, data, size);
    }
    if (ret == DART_OK) {
      // the message was successfully sent
      break;
    } else  if (ret == DART_ERR_AGAIN) {
      // cannot be sent at the moment, just try again
      dart_amsg_process(amsgq);
      continue;
    } else {
      // at this point wen can only abort!
      DART_LOG_ERROR("Failed to send active message to unit %d", team_unit.id);
      dart_abort(DART_EXIT_ABORT);
    }
  }
}

static void
process_operation_list()
{
  remote_operation_t *op;

  if (dart__base__stack_empty(&operation_list)) return;

  // capture the state of the current operation list
  dart_stack_t oplist;
  dart__base__stack_move_to(&operation_list, &oplist);
  remote_operation_t *reverse_list = NULL;

  // reverse the list to get the oldest entry first
  remote_operation_t *flush_op = NULL;
  while (NULL != (op = DART_OPLIST_ELEM_POP_NOLOCK(oplist))) {
    if (op->blocking_flush) {
      flush_op = op;
      continue;
    }
    DART_STACK_PUSH(reverse_list, op);
  }

  // process operations until the list is empty
  while (1) {
    DART_STACK_POP(reverse_list, op);
    if (op == NULL) break;
    do_send(op->direct_send, op->team_unit, op->fn, &op->op, op->size);
    DART_OPLIST_ELEM_PUSH(operation_freelist, op);
  }

  if (flush_op) {
    dart_amsg_flush_buffer(amsgq);
    *flush_op->op.progress.signal = 1;
    DART_OPLIST_ELEM_PUSH(operation_freelist, flush_op);
  }
}

static volatile int progress_thread = 0;
static int handle_comm_tasks = 0;

static void thread_progress_main(void *data)
{
  dart__unused(data);
  int sleep_us = dart__base__env__us(DART_THREAD_PROGRESS_INTERVAL_ENVSTR, 1000);
  struct timespec ts;
  ts.tv_sec  = sleep_us / 1E6;
  ts.tv_nsec = (sleep_us - (ts.tv_sec*1E6))*1E3;

  while (progress_thread) {
    uint64_t ts2;
    uint64_t ts1 = current_time_us();
    // process the message list
    process_operation_list();
    // flush the buffer
    dart_amsg_flush_buffer(amsgq);
    ts2 = current_time_us();
    progress_sending_us += ts2 - ts1;

    // process incoming messages
    dart_amsg_process(amsgq);
    ts1 = current_time_us();
    progress_processing_us += ts1 - ts2;

    // execute communication tasks we have
    if (handle_comm_tasks) {
      dart_task_t *ct;
      while (NULL != (ct = dart_tasking_taskqueue_pop(&comm_tasks))) {
        // mark as inlined, no need to allocate a context
        DART_TASK_SET_FLAG(ct, DART_TASK_IS_INLINED);
        dart__tasking__handle_task(ct);
      }
    }
    ts2 = current_time_us();
    progress_sending_us += ts2 - ts1;

    // progress blocked tasks' communication
    dart__task__wait_progress();
    ts1 = current_time_us();
    progress_waitprogress_us += ts1 - ts2;

    if (sleep_us > 0) {
      nanosleep(&ts, NULL);
    }
    ++progress_iteration_count;
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

    if (progress_thread) {
      dart__tasking__utility_thread(&thread_progress_main, NULL);
      dart_tasking_taskqueue_init(&comm_tasks);
      handle_comm_tasks = dart__base__env__us(
                                  DART_THREAD_PROGRESS_COMMTASKS_ENVSTR, false);
    }

    coalesce_release = dart__base__env__bool(DART_THREAD_COALESCE_RELEASE_ENVSTR,
                                             false);
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

void dart_tasking_remote_print_stats()
{
  if (progress_thread) {
    DART_LOG_INFO_ALWAYS("Progress thread: sending            %lu us",
                        progress_sending_us);
    DART_LOG_INFO_ALWAYS("Progress thread: processsing        %lu us",
                        progress_processing_us);
    DART_LOG_INFO_ALWAYS("Progress thread: waitprogress       %lu us",
                        progress_waitprogress_us);
    DART_LOG_INFO_ALWAYS("Progress thread: commtasks          %lu us",
                        progress_commtasks_us);
    DART_LOG_INFO_ALWAYS("Progress thread: iterations         %lu",
                        progress_iteration_count);
  }
}

/**
 * Send a remote data dependency request for dependency \c dep of
 * the local \c task
 */
dart_ret_t dart_tasking_remote_datadep(
  dart_task_dep_t   *dep,
  dart_global_unit_t guid,
  dart_task_t       *task)
{
  struct remote_data_dep rdep;
  rdep.gptr        = dep->gptr;
  rdep.rtask.local = task;
  rdep.phase       = dep->phase;
  rdep.type        = dep->type;
  dart_myid(&rdep.runit);

  dart_team_unit_t team_unit = DART_TEAM_UNIT_ID(guid.id);

  DART_ASSERT(task != NULL);

  if (progress_thread) {
    remote_operation_t *op = allocate_op();
    op->fn            = &enqueue_from_remote;
    op->size          = sizeof(rdep);
    op->team_unit     = team_unit;
    op->op.data_dep   = rdep;
    op->direct_send   = false;
    op->blocking_flush = false;
    DART_OPLIST_ELEM_PUSH(operation_list, op);
    DART_LOG_TRACE("Enqueued remote dependency request to unit %d "
                   "(segid=%i, offset=%p, fn=%p, task=%p), op=%p",
                   team_unit.id, dep->gptr.segid,
                   dep->gptr.addr_or_offs.addr,
                   &release_remote_dependency, task, op);
    return DART_OK;
  }

  do_send(false, team_unit, &enqueue_from_remote, &rdep, sizeof(rdep));
  DART_LOG_TRACE("Sent remote dependency request to unit t:%i "
      "(segid=%i, offset=%p, fn=%p, task=%p)",
      team_unit.id, dep->gptr.segid,
      dep->gptr.addr_or_offs.addr,
      &release_remote_dependency, task);
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
  team_unit.id = unit.id;

  DART_ASSERT(rtask.remote != NULL);

  bool direct_send = !coalesce_release;

  if (progress_thread) {
    remote_operation_t *op = allocate_op();
    op->fn                 = &release_remote_task;
    op->size               = sizeof(response);
    op->team_unit          = team_unit;
    op->op.task_release    = response;
    op->direct_send        = direct_send;
    op->blocking_flush     = false;
    DART_OPLIST_ELEM_PUSH(operation_list, op);
    DART_LOG_TRACE("Enqueued remote task release to unit %i "
        "(fn=%p, rtask=%p, depref=%p), op %p",
        team_unit.id, &release_remote_task, rtask.local, (void*)depref, op);
    return DART_OK;
  }

  do_send(direct_send, team_unit, &release_remote_task, &response, sizeof(response));
  DART_LOG_TRACE("Sent remote task release to unit %i "
      "(fn=%p, rtask=%p, depref=%p)",
      team_unit.id, &release_remote_task, rtask.local, (void*)depref);

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
  struct remote_dep_release response;
  response.dep = depref;
  dart_team_unit_t team_unit;
  team_unit.id = unit.id;

  bool direct_send = !coalesce_release;

  if (progress_thread) {
    remote_operation_t *op = allocate_op();
    op->fn             = &release_remote_dependency;
    op->size           = sizeof(response);
    op->team_unit      = team_unit;
    op->op.dep_release = response;
    op->direct_send    = direct_send;
    op->blocking_flush = false;
    DART_OPLIST_ELEM_PUSH(operation_list, op);
    DART_LOG_TRACE("Enqueued remote dependency release to unit %i "
                   "(fn=%p, task=%p, depref=%p), op %p",
                   team_unit.id,
                   &release_remote_dependency, rtask.local, (void*)depref, op);
    return DART_OK;
  }

  do_send(direct_send, team_unit, &release_remote_dependency, &response, sizeof(response));
  DART_LOG_TRACE("Sent remote dependency release to unit %i "
                 "(fn=%p, task=%p, depref=%p)",
                 team_unit.id,
                 &release_remote_dependency, rtask.local, (void*)depref);

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
    op->direct_send   = false;
    op->blocking_flush = false;
    DART_OPLIST_ELEM_PUSH(operation_list, op);
    return DART_OK;
  }

  do_send(false, DART_TEAM_UNIT_ID(unit.id), &request_send,
          &request, sizeof(request));

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
  if (progress_thread) {
    volatile uint32_t signal = 0;
    // signal the progress thread that we are waiting for outgoing messages to be flushed
    remote_operation_t *op = allocate_op();
    op->blocking_flush     = true;
    op->direct_send        = false;
    op->op.progress.signal = &signal;
    DART_OPLIST_ELEM_PUSH(operation_list, op);
    // wait for the progress thread to signal completion
    while (!signal) {
      // try again after a short pause
      struct timespec ts = {.tv_nsec = 1, .tv_sec = 0};
      nanosleep(&ts, NULL);
    }
  } else {
    // make sure all operations in-flight have been passed to the message queue
    process_operation_list();
  }
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
  struct remote_dep_release *response = (struct remote_dep_release *)data;
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
