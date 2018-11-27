#include <stdlib.h>
#include <unistd.h>
#include <mpi.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <alloca.h>

#include <dash/dart/base/mutex.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/base/assert.h>
#include <dash/dart/base/atomic.h>
#include <dash/dart/if/dart_active_messages.h>
#include <dash/dart/if/dart_communication.h>
#include <dash/dart/if/dart_globmem.h>
#include <dash/dart/mpi/dart_team_private.h>
#include <dash/dart/mpi/dart_globmem_priv.h>
#include <dash/dart/mpi/dart_active_messages_priv.h>

#define PROCESSING_SIGNAL ((int64_t)INT32_MIN)

struct dart_amsgq_impl_data {
  MPI_Win           queue_win;
  int64_t          *queue_ptr;
  uint64_t          queue_size;
  MPI_Comm          comm;
  dart_mutex_t      send_mutex;
  dart_mutex_t      processing_mutex;
  int64_t           prev_tailpos;
};

#ifdef DART_ENABLE_LOGGING
static uint32_t msgcnt = 0;
#endif // DART_ENABLE_LOGGING

#define OFFSET_QUEUENUM                 0
#define OFFSET_TAILPOS(q)    (sizeof(int64_t)+q*2*sizeof(int64_t))
#define OFFSET_WRITECNT(q)   (OFFSET_TAILPOS(q)+sizeof(int64_t))
#define OFFSET_DATA(q, qs)   (OFFSET_WRITECNT(1)+sizeof(int64_t)+q*qs)

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

  struct dart_amsgq_impl_data *res = calloc(1, sizeof(struct dart_amsgq_impl_data));
  res->queue_size = msg_count * msg_size;
  MPI_Comm_dup(team_data->comm, &res->comm);

  // TODO: check if it is better to use 32-bit integer?!
  size_t win_size = sizeof(int64_t)          // queuenumber (64-bit to guarantee alignment)
                    + 2*(sizeof(int64_t)     // tailpos of each queue
                       + sizeof(uint64_t))   // the writer count of each queue
                    + 2*(res->queue_size);   // queue double-buffer

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
    &(res->queue_ptr),
    &(res->queue_win));
  MPI_Info_free(&info);

  memset(res->queue_ptr, 0, win_size);

  // properly initialize the writecnt of the second queue
  *(int64_t*)(((intptr_t)res->queue_ptr) + OFFSET_WRITECNT(1)) = PROCESSING_SIGNAL;

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

  DART_LOG_DEBUG("dart_amsg_trysend: u:%i ds:%zu",
                 target.id, data_size);

  const int64_t one  = 1;
  const int64_t mone = -1;
  int64_t msg_size = data_size;
  int64_t offset;
  int64_t queuenum;
  int64_t writecnt;

  do {

    /**
     * Number of accumulate ops: 4 ops
     */

    // fetch queue number
    MPI_Fetch_and_op(
      NULL,
      &queuenum,
      MPI_INT64_T,
      target.id,
      OFFSET_QUEUENUM,
      MPI_NO_OP,
      amsgq->queue_win);
    MPI_Win_flush_local(target.id, amsgq->queue_win);

    DART_ASSERT(queuenum == 0 || queuenum == 1);

    // register as a writer
    MPI_Fetch_and_op(
      &one,
      &writecnt,
      MPI_INT64_T,
      target.id,
      OFFSET_WRITECNT(queuenum),
      MPI_SUM,
      amsgq->queue_win);
    MPI_Win_flush(target.id, amsgq->queue_win);

    if (writecnt >= 0) {
      // atomically fetch and update the writer offset
      MPI_Fetch_and_op(
        &msg_size,
        &offset,
        MPI_INT64_T,
        target.id,
        OFFSET_TAILPOS(queuenum),
        MPI_SUM,
        amsgq->queue_win);
      MPI_Win_flush(target.id, amsgq->queue_win);

      if (offset >= 0 && (offset + msg_size) <= amsgq->queue_size) break;

      // the queue is full, reset the offset
      int64_t neg_msg_size = -msg_size;
      DART_LOG_TRACE("Queue %ld at %d full (tailpos %ld, writecnt %ld),"
                    "reverting by %ld",
                    queuenum, target.id, offset, writecnt, neg_msg_size);
      MPI_Accumulate(&neg_msg_size, 1, MPI_INT64_T, target.id,
                    OFFSET_TAILPOS(queuenum), 1, MPI_INT64_T,
                    MPI_SUM, amsgq->queue_win);
      MPI_Win_flush(target.id, amsgq->queue_win);
    } else {
      DART_LOG_TRACE("Queue %ld at %d processing (writecnt %ld)",
                    queuenum, target.id, writecnt);
    }
    // deregister as a writer
    MPI_Fetch_and_op(
      &mone,
      &writecnt,
      MPI_INT64_T,
      target.id,
      OFFSET_WRITECNT(queuenum),
      MPI_SUM,
      amsgq->queue_win);
    MPI_Win_flush(target.id, amsgq->queue_win);

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
    amsgq->queue_win);
  // we have to flush here because MPI has no ordering guarantees
  MPI_Win_flush(target.id, amsgq->queue_win);

  DART_LOG_TRACE("Unregistering as writer from queue %ld at unit %i",
                 queuenum, target.id);

  // deregister as a writer
  MPI_Fetch_and_op(
    &mone,
    &writecnt,
    MPI_INT64_T,
    target.id,
    OFFSET_WRITECNT(queuenum),
    MPI_SUM,
    amsgq->queue_win);
  MPI_Win_flush(target.id, amsgq->queue_win);

  DART_LOG_INFO("Sent message of size %zu with payload %zu to unit "
                "%d starting at offset %ld (writecnt=%ld)",
                msg_size, data_size, target.id, offset, writecnt-1);

  return DART_OK;
}

