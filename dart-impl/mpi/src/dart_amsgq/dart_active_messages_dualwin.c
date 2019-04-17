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
#include <dash/dart/base/atomic.h>
#include <dash/dart/base/env.h>
#include <dash/dart/if/dart_active_messages.h>
#include <dash/dart/if/dart_communication.h>
#include <dash/dart/if/dart_globmem.h>
#include <dash/dart/if/dart_tasking.h>
#include <dash/dart/mpi/dart_team_private.h>
#include <dash/dart/mpi/dart_globmem_priv.h>
#include <dash/dart/mpi/dart_active_messages_priv.h>

#define PROCESSING_SIGNAL ((int64_t)INT32_MIN)

struct dart_amsgq_impl_data {
  MPI_Win           queue1_win;
  MPI_Win           queue2_win;
  MPI_Win           queuenum_win;
  int64_t          *queue1_ptr;
  int64_t          *queue2_ptr;
  int64_t          *queuenum_ptr;
  char             *buffer;
  uint64_t          queue_size;
  MPI_Comm          comm;
  dart_mutex_t      send_mutex;
  dart_mutex_t      processing_mutex;
  int64_t           prev_tailpos;
};

/**
 * Name of the environment variable specifying the number microseconds a caller
 * sleeps between consecutive reads of the active message in a blocking
 * processing call.
 *
 * Type: integral value with optional us, ms, s qualifier
 */
#define DART_AMSGQ_DUALWIN_SLEEP_ENVSTR  "DART_AMSGQ_DUALWIN_SLEEP"

#ifdef DART_ENABLE_LOGGING
static uint32_t msgcnt = 0;
#endif // DART_ENABLE_LOGGING

static struct timespec sleeptime = {-1, -1};
static int64_t sleep_us = -1;

static dart_ret_t
dart_amsg_dualwin_openq(
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
    sleep_us  = dart__base__env__us(DART_AMSGQ_DUALWIN_SLEEP_ENVSTR, 0);
    sleeptime.tv_sec  = sleep_us / 1000000;
    sleeptime.tv_nsec = sleep_us % 1000000;
  }

  struct dart_amsgq_impl_data *res = calloc(1, sizeof(struct dart_amsgq_impl_data));
  res->queue_size = msg_count * msg_size;
  MPI_Comm_dup(team_data->comm, &res->comm);

  // TODO: check if it is better to use 32-bit integer?!
  size_t win_size = sizeof(int64_t)      // queue offset (64-bit to guarantee alignment)
                    + res->queue_size;   // queue buffer

  dart__base__mutex_init(&res->send_mutex);
  dart__base__mutex_init(&res->processing_mutex);

  // we don't need MPI to take care of the ordering since we use
  // explicit flushes to guarantee ordering
  MPI_Info info;
  MPI_Info_create(&info);
  MPI_Info_set(info, "accumulate_ordering", "none");
  MPI_Info_set(info, "same_size"          , "true");
  MPI_Info_set(info, "same_disp_unit"     , "true");
  MPI_Info_set(info, "accumulate_ops"     , "same_op_no_op");

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
    &(res->queue1_ptr),
    &(res->queue1_win));
  MPI_Win_allocate(
    win_size,
    1,
    info,
    res->comm,
    &(res->queue2_ptr),
    &(res->queue2_win));
  MPI_Win_allocate(
    sizeof(uint64_t),
    1,
    info,
    res->comm,
    &(res->queuenum_ptr),
    &(res->queuenum_win));
  MPI_Info_free(&info);

  memset(res->queue1_ptr, 0, win_size);
  memset(res->queue2_ptr, 0, win_size);
  // mark the second queue as inactive
  *res->queue2_ptr = PROCESSING_SIGNAL;
  memset(res->queuenum_ptr, 0, sizeof(uint64_t));

  res->buffer = malloc(win_size);

  MPI_Win_lock_all(0, res->queuenum_win);

  MPI_Barrier(res->comm);

  DART_LOG_DEBUG("Allocated double-buffered message queue (buffer: %ld)",
                 res->queue_size);

  *queue = res;

  return DART_OK;
}


