#include <stdlib.h>
#include <unistd.h>
#include <mpi.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>

#include <dash/dart/base/mutex.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/if/dart_active_messages.h>
#include <dash/dart/if/dart_communication.h>
#include <dash/dart/if/dart_globmem.h>
#include <dash/dart/mpi/dart_team_private.h>
#include <dash/dart/mpi/dart_globmem_priv.h>
#include <dash/dart/mpi/dart_active_messages_priv.h>


struct dart_amsgq_impl_data {
  /// window holding the tail ptr
  MPI_Win      tailpos_win;
  uint64_t    *tailpos_ptr;
  /// window to which active messages are written
  MPI_Win      queue_win;
  char        *queue_ptr;
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

struct dart_amsg_header {
  dart_task_action_t fn;
  dart_global_unit_t remote;
  size_t             data_size;
};

static
dart_ret_t
dart_amsg_dualwin_openq(
  size_t         msg_size,
  size_t         msg_count,
  dart_team_t    team,
  struct dart_amsgq_impl_data** queue)
{
  struct dart_amsgq_impl_data* res = calloc(1, sizeof(*res));
  res->size = msg_count * (sizeof(struct dart_amsg_header) + msg_size);
  res->dbuf = malloc(res->size);
  res->team = team;

  *queue = NULL;

  dart__base__mutex_init(&res->send_mutex);
  dart__base__mutex_init(&res->processing_mutex);

  dart_team_data_t *team_data = dart_adapt_teamlist_get(team);
  if (team_data == NULL) {
    DART_LOG_ERROR("dart_gptr_getaddr ! Unknown team %i", team);
    return DART_ERR_INVAL;
  }
  res->comm = team_data->comm;
//  MPI_Comm_dup(team_data->comm, &res->comm);
  MPI_Comm_rank(res->comm, &res->my_rank);
  /**
   * Allocate the queue
   * We cannot use dart_team_memalloc_aligned because it uses
   * MPI_Win_allocate_shared that cannot be used for window locking.
   */
  MPI_Win_allocate(
    sizeof(uint64_t),
    1,
    MPI_INFO_NULL,
    res->comm,
    (void*)&(res->tailpos_ptr),
    &(res->tailpos_win));
  *(res->tailpos_ptr) = 0;

  MPI_Win_allocate(
    res->size,
    1,
    MPI_INFO_NULL,
    res->comm,
    (void*)&(res->queue_ptr),
    &(res->queue_win));
  memset(res->queue_ptr, 0, res->size);

  MPI_Barrier(res->comm);

  *queue = res;

  return DART_OK;
}

static
dart_ret_t
dart_amsg_dualwin_trysend(
  dart_team_unit_t              target,
  struct dart_amsgq_impl_data * amsgq,
  dart_task_action_t            fn,
  const void                  * data,
  size_t                        data_size)
{
  dart_global_unit_t unitid;
  uint64_t msg_size = (sizeof(struct dart_amsg_header) + data_size);
  uint64_t remote_offset = 0;

  dart__base__mutex_lock(&amsgq->send_mutex);

  dart_task_action_t remote_fn_ptr = fn;
  dart_myid(&unitid);

  //lock the tailpos window
  MPI_Win_lock(MPI_LOCK_EXCLUSIVE, target.id, 0, amsgq->tailpos_win);

  // Add the size of the message to the tailpos at the target
  if (MPI_Fetch_and_op(
        &msg_size,
        &remote_offset,
        MPI_UINT64_T,
        target.id,
        0,
        MPI_SUM,
        amsgq->tailpos_win) != MPI_SUCCESS) {
    DART_LOG_ERROR("MPI_Fetch_and_op failed!");
    dart__base__mutex_unlock(&amsgq->send_mutex);
    return DART_ERR_NOTINIT;
  }

  MPI_Win_flush(target.id, amsgq->tailpos_win);

  DART_LOG_TRACE("MPI_Fetch_and_op returned offset %lu at unit %i",
                 remote_offset, target.id);

  if (remote_offset >= amsgq->size) {
    DART_LOG_ERROR("Received offset larger than message queue size from "
                   "unit %i (%lu but expected < %lu)",
                   target.id, remote_offset, amsgq->size);
    dart__base__mutex_unlock(&amsgq->send_mutex);
    return DART_ERR_INVAL;
  }

  if ((remote_offset + msg_size) >= amsgq->size) {
    uint64_t tmp;
    // if not, revert the operation and free the lock to try again.
    MPI_Fetch_and_op(
      &remote_offset,
      &tmp,
      MPI_UINT64_T,
      target.id,
      0,
      MPI_REPLACE,
      amsgq->tailpos_win);
    MPI_Win_unlock(target.id, amsgq->tailpos_win);
    DART_LOG_TRACE("Not enough space for message of size %i at unit %i "
                   "(current offset %llu of %llu)",
                   msg_size, target.id, remote_offset, amsgq->size);
    dart__base__mutex_unlock(&amsgq->send_mutex);
    return DART_ERR_AGAIN;
  }

  MPI_Win_lock(MPI_LOCK_EXCLUSIVE, target.id, 0, amsgq->queue_win);
  MPI_Win_unlock(target.id, amsgq->tailpos_win);

  struct dart_amsg_header header;
  header.remote = unitid;
  header.fn = remote_fn_ptr;
  header.data_size = data_size;

  // we now have a slot in the message queue
  MPI_Aint queue_disp = remote_offset;
  MPI_Put(
      &header,
      sizeof(header),
      MPI_BYTE,
      target.id,
      queue_disp, sizeof
      (header),
      MPI_BYTE,
      amsgq->queue_win);
  queue_disp += sizeof(header);
  MPI_Put(
    data,
    data_size,
    MPI_BYTE,
    target.id,
    queue_disp,
    data_size,
    MPI_BYTE,
    amsgq->queue_win);
  MPI_Win_unlock(target.id, amsgq->queue_win);

  dart__base__mutex_unlock(&amsgq->send_mutex);

  DART_LOG_INFO("Sent message of size %zu with payload %zu to unit "
                "%i starting at offset %li",
                    msg_size, data_size, target.id, remote_offset);

  return DART_OK;
}

static dart_ret_t
amsg_process_internal(
  struct dart_amsgq_impl_data * amsgq,
  bool                          blocking)
{
  uint64_t         tailpos;

  // trigger progress
  int flag;
  MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, amsgq->comm, &flag, MPI_STATUS_IGNORE);

