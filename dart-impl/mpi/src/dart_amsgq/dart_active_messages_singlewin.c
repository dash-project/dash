#include <stdlib.h>
#include <unistd.h>
#include <mpi.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>

#include <dash/dart/base/mutex.h>
#include <dash/dart/base/assert.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/if/dart_active_messages.h>
#include <dash/dart/if/dart_communication.h>
#include <dash/dart/if/dart_globmem.h>
#include <dash/dart/mpi/dart_team_private.h>
#include <dash/dart/mpi/dart_globmem_priv.h>
#include <dash/dart/mpi/dart_active_messages_priv.h>

/**
 * TODO: check whether it makes sense to use atomics+shared locks for writers!
 */

//#define DART_AMSGQ_ATOMICS 1

struct dart_amsgq_impl_data {
  /// window holding the tail ptr
  MPI_Win      win;
  uint64_t    *tailpos_ptr;
  /// a double buffer used during message processing
  char        *dbuf;
  /// the size (in byte) of the message queue
  uint64_t     size;
  dart_team_t  team;
  MPI_Comm     comm;
  dart_mutex_t send_mutex;
  dart_mutex_t processing_mutex;
  int          my_rank;
};

static
dart_ret_t
dart_amsg_singlewin_openq(
  size_t         msg_size,
  size_t         msg_count,
  dart_team_t    team,
  struct dart_amsgq_impl_data** queue)
{
  struct dart_amsgq_impl_data* res = calloc(1, sizeof(struct dart_amsgq_impl_data));
  res->size = msg_count * + msg_size;
  res->dbuf = malloc(res->size);
  res->size += sizeof(*(res->tailpos_ptr));
  res->team = team;

  *queue = NULL;

  dart__base__mutex_init(&res->send_mutex);
  dart__base__mutex_init(&res->processing_mutex);

  dart_team_data_t *team_data = dart_adapt_teamlist_get(team);
  if (team_data == NULL) {
    DART_LOG_ERROR("dart_gptr_getaddr ! Unknown team %i", team);
    return DART_ERR_INVAL;
  }
  MPI_Comm_dup(team_data->comm, &res->comm);
  MPI_Comm_rank(res->comm, &res->my_rank);

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
    res->size,
    1,
    info,
    res->comm,
    (void*)&(res->tailpos_ptr),
    &(res->win));
  *(res->tailpos_ptr) = 0;

  MPI_Info_free(&info);

  MPI_Barrier(res->comm);

  *queue = res;

  return DART_OK;
}

static
dart_ret_t
dart_amsg_singlewin_trysend(
  dart_team_unit_t              target,
  struct dart_amsgq_impl_data * amsgq,
  const void                  * data,
  size_t                        data_size)
{
  uint64_t msg_size = data_size;
  uint64_t remote_offset = 0;

  dart__base__mutex_lock(&amsgq->send_mutex);

  //lock the tailpos window
  MPI_Win_lock(MPI_LOCK_EXCLUSIVE, target.id, 0, amsgq->win);

#ifdef DART_AMSGQ_ATOMICS
  // Add the size of the message to the tailpos at the target
  if (MPI_Fetch_and_op(
        &msg_size,
        &remote_offset,
        MPI_UINT64_T,
        target.id,
        0,
        MPI_SUM,
        amsgq->win) != MPI_SUCCESS) {
    DART_LOG_ERROR("MPI_Fetch_and_op failed!");
    dart__base__mutex_unlock(&amsgq->send_mutex);
    return DART_ERR_NOTINIT;
  }
  MPI_Win_flush(target.id, amsgq->win);

  DART_LOG_TRACE("MPI_Fetch_and_op returned offset %lu at unit %i",
                 remote_offset, target.id);

#else

  MPI_Request req;
  MPI_Rget(&remote_offset, 1, MPI_UINT64_T, target.id, 0, 1, MPI_UINT64_T, amsgq->win, &req);
  MPI_Wait(&req, MPI_STATUS_IGNORE);

  DART_LOG_TRACE("MPI_Rget returned offset %lu at unit %i",
                 remote_offset, target.id);

#endif // DART_AMSGQ_ATOMICS

  if (remote_offset >= amsgq->size) {
    DART_LOG_ERROR("Received offset larger than message queue size from "
                   "unit %i (%lu but expected < %lu)",
                   target.id, remote_offset, amsgq->size);
    MPI_Win_unlock(target.id, amsgq->win);
    dart__base__mutex_unlock(&amsgq->send_mutex);
    return DART_ERR_INVAL;
  }

  if ((remote_offset + msg_size) >= amsgq->size) {
#ifdef DART_AMSGQ_ATOMICS
    uint64_t tmp;
    static int msg_printed = 0;
    if (!msg_printed) {
      msg_printed = 1;
      DART_LOG_WARN("Message queue at unit %d is full, please consider raising "
                    "the queue size (currently %zuB)", target.id, amsgq->size);
    }
    // if not, revert the operation and free the lock to try again.
    MPI_Fetch_and_op(
      &remote_offset,
      &tmp,
      MPI_UINT64_T,
      target.id,
      0,
      MPI_REPLACE,
      amsgq->win);
#endif // DART_AMSGQ_ATOMICS
    MPI_Win_unlock(target.id, amsgq->win);
    DART_LOG_TRACE("Not enough space for message of size %lu at unit %i "
                   "(current offset %lu of %lu)",
                   msg_size, target.id, remote_offset, amsgq->size);
    dart__base__mutex_unlock(&amsgq->send_mutex);
    return DART_ERR_AGAIN;
  }

  // we now have a slot in the message queue
  MPI_Aint queue_disp = remote_offset + sizeof(*(amsgq->tailpos_ptr));
  // put the payload
  MPI_Put(
    data,
    data_size,
    MPI_BYTE,
    target.id,
    queue_disp,
    data_size,
    MPI_BYTE,
    amsgq->win);
#ifndef DART_AMSGQ_ATOMICS
  // update the offset
  remote_offset += msg_size;
  MPI_Put(&remote_offset, 1, MPI_UINT64_T, target.id, 0, 1, MPI_UINT64_T, amsgq->win);
#endif // !DART_AMSGQ_ATOMICS
  MPI_Win_unlock(target.id, amsgq->win);

  dart__base__mutex_unlock(&amsgq->send_mutex);

  DART_LOG_INFO("Sent message of size %zu with payload %zu to unit "
                "%i starting at offset %li",
                    msg_size, data_size, target.id, remote_offset);

  return DART_OK;
}


