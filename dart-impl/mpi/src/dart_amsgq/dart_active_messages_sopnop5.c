
#include <stdlib.h>
#include <unistd.h>
#include <mpi.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <alloca.h>
#include <time.h>

#include <dash/dart/base/mutex.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/base/assert.h>
#include <dash/dart/base/env.h>
#include <dash/dart/base/atomic.h>
#include <dash/dart/if/dart_active_messages.h>
#include <dash/dart/if/dart_communication.h>
#include <dash/dart/if/dart_globmem.h>
#include <dash/dart/mpi/dart_team_private.h>
#include <dash/dart/mpi/dart_globmem_priv.h>
#include <dash/dart/mpi/dart_active_messages_priv.h>


#ifdef DART_HAVE_MPI_EGREQ


#ifdef OMPI_MAJOR_VERSION
// forward declaration, not needed once we figure out how this works in OMPI
typedef int (ompi_grequestx_poll_function)(void *, MPI_Status *);
int ompi_grequestx_start(
    MPI_Grequest_query_function *gquery_fn,
    MPI_Grequest_free_function *gfree_fn,
    MPI_Grequest_cancel_function *gcancel_fn,
    ompi_grequestx_poll_function *gpoll_fn,
    void* extra_state,
    MPI_Request* request);
#define MPIX_Grequest_start(query_fn,free_fn,cancel_fn,poll_fn,extra_state,request) \
  ompi_grequestx_start(query_fn,free_fn,cancel_fn,poll_fn,extra_state,request)

#elif defined (MPICH_VERSION)
#define MPIX_Grequest_start(query_fn,free_fn,cancel_fn,poll_fn,extra_state,request) \
  MPIX_Grequest_start(query_fn,free_fn,cancel_fn,poll_fn,NULL/*MPICH has an extra wait function*/,extra_state,request)
#endif


#define DART_STACK_PUSH(_head, _elem) \
  DART_STACK_PUSH_MEMB(_head, _elem, next)

#define DART_STACK_POP(_head, _elem) \
  DART_STACK_POP_MEMB(_head, _elem, next)

#define DART_STACK_PUSH_MEMB(_head, _elem, _memb) \
  do {                                \
    (_elem)->_memb = (_head);           \
    (_head) = (_elem);                    \
  } while (0)

#define DART_STACK_POP_MEMB(_head, _elem, _memb) \
  do {                               \
    _elem = _head;                   \
    if (_elem != NULL) {             \
      _head = (_elem)->_memb;        \
      (_elem)->_memb = NULL;         \
    }                                \
  } while (0)


/* Setting the upper-most bit on the reader count signals processing */
#define PROCESSING_SIGNAL (-((int32_t)1<<30))

/**
 * Name of the environment variable specifying the number microseconds caller
 * sleeps between consecutive reads of the active message in a blocking
 * processing call.
 *
 * Type: integral value with optional us, ms, s qualifier
 */
#define DART_AMSGQ_SOPNOP_SLEEP_ENVSTR  "DART_AMSGQ_SOPNOP_SLEEP"

/**
 * Name of the environment variable specifying whether a flush is performed
 * at the end of a write operation. Avoiding thus flush can reduce latency
 * but *may* lead to deadlocks due to the weak guarantees MPI gives us.
 *
 * Type: boolean
 * Default: true
 */
#define DART_AMSGQ_SOPNOP_FLUSH_ENVSTR  "DART_AMSGQ_SOPNOP_FLUSH"

/**
 * Name of the environment variable controling whether single-value
 * single-direction updates should be performed using MPI_Fetch_and_op or
 * MPI_Accumulate.
 *
 * Type: boolean
 * Default: false
 */
#define DART_AMSGQ_SOPNOP_FETCHOP_ENVSTR  "DART_AMSGQ_SOPNOP_FETCHOP"

struct dart_amsgq_impl_data {
  MPI_Win           queue_win;
  int64_t          *queue_ptr;
  uint64_t          queue_size;
  MPI_Comm          comm;
  dart_mutex_t      send_mutex;
  dart_mutex_t      processing_mutex;
  struct dart_grequest_state *states;
  MPI_Request      *reqs;
  int              *outidx;
  int               comm_rank;
  int               prev_tailpos;
  bool              needs_flush;
};