static
dart_ret_t
dart_amsg_dualwin_sendbuf(
  dart_team_unit_t              target,
  struct dart_amsgq_impl_data * amsgq,
  const void                  * data,
  size_t                        data_size)
{

  DART_LOG_DEBUG("dart_amsg_trysend: u:%i ds:%zu",
                 target.id, data_size);

  dart__base__mutex_lock(&amsgq->send_mutex);

  int64_t msg_size = data_size;
  int64_t offset;
  int64_t queuenum;
  MPI_Win queue_win;

  do {

    /**
     * Number of accumulate ops: 1 op
     */

    // fetch queue number
    MPI_Request req;
    MPI_Rget(&queuenum, 1, MPI_INT64_T, target.id,
            0, 1, MPI_INT64_T, amsgq->queuenum_win, &req);
    MPI_Wait(&req, MPI_STATUS_IGNORE);

    DART_ASSERT(queuenum == 0 || queuenum == 1);

    // query the queue number
    queue_win = (queuenum == 0) ? amsgq->queue1_win : amsgq->queue2_win;

    // get a shared lock to keep the reader out
    MPI_Win_lock(MPI_LOCK_SHARED, target.id, 0, queue_win);

    // atomically fetch and update the writer offset
    MPI_Fetch_and_op(
      &msg_size,
      &offset,
      MPI_INT64_T,
      target.id,
      0,
      MPI_SUM,
      queue_win);
    MPI_Win_flush_local(target.id, queue_win);

    if (offset >= 0 && (offset + msg_size) <= amsgq->queue_size) break;

    // the queue is full, reset the offset
    int64_t neg_msg_size = -msg_size;
    DART_LOG_TRACE("Queue %ld at %d full (tailpos %ld),"
                  "reverting by %ld",
                  queuenum, target.id, offset, neg_msg_size);
    MPI_Accumulate(&neg_msg_size, 1, MPI_INT64_T, target.id,
                    0, 1, MPI_INT64_T, MPI_SUM, queue_win);
    MPI_Win_unlock(target.id, queue_win);

    // return error if the queue is full, otherwise try again
    if (offset >= 0) {
    dart__base__mutex_unlock(&amsgq->send_mutex);
      return DART_ERR_AGAIN;
    }
  } while (1);

  DART_LOG_TRACE("Writing %ld B at offset %ld at unit %i",
                 data_size, offset, target.id);

  // Write our payload

  DART_LOG_TRACE("MPI_Put into queue %ld offset %ld (%ld)",
                 queuenum, offset, sizeof(uint64_t) + offset);
  MPI_Put(
    data,
    data_size,
    MPI_BYTE,
    target.id,
    sizeof(uint64_t) + offset,
    data_size,
    MPI_BYTE,
    queue_win);

  MPI_Win_unlock(target.id, queue_win);

  DART_LOG_TRACE("Sent message of size %zu with payload %zu to unit "
                "%d starting at offset %ld",
                msg_size, data_size, target.id, offset);

  dart__base__mutex_unlock(&amsgq->send_mutex);
  return DART_OK;
}

