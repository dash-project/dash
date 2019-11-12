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
  int               comm_rank;
  int64_t           prev_tailpos;
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

    uint64_t writecnt_offset = fuse(1, msg_size);

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

    DART_LOG_TRACE("Queue %ld at %d: writecnt %ld, offset %ld (fused_val %lu)",
                   queuenum, target.id, writecnt, offset, fused_val);

    bool do_return = false;

    if (writecnt >= 0) {

      // if the message fits we can write it
      if (offset >= 0 && (offset + msg_size) <= amsgq->queue_size) break;

      // the queue is full, reset the offset
      DART_LOG_TRACE("Queue %ld at %d full (tailpos %ld, writecnt %ld)",
                    queuenum, target.id, offset, writecnt);
      do_return = true;
    } else {
      DART_LOG_TRACE("Queue %ld at %d processing, retrying (writecnt %ld)",
                    queuenum, target.id, writecnt);
    }
    writecnt_offset = fuse(-1, -msg_size);
    // deregister as a writer
    update_value(&writecnt_offset, target.id, OFFSET_WRITECNT(queuenum), queue_win);

    MPI_Win_flush_local(target.id, queue_win);

    if (do_return) {
      return DART_ERR_AGAIN;
    }
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
      int64_t tmp = 0;
      int64_t newqueue = (queuenum == 0) ? 1 : 0;
      int64_t queue_swap_sum = (queuenum == 0) ? 1 : -1;

      const int64_t processing_signal     = fuse( PROCESSING_SIGNAL, 0);
      const int64_t neg_processing_signal = fuse(-PROCESSING_SIGNAL, 0);

      // reset the writecnt on the new queue to release it
      uint64_t fused_tmpval;
      MPI_Fetch_and_op(
        &neg_processing_signal,
        &fused_tmpval,
        MPI_INT64_T,
        comm_rank,
        OFFSET_WRITECNT(newqueue),
        MPI_SUM,
        queue_win);

      // swap the queue number
      MPI_Fetch_and_op(
        &queue_swap_sum,
        &tmp,
        MPI_INT64_T,
        comm_rank,
        OFFSET_QUEUENUM,
        MPI_SUM,
        queue_win);

#ifdef DART_ENABLE_ASSERTIONS
      MPI_Win_flush(comm_rank, queue_win);
      DART_ASSERT(tmp == queuenum);
      int32_t oldwritecnt = first(fused_tmpval);
      if (oldwritecnt < PROCESSING_SIGNAL) {
        DART_LOG_ERROR("oldwritecnt too small: %d (limit %d)", oldwritecnt, PROCESSING_SIGNAL);
      }
      DART_ASSERT(oldwritecnt >= PROCESSING_SIGNAL);
#endif // DART_ENABLE_ASSERTIONS

      // wait for all writers to finish
      MPI_Fetch_and_op(
        &processing_signal,
        &fused_val,
        MPI_INT64_T,
        comm_rank,
        OFFSET_WRITECNT(queuenum),
        MPI_SUM,
        queue_win);
      MPI_Win_flush(comm_rank, queue_win);

      writecnt = first(fused_val);

      if (writecnt > 0) {
        DART_LOG_TRACE("Waiting for writecnt=%ld writers on queue %ld to finish",
                       writecnt, queuenum);

        do {
          MPI_Fetch_and_op(
            &tmp, // not relevant
            &fused_val,
            MPI_INT64_T,
            comm_rank,
            OFFSET_WRITECNT(queuenum),
            MPI_NO_OP,
            queue_win);
          /*
          MPI_Get(
            &writecnt,
            1,
            MPI_INT64_T,
            comm_rank,
            OFFSET_WRITECNT(queuenum),
            1,
            MPI_INT64_T,
            queue_win);
          */
          MPI_Win_flush(comm_rank, queue_win);
          writecnt = first(fused_val);
        } while (writecnt > PROCESSING_SIGNAL);
        DART_LOG_TRACE("Done waiting for writers on queue %ld", queuenum);
      }
      tailpos = second(fused_val);

      // reset tailpos
      fused_val = fuse(0, -tailpos);
      update_value(&fused_val, comm_rank, OFFSET_WRITECNT(queuenum), queue_win);

      DART_LOG_TRACE("Starting processing queue %ld: tailpos %ld",
                     queuenum, tailpos);

      void *dbuf = (void*)((intptr_t)amsgq->queue_ptr +
                                  OFFSET_DATA(queuenum, amsgq->queue_size));
      dart__amsgq__process_buffer(dbuf, tailpos);

      // flush the reset of the tailpos
      MPI_Win_flush(comm_rank, queue_win);

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
  int64_t tailpos1, tailpos2;
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
  tailpos2 = second(fused_val2);

  if (tailpos1 > 0 || tailpos2 > 0) {
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

  free(amsgq);

  return DART_OK;
}


dart_ret_t
dart_amsg_sopnop2_init(dart_amsgq_impl_t* impl)
{
  impl->openq   = dart_amsg_sopnop_openq;
  impl->closeq  = dart_amsg_sopnop_closeq;
  impl->trysend = dart_amsg_sopnop_sendbuf;
  impl->process = dart_amsg_sopnop_process;
  impl->process_blocking = dart_amsg_sopnop_process_blocking;
  return DART_OK;
}