enum {
  DART_GREQUEST_FAILED = 0, // reserved for a failed attempt
  DART_GREQUEST_START = 1,
  DART_GREQUEST_RETRY,
  DART_GREQUEST_QUEUENUM,
  DART_GREQUEST_REGISTER,
  DART_GREQUEST_OFFSET,
  DART_GREQUEST_PUT,
  DART_GREQUEST_COMPLETE,
};

struct dart_grequest_state {
  struct dart_grequest_state *next;     // linked list pointer
  uint64_t offset;                      // the offset at which to write
  uint64_t fused_val;                   // values that holds fused reqistration/msgsize
  uint64_t fused_op;                    // values that holds fused reqistration/msgsize
  uint64_t queuenum;                    // the queue into which to write
  int      state;                       // where in the process we are
  int      idx;                         // index in the allocated array
  struct dart_flush_info * flush_info;  // the flush information
  struct dart_amsgq_impl_data * amsgq;  // the amsgq to use
  MPI_Request req;                      // request to store the grequest in
  MPI_Request opreq;                    // request that can be used by operations
};

#ifdef DART_ENABLE_LOGGING
static uint32_t msgcnt = 0;
#endif // DART_ENABLE_LOGGING

static struct timespec sleeptime = {-1, -1};
static int64_t sleep_us = -1;

static const int64_t one  = 1;
static const int64_t mone = -1;
static       int64_t tmp  = -1;
static       uint64_t dereg_value;

static bool do_flush = true;
static bool use_fetchop = false;

static int pipeline_depth = 4;

#define NUM_QUEUES                      2
#define OFFSET_QUEUENUM                 0
#define OFFSET_WRITECNT(q)   (OFFSET_QUEUENUM+(q+1)*sizeof(uint64_t))
#define OFFSET_DATA(q, qs)   ((NUM_QUEUES+1)*sizeof(uint64_t)+q*qs)

/**
 * Bit manipulation functions used to pack and unpack 2 32-bit integers into
 * a 64-bit integer to use with 64-bit arithmetic operations (+ and - only).
 */

static inline
int set_bit32(int value, int pos)
{
  return (value | 1 << pos);
}

static inline
uint64_t set_bit64(uint64_t value, int pos)
{
  return (value | ((uint64_t)1) << pos);
}

#define set_bit(_val, _pos) _Generic((_val),                           \
                                      int: set_bit32(_val, _pos),      \
                                      uint32_t: set_bit32(_val, _pos), \
                                      uint64_t: set_bit64(_val, _pos))

static inline
uint32_t unset_bit32(uint32_t value, int pos)
{
  return (value & ~(1 << pos));
}

static inline
uint64_t unset_bit64(uint64_t value, int pos)
{
  return (value & ~(((uint64_t)1) << pos));
}

#define unset_bit(_val, _pos) _Generic((_val),                           \
                                      int: unset_bit32(_val, _pos),      \
                                      uint32_t: unset_bit32(_val, _pos), \
                                      uint64_t: unset_bit64(_val, _pos))


static inline
uint64_t fuse(int a, int b)
{
  uint64_t aa = a;
  if (b > 0) {
    uint64_t bb = b;
    return (aa << 32 | bb);
  } else {
    return (aa << 32) + b;
  }
}

static inline
int first(uint64_t a)
{
  int aa = a >> 32;
  return aa;
}

static inline
int second(uint64_t a)
{
  int bb = a & (((uint64_t)1<<32) - 1);
  return bb;
}

static inline
void update_value(const uint64_t *val, int target, int offset, MPI_Win win)
{
  if (use_fetchop) {
    // tmp is a dummy, not relevant
    MPI_Fetch_and_op(val, &tmp, MPI_INT64_T, target, offset, MPI_SUM, win);
  } else {
    MPI_Accumulate(val, 1, MPI_INT64_T, target, offset,
                   1, MPI_INT64_T, MPI_SUM, win);
  }
}

static dart_ret_t
dart_amsg_sopnop_openq(
  size_t      msg_size,
  size_t      msg_count,
  dart_team_t team,
  struct dart_amsgq_impl_data** queue)
{
  dart_team_data_t *team_data = dart_adapt_teamlist_get(team);
  if (team_data == NULL) {
    DART_LOG_ERROR("dart_gptr_getaddr ! Unknown team %i", team);
    return DART_ERR_INVAL;
  }