static dart_ret_t
amsg_sopnop_process_internal(
  struct dart_amsgq_impl_data * amsgq,
  bool                          blocking)
{
  int     unitid;
  uint64_t tailpos;

  if (!blocking) {
    dart_ret_t ret = dart__base__mutex_trylock(&amsgq->processing_mutex);
    if (ret != DART_OK) {
      return DART_ERR_AGAIN;
    }
  } else {
    dart__base__mutex_lock(&amsgq->processing_mutex);
  }

  MPI_Comm_rank(amsgq->comm, &unitid);

  do {

    uint64_t queuenum = *(uint64_t*)amsgq->queuenum_ptr;

    DART_ASSERT(queuenum == 0 || queuenum == 1);

    MPI_Win newqueue_win = (queuenum == 1) ? amsgq->queue1_win : amsgq->queue2_win;
    uint64_t *newqueue_ptr = (uint64_t*)(queuenum == 1 ? amsgq->queue1_ptr
                                                       : amsgq->queue2_ptr);
    MPI_Win queue_win = (queuenum == 0) ? amsgq->queue1_win : amsgq->queue2_win;
    int64_t *queue_ptr = (int64_t*)(queuenum == 0 ? amsgq->queue1_ptr
                                                  : amsgq->queue2_ptr);

    // mark the new queue as active
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, unitid, 0, newqueue_win);
    int64_t tmp = *newqueue_ptr;
    *newqueue_ptr = 0;
    MPI_Win_unlock(unitid, newqueue_win);
    DART_ASSERT_MSG(tmp < 0, "Invalid previous offset %ld found!", tmp);

    // swap the queue number
    uint64_t newqueue = (queuenum == 0) ? 1 : 0;
    MPI_Put(&newqueue, 1, MPI_UINT64_T, unitid, 0, 1,
            MPI_UINT64_T, amsgq->queuenum_win);
    MPI_Win_flush(unitid, amsgq->queuenum_win);

    // take an exclusive lock to keep writers out
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, unitid, 0, queue_win);

    tailpos = *queue_ptr;

    // mark the queue as inactive
    *queue_ptr = PROCESSING_SIGNAL;

    if (tailpos > 0) {
      DART_LOG_TRACE("Queue %ld has tailpos %ld", queuenum, tailpos);

      DART_LOG_TRACE("Starting to process queue %ld: tailpos %ld",
                     queuenum, tailpos);

      // copy the data out...
      memcpy(amsgq->buffer, queue_ptr + 1, tailpos);

      // ... and unlock the window
      MPI_Win_unlock(unitid, queue_win);

      // start processing
      dart__amsgq__process_buffer(amsgq->buffer, tailpos);
    } else {
      // release the exclusive lock
      MPI_Win_unlock(unitid, queue_win);
    }
  } while (blocking && tailpos > 0);
  dart__base__mutex_unlock(&amsgq->processing_mutex);
  return DART_OK;
}

static
dart_ret_t
dart_amsg_dualwin_process(struct dart_amsgq_impl_data * amsgq)
{
  return amsg_sopnop_process_internal(amsgq, false);
}


static
dart_ret_t
dart_amsg_dualwin_process_blocking(
  struct dart_amsgq_impl_data * amsgq, dart_team_t team)
{
  int         flag = 0;
  MPI_Request req;

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

  DART_LOG_TRACE("Finished blocking processing of queue!");

  return DART_OK;
}


static dart_ret_t
dart_amsg_dualwin_closeq(struct dart_amsgq_impl_data* amsgq)
{

  MPI_Win_free(&(amsgq->queue1_win));
  MPI_Win_free(&(amsgq->queue2_win));
  MPI_Win_unlock_all(amsgq->queuenum_win);
  MPI_Win_free(&(amsgq->queuenum_win));

  MPI_Comm_free(&amsgq->comm);

  dart__base__mutex_destroy(&amsgq->send_mutex);
  dart__base__mutex_destroy(&amsgq->processing_mutex);

  free(amsgq->buffer);
  free(amsgq);

  return DART_OK;
}


dart_ret_t
dart_amsg_dualwin_init(dart_amsgq_impl_t* impl)
{
  impl->openq   = dart_amsg_dualwin_openq;
  impl->closeq  = dart_amsg_dualwin_closeq;
  impl->trysend = dart_amsg_dualwin_sendbuf;
  impl->process = dart_amsg_dualwin_process;
  impl->process_blocking = dart_amsg_dualwin_process_blocking;
  return DART_OK;
}