  if (!blocking) {
    // shortcut if there is nothing to do
    // if (*amsgq->tailpos_ptr == 0) return DART_OK;
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
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, amsgq->my_rank, 0, amsgq->tailpos_win);

    // query local tail position
    MPI_Get(
        &tailpos,
        1,
        MPI_UINT64_T,
        amsgq->my_rank,
        0,
        1,
        MPI_UINT64_T,
        amsgq->tailpos_win);

    // MPI_Win_flush_local should be sufficient but hangs in OMPI 2.1.1
    MPI_Win_flush(amsgq->my_rank, amsgq->tailpos_win);

    if (tailpos > 0) {
      uint64_t   zero = 0;
      DART_LOG_INFO("Checking for new active messages (tailpos=%i)", tailpos);
      // lock the tailpos window
      MPI_Win_lock(MPI_LOCK_EXCLUSIVE, amsgq->my_rank, 0, amsgq->queue_win);
      // copy the content of the queue for processing
      MPI_Get(
          dbuf,
          tailpos,
          MPI_BYTE,
          amsgq->my_rank,
          0,
          tailpos,
          MPI_BYTE,
          amsgq->queue_win);
      MPI_Win_unlock(amsgq->my_rank, amsgq->queue_win);

      // reset the tailpos and release the lock on the message queue
      MPI_Put(
          &zero,
          1,
          MPI_INT64_T,
          amsgq->my_rank,
          0,
          1,
          MPI_INT64_T,
          amsgq->tailpos_win);
      MPI_Win_unlock(amsgq->my_rank, amsgq->tailpos_win);

      // process the messages by invoking the functions on the data supplied
      uint64_t pos = 0;
      while (pos < tailpos) {
  #ifdef DART_ENABLE_LOGGING
        int startpos = pos;
  #endif
        // unpack the message
        struct dart_amsg_header *header =
                                    (struct dart_amsg_header *)(dbuf + pos);
        pos += sizeof(struct dart_amsg_header);
        void *data     = dbuf + pos;
        pos += header->data_size;

        if (pos > tailpos) {
          DART_LOG_ERROR("Message out of bounds (expected %lu but saw %lu)\n",
                         tailpos,
                         pos);
          dart__base__mutex_unlock(&amsgq->processing_mutex);
          return DART_ERR_INVAL;
        }

        // invoke the message
        DART_LOG_INFO("Invoking active message %p from %i on data %p of "
                      "size %i starting from tailpos %i",
                      header->fn,
                      header->remote.id,
                      data,
                      header->data_size,
                      startpos);
        header->fn(data);
      }
    } else {
      MPI_Win_unlock(amsgq->my_rank, amsgq->tailpos_win);
    }
  } while (blocking && tailpos > 0);
  dart__base__mutex_unlock(&amsgq->processing_mutex);
  return DART_OK;
}

static
dart_ret_t
dart_amsg_dualwin_process(struct dart_amsgq_impl_data *amsgq)
{
  return amsg_process_internal(amsgq, false);
}

static
dart_ret_t
dart_amsg_dualwin_process_blocking(struct dart_amsgq_impl_data * amsgq, dart_team_t team)
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
    amsg_process_internal(amsgq, true);
    MPI_Test(&req, &flag, MPI_STATUSES_IGNORE);
  } while (!flag);
  amsg_process_internal(amsgq, true);
  MPI_Barrier(team_data->comm);
  return DART_OK;
}

static
dart_ret_t
dart_amsg_dualwin_closeq(struct dart_amsgq_impl_data *amsgq)
{
  free(amsgq->dbuf);
  amsgq->dbuf = NULL;
  amsgq->queue_ptr = NULL;
  MPI_Win_free(&(amsgq->tailpos_win));
  MPI_Win_free(&(amsgq->queue_win));

//  MPI_Comm_free(&amsgq->comm);

  dart__base__mutex_destroy(&amsgq->send_mutex);
  dart__base__mutex_destroy(&amsgq->processing_mutex);

  free(amsgq);
  return DART_OK;
}


dart_ret_t
dart_amsg_dualwin_init(dart_amsgq_impl_t* impl)
{
  impl->openq   = dart_amsg_dualwin_openq;
  impl->closeq  = dart_amsg_dualwin_closeq;
  impl->bsend   = NULL;
  impl->trysend = dart_amsg_dualwin_trysend;
  impl->flush   = NULL;
  impl->process = dart_amsg_dualwin_process;
  impl->process_blocking = dart_amsg_dualwin_process_blocking;
  return DART_OK;
}