  if (sleeptime.tv_sec == -1) {
    sleep_us  = dart__base__env__us(DART_AMSGQ_SOPNOP_SLEEP_ENVSTR, 0);
    sleeptime.tv_sec  = sleep_us / 1000000;
    sleeptime.tv_nsec = sleep_us % 1000000;

    do_flush    = dart__base__env__bool(DART_AMSGQ_SOPNOP_FLUSH_ENVSTR, true);
    use_fetchop = dart__base__env__bool(DART_AMSGQ_SOPNOP_FETCHOP_ENVSTR, false);

    dereg_value = fuse(-1, 0);
  }

  struct dart_amsgq_impl_data *res = calloc(1, sizeof(struct dart_amsgq_impl_data));
  res->queue_size = msg_count * msg_size;
  MPI_Comm_dup(team_data->comm, &res->comm);

  MPI_Comm_rank(res->comm, &res->comm_rank);

  // TODO: check if it is better to use 32-bit integer?!
  size_t win_size = sizeof(int64_t)          // queuenumber (64-bit to guarantee alignment)
                    + NUM_QUEUES*(sizeof(int64_t))    // the writer count and tailpos of each queue (fused into one 64-bit integer)
                    + NUM_QUEUES*(res->queue_size);   // queue double-buffer

  dart__base__mutex_init(&res->send_mutex);
  dart__base__mutex_init(&res->processing_mutex);

  // we don't need MPI to take care of the ordering since we use
  // explicit flushes to guarantee ordering
  MPI_Info info;
  MPI_Info_create(&info);
  MPI_Info_set(info, "accumulate_ordering" , "none");
  MPI_Info_set(info, "same_size"           , "true");
  MPI_Info_set(info, "same_disp_unit"      , "true");
  MPI_Info_set(info, "accumulate_ops"      , "same_op_no_op");
  MPI_Info_set(info, "acc_single_intrinsic", "true");

  /**
   * Allocate the queue
   * We cannot use dart_team_memalloc_aligned because it uses
   * MPI_Win_allocate_shared that cannot be used for window locking.
   */
  MPI_Win_allocate(
    win_size,
    1,
    info,
    res->comm,
    &(res->queue_ptr),
    &(res->queue_win));
  MPI_Info_free(&info);

  memset(res->queue_ptr, 0, win_size);

  res->states = malloc(pipeline_depth * sizeof(struct dart_grequest_state));
  res->reqs   = malloc(pipeline_depth * sizeof(MPI_Request));
  res->outidx = malloc(pipeline_depth * sizeof(*res->outidx));

  // properly initialize the writecnt of the second queue
  *(int64_t*)(((intptr_t)res->queue_ptr) + OFFSET_WRITECNT(1)) = fuse(PROCESSING_SIGNAL, 0);

  MPI_Win_lock_all(0, res->queue_win);

  MPI_Barrier(res->comm);

  DART_LOG_DEBUG("Allocated double-buffered message queue (buffer: %ld)",
                 res->queue_size);

  *queue = res;

  return DART_OK;
}