static dart_ret_t
amsg_sopnop_process_internal(
  struct dart_amsgq_impl_data * amsgq,
  bool                          blocking)
{
  int     unitid;
  int64_t tailpos;

  if (!blocking) {
    dart_ret_t ret = dart__base__mutex_trylock(&amsgq->processing_mutex);
    if (ret != DART_OK) {
      return DART_ERR_AGAIN;
    }
  } else {
    dart__base__mutex_lock(&amsgq->processing_mutex);
  }

  do {
    MPI_Comm_rank(amsgq->comm, &unitid);

    int64_t queuenum = *(int64_t*)amsgq->queue_ptr;

    DART_ASSERT(queuenum == 0 || queuenum == 1);

    //printf("Reading from queue %i\n", queuenum);

    /**
     * Number of accumulate ops: 5 ops
     */

    // see whether there is anything available
    MPI_Fetch_and_op(
      NULL,
      &tailpos,
      MPI_INT64_T,
      unitid,
      OFFSET_TAILPOS(queuenum),
      MPI_NO_OP,
      amsgq->queue_win);
    MPI_Win_flush(unitid, amsgq->queue_win);


    if (tailpos > 0) {
      DART_LOG_TRACE("Queue %ld has tailpos %ld", queuenum, tailpos);
      const int64_t zero = 0;

      int64_t writecnt;
      int64_t tmp = 0;
      int64_t newqueue = (queuenum == 0) ? 1 : 0;
      int64_t queue_swap_sum = (queuenum == 0) ? 1 : -1;

      const int64_t processing_signal     = PROCESSING_SIGNAL;
      const int64_t neg_processing_signal = -PROCESSING_SIGNAL;

      // swap the queue number and reset writecnt
      MPI_Fetch_and_op(
        &queue_swap_sum,
        &tmp,
        MPI_INT64_T,
        unitid,
        OFFSET_QUEUENUM,
        MPI_SUM,
        amsgq->queue_win);

      MPI_Fetch_and_op(
        &neg_processing_signal,
        &writecnt,
        MPI_INT64_T,
        unitid,
        OFFSET_WRITECNT(newqueue),
        MPI_SUM,
        amsgq->queue_win);

      MPI_Win_flush(unitid, amsgq->queue_win);
      DART_ASSERT(tmp == queuenum);
      DART_ASSERT(writecnt >= processing_signal);

      // wait for all writers to finish
      MPI_Fetch_and_op(
        &processing_signal,
        &writecnt,
        MPI_INT64_T,
        unitid,
        OFFSET_WRITECNT(queuenum),
        MPI_SUM,
        amsgq->queue_win);
      MPI_Win_flush(unitid, amsgq->queue_win);

      if (writecnt > 0) {
        DART_LOG_TRACE("Waiting for writecnt=%ld writers on queue %ld to finish",
                       writecnt, queuenum);

        do {
          MPI_Fetch_and_op(
            NULL,
            &writecnt,
            MPI_INT64_T,
            unitid,
            OFFSET_WRITECNT(queuenum),
            MPI_NO_OP,
            amsgq->queue_win);
          MPI_Win_flush(unitid, amsgq->queue_win);
        } while (writecnt > processing_signal);
        DART_LOG_TRACE("Done waiting for writers on queue %ld", queuenum);
      }


      // reset tailpos
      // NOTE: using MPI_REPLACE here is valid as no-one else will write to it
      //       at this time.
      MPI_Fetch_and_op(
        &zero,
        &tailpos,
        MPI_INT64_T,
        unitid,
        OFFSET_TAILPOS(queuenum),
        MPI_REPLACE,
        amsgq->queue_win);
      MPI_Win_flush(unitid, amsgq->queue_win);

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

  // keep processing until all incoming messages have been dealt with
  MPI_Ibarrier(amsgq->comm, &req);
  do {
    amsg_sopnop_process_internal(amsgq, false);
    MPI_Test(&req, &flag, MPI_STATUSES_IGNORE);
  } while (!flag);
  amsg_sopnop_process_internal(amsgq, true);
  MPI_Barrier(amsgq->comm);
  return DART_OK;
}


static dart_ret_t
dart_amsg_sopnop_closeq(struct dart_amsgq_impl_data* amsgq)
{
  // check for late messages
  uint32_t tailpos1, tailpos2;
  int      unitid;

  MPI_Comm_rank(amsgq->comm, &unitid);

  MPI_Fetch_and_op(
    NULL,
    &tailpos1,
    MPI_INT32_T,
    unitid,
    OFFSET_TAILPOS(0),
    MPI_NO_OP,
    amsgq->queue_win);
  MPI_Fetch_and_op(
    NULL,
    &tailpos2,
    MPI_INT32_T,
    unitid,
    OFFSET_TAILPOS(1),
    MPI_NO_OP,
    amsgq->queue_win);
  MPI_Win_flush_local(unitid, amsgq->queue_win);

  if (tailpos1 > 0 || tailpos2 > 0) {
    DART_LOG_WARN("Cowardly refusing to invoke unhandled incoming active "
                  "messages upon shutdown (tailpos %d+%d)!", tailpos1, tailpos2);
  }

  // free window
  amsgq->queue_ptr = NULL;
  MPI_Win_unlock_all(amsgq->queue_win);
  MPI_Win_free(&(amsgq->queue_win));

  MPI_Comm_free(&amsgq->comm);

  free(amsgq);

  dart__base__mutex_destroy(&amsgq->send_mutex);
  dart__base__mutex_destroy(&amsgq->processing_mutex);

  return DART_OK;
}


dart_ret_t
dart_amsg_sopnop_init(dart_amsgq_impl_t* impl)
{
  impl->openq   = dart_amsg_sopnop_openq;
  impl->closeq  = dart_amsg_sopnop_closeq;
  impl->trysend = dart_amsg_sopnop_sendbuf;
  impl->process = dart_amsg_sopnop_process;
  impl->process_blocking = dart_amsg_sopnop_process_blocking;
  return DART_OK;
}