static dart_ret_t
amsg_singlewin_process_internal(
  struct dart_amsgq_impl_data* amsgq,
  bool                         blocking)
{
  uint64_t         tailpos;

  if (!blocking) {
    dart_ret_t ret = dart__base__mutex_trylock(&amsgq->processing_mutex);
    if (ret != DART_OK) {
      return DART_ERR_AGAIN;
    }
  } else {
    dart__base__mutex_lock(&amsgq->processing_mutex);
  }

  do {
    char *dbuf = amsgq->dbuf;

    // local lock
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, amsgq->my_rank, 0, amsgq->win);

    // query local tail position
    MPI_Get(
        &tailpos,
        1,
        MPI_UINT64_T,
        amsgq->my_rank,
        0,
        1,
        MPI_UINT64_T,
        amsgq->win);

    // MPI_Win_flush_local should be sufficient but hangs in OMPI 2.1.1
    MPI_Win_flush(amsgq->my_rank, amsgq->win);

    if (tailpos > 0) {
      const uint64_t zero = 0;
      DART_LOG_INFO("Checking for new active messages (tailpos=%lu)", tailpos);
      // copy the content of the queue for processing
      MPI_Get(
          dbuf,
          tailpos,
          MPI_BYTE,
          amsgq->my_rank,
          sizeof(*(amsgq->tailpos_ptr)),
          tailpos,
          MPI_BYTE,
          amsgq->win);

      // reset the tailpos and release the lock on the message queue
      MPI_Put(
          &zero,
          1,
          MPI_INT64_T,
          amsgq->my_rank,
          0,
          1,
          MPI_INT64_T,
          amsgq->win);
      MPI_Win_unlock(amsgq->my_rank, amsgq->win);

      dart__amsgq__process_buffer(dbuf, tailpos);

    } else {
      MPI_Win_unlock(amsgq->my_rank, amsgq->win);
    }
  } while (blocking && tailpos > 0);
  dart__base__mutex_unlock(&amsgq->processing_mutex);
  return DART_OK;
}

static
dart_ret_t
dart_amsg_singlewin_process(struct dart_amsgq_impl_data* amsgq)
{
  return amsg_singlewin_process_internal(amsgq, false);
}

static
dart_ret_t
dart_amsg_singlewin_process_blocking(
  struct dart_amsgq_impl_data* amsgq,
  dart_team_t                  team)
{
  int         flag = 0;
  MPI_Request req;

  dart_team_data_t *team_data = dart_adapt_teamlist_get(team);
  if (team_data == NULL) {
    DART_LOG_ERROR("dart_gptr_getaddr ! Unknown team %i", team);
    return DART_ERR_INVAL;
  }

  MPI_Ibarrier(team_data->comm, &req);
  do {
    amsg_singlewin_process_internal(amsgq, true);
    MPI_Test(&req, &flag, MPI_STATUSES_IGNORE);
  } while (!flag);
  amsg_singlewin_process_internal(amsgq, true);
  MPI_Barrier(team_data->comm);
  return DART_OK;
}

static
dart_ret_t
dart_amsg_singlewin_closeq(struct dart_amsgq_impl_data* amsgq)
{
  free(amsgq->dbuf);
  amsgq->dbuf = NULL;
  amsgq->tailpos_ptr = NULL;
  MPI_Win_free(&(amsgq->win));

  dart__base__mutex_destroy(&amsgq->send_mutex);
  dart__base__mutex_destroy(&amsgq->processing_mutex);

  free(amsgq);

  return DART_OK;
}


dart_ret_t dart_amsg_singlewin_init(dart_amsgq_impl_t* impl)
{
  impl->openq   = dart_amsg_singlewin_openq;
  impl->closeq  = dart_amsg_singlewin_closeq;
  impl->trysend = dart_amsg_singlewin_trysend;
  impl->process = dart_amsg_singlewin_process;
  impl->process_blocking = dart_amsg_singlewin_process_blocking;
  return DART_OK;
}