static
dart_ret_t
dart_amsg_sopnop_sendbuf(
  dart_team_unit_t              target,
  struct dart_amsgq_impl_data * amsgq,
  const void                  * data,
  size_t                        data_size)
{
  // no locks needed, MPI will take care of it

  MPI_Win queue_win = amsgq->queue_win;

  DART_LOG_DEBUG("dart_amsg_trysend: u:%i ds:%zu",
                 target.id, data_size);

  int64_t msg_size = data_size;
  int64_t offset;
  int64_t queuenum;

  do {

    /**
     * Number of accumulate ops: 4 ops
     */

    // fetch queue number
    MPI_Get(&queuenum, 1, MPI_INT64_T, target.id,
             OFFSET_QUEUENUM, 1, MPI_INT64_T, queue_win);
    MPI_Win_flush(target.id, queue_win);

    DART_LOG_TRACE("Writing to queue %ld at %d", queuenum, target.id);

    DART_ASSERT(queuenum == 0 || queuenum == 1);

    uint64_t writecnt_offset = fuse(1, 0);

    // NOTE: we cannot fuse registration+reservation in one (or fenced) operation
    //       because that might create a race condition with the reader
    //       adding back the PROCESSING_SIGNAL

    uint64_t fused_val;
    // register as a writer
    MPI_Fetch_and_op(
      &writecnt_offset,
      &fused_val,
      MPI_UINT64_T,
      target.id,
      OFFSET_WRITECNT(queuenum),
      MPI_SUM,
      queue_win);
    MPI_Win_flush(target.id, queue_win);

    int64_t writecnt = first(fused_val);
    offset = second(fused_val);


    if (writecnt < 0) {
      // queue is processing
      DART_LOG_TRACE("Queue %ld at %d processing, retrying (writecnt %ld)",
                    queuenum, target.id, writecnt);
      update_value(&dereg_value, target.id, OFFSET_WRITECNT(queuenum), queue_win);
      continue;
    }

    DART_LOG_TRACE("Queue %ld at %d: writecnt %ld, offset %ld (fused_val %lu)",
                   queuenum, target.id, writecnt, offset, fused_val);


    // check early if the queue is full
    if (offset < 0 || (offset + msg_size) > amsgq->queue_size) {
      // the queue is full, deregister and come back later
      DART_LOG_TRACE("Queue %ld at %d full (tailpos %ld, writecnt %ld)",
                    queuenum, target.id, offset, writecnt);
      update_value(&dereg_value, target.id, OFFSET_WRITECNT(queuenum), queue_win);
      return DART_ERR_AGAIN;
    }

    writecnt_offset = fuse(0, msg_size);

    // reserve a slot
    MPI_Fetch_and_op(
      &writecnt_offset,
      &fused_val,
      MPI_UINT64_T,
      target.id,
      OFFSET_WRITECNT(queuenum),
      MPI_SUM,
      queue_win);
    MPI_Win_flush(target.id, queue_win);

    offset = second(fused_val);

    // if the message fits we can write it
    if (offset >= 0 && (offset + msg_size) <= amsgq->queue_size) break;

    // the queue is full, reset the offset
    DART_LOG_TRACE("Queue %ld at %d full (tailpos %ld, writecnt %ld)",
                  queuenum, target.id, offset, writecnt);
    writecnt_offset = fuse(-1, -msg_size);
    // deregister as a writer
    update_value(&writecnt_offset, target.id, OFFSET_WRITECNT(queuenum), queue_win);

    MPI_Win_flush_local(target.id, queue_win);

    return DART_ERR_AGAIN;
  } while (1);

  DART_LOG_TRACE("Writing %ld into queue %ld at offset %ld at unit %i",
                 data_size, queuenum, offset, target.id);

  // Write our payload

  DART_LOG_TRACE("MPI_Put into queue %ld offset %ld (%ld)",
                 queuenum, offset, OFFSET_DATA(queuenum, amsgq->queue_size) + offset);
  MPI_Put(
    data,
    data_size,
    MPI_BYTE,
    target.id,
    OFFSET_DATA(queuenum, amsgq->queue_size) + offset,
    data_size,
    MPI_BYTE,
    queue_win);
  // we have to flush here because MPI has no ordering guarantees
  MPI_Win_flush(target.id, queue_win);

  DART_LOG_TRACE("Unregistering as writer from queue %ld at unit %i",
                 queuenum, target.id);

  // deregister as a writer
  update_value(&dereg_value, target.id, OFFSET_WRITECNT(queuenum), queue_win);

  if (do_flush) {
    MPI_Win_flush(target.id, queue_win);
  }

  DART_LOG_TRACE("Sent message of size %zu with payload %zu to unit "
                "%d starting at offset %ld",
                msg_size, data_size, target.id, offset);

  return DART_OK;
}

static
int grequest_query_fn(void *data, MPI_Status *status)
{
  struct dart_grequest_state *state = (struct dart_grequest_state *)data;
  //printf("query_fn: state %d\n", state->state);
}

static void initiate_queuenum_fetch(struct dart_grequest_state *state)
{
  MPI_Rget_accumulate(
    &tmp, 1, MPI_UINT64_T,
    &state->queuenum, 1, MPI_UINT64_T,
    state->flush_info->target, OFFSET_QUEUENUM, 1, MPI_UINT64_T, MPI_NO_OP,
    state->amsgq->queue_win, &state->opreq);
}

static
int grequest_poll_fn(void *data, MPI_Status *status)
{
  struct dart_grequest_state *state = (struct dart_grequest_state *)data;

  //printf("poll_fn: state %d\n", state->state);

  switch (state->state) {
    case DART_GREQUEST_START:
    {
      initiate_queuenum_fetch(state);
      state->state = DART_GREQUEST_QUEUENUM;

      return MPI_SUCCESS;
    }
    case DART_GREQUEST_RETRY:
    {
      int flag;
      MPI_Test(&state->opreq, &flag, MPI_STATUS_IGNORE);
      if (0 == flag) {
        // come back later
        return MPI_SUCCESS;
      }
      int64_t writecnt = first(state->fused_val);

      if (writecnt < 0) {
        initiate_queuenum_fetch(state);
        state->state = DART_GREQUEST_QUEUENUM;
        return MPI_SUCCESS;
      }

      // we skipped one round so don't requery the queue number but try again to register

      state->opreq = MPI_REQUEST_NULL;

      /* fall through */
    }
    case DART_GREQUEST_QUEUENUM:
    {
      if (state->opreq != MPI_REQUEST_NULL) {
        int flag;
        MPI_Test(&state->opreq, &flag, MPI_STATUS_IGNORE);
        if (0 == flag) {
          // come back later
          return MPI_SUCCESS;
        }
      }

      // kick off registration
      state->fused_op = fuse(1, 0);

      // register as a writer
      MPI_Rget_accumulate(
        &state->fused_op,  1, MPI_UINT64_T,
        &state->fused_val, 1, MPI_UINT64_T,
        state->flush_info->target, OFFSET_WRITECNT(state->queuenum),
        1, MPI_UINT64_T, MPI_SUM, state->amsgq->queue_win, &state->opreq);

      // that's it for this iteration
      state->state = DART_GREQUEST_REGISTER;
      return MPI_SUCCESS;
    }
    case DART_GREQUEST_REGISTER:
    {
      int flag;
      MPI_Test(&state->opreq, &flag, MPI_STATUS_IGNORE);
      if (0 == flag) {
        // come back later
        return MPI_SUCCESS;
      }

      int64_t writecnt = first(state->fused_val);

      int64_t queuenum = state->queuenum;
      int target = state->flush_info->target;

      if (writecnt < 0) {
        // queue is processing, go back to start
        DART_LOG_TRACE("Queue %ld at %d processing, retrying (writecnt %ld)",
                      queuenum, target, writecnt);

        // kick off deregistration
        MPI_Rget_accumulate(
          &dereg_value,      1, MPI_UINT64_T,
          &state->fused_val, 1, MPI_UINT64_T,
          target, OFFSET_WRITECNT(queuenum),
          1, MPI_UINT64_T, MPI_SUM, state->amsgq->queue_win, &state->opreq);

        state->state = DART_GREQUEST_RETRY;
        return MPI_SUCCESS;
      }

      int64_t offset = second(state->fused_val);
      if ((offset + state->flush_info->size) > state->amsgq->queue_size) {
        // queue is full, come back later

        // the queue is full, reset the offset
        DART_LOG_TRACE("Queue %ld at %d full (tailpos %ld)",
                      queuenum, target, offset);
        // deregister as a writer
        update_value(&dereg_value, target, OFFSET_WRITECNT(queuenum),
                     state->amsgq->queue_win);

        // we throw our hands up and mark the request as completed
        state->state = DART_GREQUEST_FAILED;
        MPI_Grequest_complete(state->req);
        return MPI_SUCCESS;
      }

      // reserve a slot
      state->fused_op = fuse(0, state->flush_info->size);
      MPI_Rget_accumulate(
        &state->fused_op, 1, MPI_UINT64_T,
        &state->fused_val, 1, MPI_UINT64_T,
        target, OFFSET_WRITECNT(queuenum),
        1, MPI_UINT64_T, MPI_SUM, state->amsgq->queue_win, &state->opreq);

      // that's it for this iteration
      state->state = DART_GREQUEST_OFFSET;
      return MPI_SUCCESS;
    }
    case DART_GREQUEST_OFFSET:
    {
      int flag;
      MPI_Test(&state->opreq, &flag, MPI_STATUS_IGNORE);
      if (0 == flag) {
        // come back later
        return MPI_SUCCESS;
      }

      int target = state->flush_info->target;
      int64_t queuenum = state->queuenum;
      MPI_Win queue_win = state->amsgq->queue_win;

      int64_t offset   = second(state->fused_val);
      int64_t writecnt = first(state->fused_val);
      int64_t msg_size = state->flush_info->size;

      DART_LOG_TRACE("Queue %ld at %d: writecnt %ld, offset %ld (fused_val %lu)",
                     queuenum, target, writecnt,
                     offset, state->fused_val);

      // if the message does not fit we have to wait for the queue to be processed
      if (!(offset >= 0 && (offset + msg_size) <= state->amsgq->queue_size)) {

        // the queue is full, reset the offset
        DART_LOG_TRACE("Queue %ld at %d full (tailpos %ld)",
                      queuenum, target, offset);
        state->fused_op = fuse(-1, -msg_size);
        // deregister as a writer
        update_value(&state->fused_op, target, OFFSET_WRITECNT(queuenum), queue_win);

        state->amsgq->needs_flush = true;

        // we throw our hands up and mark the request as completed
        state->state = DART_GREQUEST_FAILED;
        MPI_Grequest_complete(state->req);
        return MPI_SUCCESS;
      }

      // advance to next step

      DART_LOG_TRACE("Writing %ld into queue %ld at offset %ld at unit %i",
                    msg_size, queuenum, offset, target);

      // Write our payload
      MPI_Put(
        state->flush_info->data,
        msg_size,
        MPI_BYTE,
        target,
        OFFSET_DATA(queuenum, state->amsgq->queue_size) + offset,
        msg_size,
        MPI_BYTE,
        queue_win);

      state->state = DART_GREQUEST_PUT;

      return MPI_SUCCESS;
    }
    case DART_GREQUEST_PUT:
    {
      int target = state->flush_info->target;
      MPI_Win queue_win = state->amsgq->queue_win;
      int64_t queuenum = state->queuenum;
      // we have to flush here because MPI has no ordering guarantees
      MPI_Win_flush(target, queue_win);

      DART_LOG_TRACE("Unregistering as writer from queue %ld at unit %i",
                    queuenum, target);

      // deregister as a writer
      update_value(&dereg_value, target, OFFSET_WRITECNT(queuenum), queue_win);

      DART_LOG_TRACE("Sent message of size %zu to unit "
                    "%d starting at offset %ld",
                    state->flush_info->size, target, second(state->fused_val));

      state->state = DART_GREQUEST_COMPLETE;
      state->flush_info->status = 1;
      // we're done, mark the request as complete and return
      MPI_Grequest_complete(state->req);
      return MPI_SUCCESS;
    }
    default:
      DART_ASSERT_MSG(state->state <= DART_GREQUEST_PUT,
                      "Unexpected state request found in polling function!");
  }

}

static
int grequest_free_fn(void *data)
{
  struct dart_grequest_state *state = (struct dart_grequest_state *)data;
  //printf("free_fn: state %d\n", state->state);
}

static
int grequest_cancel_fn(void *data, int complete)
{
  struct dart_grequest_state *state = (struct dart_grequest_state *)data;
  printf("cancel_fn: state %d\n", state->state);
}

static
dart_ret_t
dart_amsg_sopnop_sendbuf_all(
  struct dart_amsgq_impl_data * amsgq,
  struct dart_flush_info      * flush_info,
  int                           num_info)
{
  dart__base__mutex_lock(&amsgq->send_mutex);


  // put all elements into a lifo
  struct dart_grequest_state *state_lifo = NULL;
  for (int i = 0; i < pipeline_depth; ++i) {
    amsgq->states[i].idx = i;
    DART_STACK_PUSH(state_lifo, &amsgq->states[i]);
  }

  //printf("dart_amsg_sopnop_sendbuf_all\n");

  int num_active = 0;

  for (int i = 0; i < num_info; ++i) {
    struct dart_grequest_state *state = NULL;

    do {
      DART_STACK_POP(state_lifo, state);
      if (NULL != state) break;
      // wait for some operations to complete
      int outcount;
      MPI_Waitsome(num_active, amsgq->reqs, &outcount, amsgq->outidx, MPI_STATUSES_IGNORE);
      num_active -= outcount;
      for (int i = 0; i < outcount; ++i) {
        DART_STACK_PUSH(state_lifo, &amsgq->states[amsgq->outidx[i]]);
      }
    } while (true);

    state->state = DART_GREQUEST_QUEUENUM;
    state->flush_info = &flush_info[i];
    state->amsgq = amsgq;
    state->flush_info->status = 0;

    // fetch queue number
    initiate_queuenum_fetch(state);
    // hand the operation to MPI
    MPIX_Grequest_start(
      grequest_query_fn, grequest_free_fn,
      grequest_cancel_fn, grequest_poll_fn,
      state, &state->req);
    amsgq->reqs[state->idx] = state->req;

    ++num_active;
  }

  MPI_Waitall(num_info, amsgq->reqs, MPI_STATUSES_IGNORE);

  if (do_flush || amsgq->needs_flush) {
    MPI_Win_flush_all(amsgq->queue_win);
    amsgq->needs_flush = false;
  }

  dart__base__mutex_unlock(&amsgq->send_mutex);

  return DART_OK;
}


static dart_ret_t
amsg_sopnop_process_internal(
  struct dart_amsgq_impl_data * amsgq,
  bool                          blocking)
{
  int64_t tailpos;
  int comm_rank = amsgq->comm_rank;
  MPI_Win queue_win = amsgq->queue_win;

  if (!blocking) {
    dart_ret_t ret = dart__base__mutex_trylock(&amsgq->processing_mutex);
    if (ret != DART_OK) {
      return DART_ERR_AGAIN;
    }
  } else {
    dart__base__mutex_lock(&amsgq->processing_mutex);
  }

  do {

    int64_t queuenum = *(int64_t*)amsgq->queue_ptr;

    DART_ASSERT(queuenum == 0 || queuenum == 1);

    /**
     * Number of accumulate ops: 5 ops
     */

    // see whether there is anything available
    /*
    MPI_Fetch_and_op(
      NULL,
      &tailpos,
      MPI_INT64_T,
      comm_rank,
      OFFSET_TAILPOS(queuenum),
      MPI_NO_OP,
      queue_win);
    */
    uint64_t fused_val;
    MPI_Get(
      &fused_val,
      1,
      MPI_INT64_T,
      comm_rank,
      OFFSET_WRITECNT(queuenum),
      1,
      MPI_INT64_T,
      queue_win);
    MPI_Win_flush(comm_rank, queue_win);

    tailpos = second(fused_val);

    if (tailpos > 0) {
      DART_LOG_TRACE("Queue %ld has tailpos %ld", queuenum, tailpos);

      int64_t writecnt;
      int64_t queue_tmp      = 0;
      int64_t newqueue       = (queuenum == 0) ? 1 :  0;
      int64_t queue_swap_sum = (queuenum == 0) ? 1 : -1;

      const int64_t processing_signal     = fuse( PROCESSING_SIGNAL, 0);
      const int64_t neg_processing_signal = fuse(-PROCESSING_SIGNAL, -amsgq->prev_tailpos);

      // reset the writecnt on the new queue
      MPI_Accumulate(
        &neg_processing_signal,    1, MPI_INT64_T, comm_rank,
        OFFSET_WRITECNT(newqueue), 1, MPI_INT64_T, MPI_SUM, queue_win);

      // swap the queue number
      MPI_Fetch_and_op(
        &queue_swap_sum,
        &queue_tmp,
        MPI_INT64_T,
        comm_rank,
        OFFSET_QUEUENUM,
        MPI_SUM,
        queue_win);

      // wait for all writers to finish
      MPI_Fetch_and_op(
        &processing_signal,
        &fused_val,
        MPI_INT64_T,
        comm_rank,
        OFFSET_WRITECNT(queuenum),
        MPI_SUM,
        queue_win);

      do {
        MPI_Win_flush(comm_rank, queue_win);

        writecnt = first(fused_val);
        if (!(writecnt == 0 || writecnt == PROCESSING_SIGNAL)) {
          /*
          MPI_Fetch_and_op(
            &tmp, // not relevant
            &fused_val,
            MPI_INT64_T,
            comm_rank,
            OFFSET_WRITECNT(queuenum),
            MPI_NO_OP,
            queue_win);
          */
          // TODO: using get here to save on atomics, might have to change later
          MPI_Get(&fused_val, 1, MPI_INT64_T, comm_rank,
                  OFFSET_WRITECNT(queuenum), 1, MPI_UINT64_T, queue_win);
          // flush will happen in next iteration
        }
      } while (!(writecnt == 0 || writecnt == PROCESSING_SIGNAL));

      DART_ASSERT(queue_tmp == queuenum);

      tailpos = second(fused_val);
      amsgq->prev_tailpos = tailpos;

      DART_LOG_TRACE("Starting processing queue %ld: tailpos %ld",
                     queuenum, tailpos);

      void *dbuf = (void*)((intptr_t)amsgq->queue_ptr +
                                  OFFSET_DATA(queuenum, amsgq->queue_size));
      dart__amsgq__process_buffer(dbuf, tailpos);

    }
  } while (blocking && tailpos > 0);
  dart__base__mutex_unlock(&amsgq->processing_mutex);
  return DART_OK;
}

static
dart_ret_t
dart_amsg_sopnop_process(struct dart_amsgq_impl_data * amsgq)
{
  return amsg_sopnop_process_internal(amsgq, false);
}


static
dart_ret_t
dart_amsg_sopnop_process_blocking(
  struct dart_amsgq_impl_data * amsgq, dart_team_t team)
{
  int         flag = 0;
  MPI_Request req;

  if (!do_flush) {
    // flush all outstanding deregistrations
    MPI_Win_flush_all(amsgq->queue_win);
  }

  // keep processing until all incoming messages have been dealt with
  MPI_Ibarrier(amsgq->comm, &req);
  do {
    amsg_sopnop_process_internal(amsgq, false);
    MPI_Test(&req, &flag, MPI_STATUSES_IGNORE);
    if (!flag && sleep_us > 0) {
      // sleep for the requested number of microseconds
      nanosleep(&sleeptime, NULL);
    }
  } while (!flag);
  amsg_sopnop_process_internal(amsgq, true);
  MPI_Barrier(amsgq->comm);
  return DART_OK;
}


static dart_ret_t
dart_amsg_sopnop_closeq(struct dart_amsgq_impl_data* amsgq)
{
  // check for late messages
  int64_t tailpos1, tailpos2, writecnt1, writecnt2;
  uint64_t fused_val1, fused_val2;
  int     unitid = amsgq->comm_rank;

  MPI_Fetch_and_op(
    NULL,
    &fused_val1,
    MPI_INT64_T,
    unitid,
    OFFSET_WRITECNT(0),
    MPI_NO_OP,
    amsgq->queue_win);
  MPI_Fetch_and_op(
    NULL,
    &fused_val2,
    MPI_INT64_T,
    unitid,
    OFFSET_WRITECNT(1),
    MPI_NO_OP,
    amsgq->queue_win);
  MPI_Win_flush_local(unitid, amsgq->queue_win);

  tailpos1 = second(fused_val1);
  writecnt1 = first(fused_val1);
  tailpos2 = second(fused_val2);
  writecnt2 = first(fused_val2);

  if ((writecnt1 >= 0 && tailpos1 > 0) || (writecnt2 >= 0 && tailpos2 > 0)) {
    DART_LOG_WARN("Cowardly refusing to invoke unhandled incoming active "
                  "messages upon shutdown (tailpos %d+%d)!", tailpos1, tailpos2);
  }

  // free window
  amsgq->queue_ptr = NULL;
  MPI_Win_unlock_all(amsgq->queue_win);
  MPI_Win_free(&(amsgq->queue_win));

  MPI_Comm_free(&amsgq->comm);

  dart__base__mutex_destroy(&amsgq->send_mutex);
  dart__base__mutex_destroy(&amsgq->processing_mutex);

  free(amsgq->outidx);
  free(amsgq->states);
  free(amsgq->reqs);

  free(amsgq);

  return DART_OK;
}


dart_ret_t
dart_amsg_sopnop5_init(dart_amsgq_impl_t* impl)
{
  impl->openq   = dart_amsg_sopnop_openq;
  impl->closeq  = dart_amsg_sopnop_closeq;
  impl->trysend = dart_amsg_sopnop_sendbuf;
  impl->trysend_all = dart_amsg_sopnop_sendbuf_all;
  impl->process = dart_amsg_sopnop_process;
  impl->process_blocking = dart_amsg_sopnop_process_blocking;
  return DART_OK;
}

#else

dart_ret_t
dart_amsg_sopnop4_init(dart_amsgq_impl_t* impl)
{
  DART_LOG_ERROR("MPI is missing support for extended generalized requests!");
  return DART_ERR_INVAL;
}

#endif // DART_HAVE_MPI_EGREQ
