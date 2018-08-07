#include <stdlib.h>
#include <unistd.h>
#include <mpi.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>

#include <dash/dart/base/mutex.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/base/assert.h>
#include <dash/dart/if/dart_active_messages.h>
#include <dash/dart/if/dart_communication.h>
#include <dash/dart/if/dart_globmem.h>
#include <dash/dart/mpi/dart_team_private.h>
#include <dash/dart/mpi/dart_globmem_priv.h>
#include <dash/dart/mpi/dart_active_messages_priv.h>


// 100*512B (512KB) receives posted by default
//#define DEFAULT_MSG_SIZE 256
//#define NUM_MSG 100

static int amsgq_mpi_tag = 10001;

// on MPICH and it's derivatives ISSEND seems broken
#ifdef MPICH
#define IS_ISSEND_BROKEN 1
#endif // MPICH


struct dart_amsgq_impl_data {
  MPI_Request *recv_reqs;
  char *      *recv_bufs;
  MPI_Request *send_reqs;
  char *      *send_bufs;
  int         *recv_outidx;
  int         *send_outidx;
  int          send_tailpos;
  size_t       msg_size;
  int          msg_count;
  dart_team_t  team;
  MPI_Comm     comm;
  dart_mutex_t send_mutex;
  dart_mutex_t processing_mutex;
  int          my_rank;
  int          tag;
};

struct dart_amsg_header {
  dart_task_action_t fn;
  dart_global_unit_t remote;
  uint32_t           data_size;
};

static dart_ret_t
amsgq_test_sendreqs_unsafe(struct dart_amsgq_impl_data* amsgq)
{
  int outcount;
  MPI_Testsome(
    amsgq->send_tailpos, amsgq->send_reqs, &outcount,
    amsgq->send_outidx, MPI_STATUSES_IGNORE);
  DART_LOG_TRACE("  MPI_Testsome: send_tailpos %d, outcount %d",
                 amsgq->send_tailpos, outcount);
  if (outcount > 0) {
    if (outcount == amsgq->send_tailpos) {
      // all messages have finished --> nothing to be done
      amsgq->send_tailpos = 0;
      DART_LOG_TRACE("  All send messages finished");
    } else {
#if 0
      printf("  Finished requests: ");
      for (int outidx = 0; outidx < outcount; ++outidx) {
        printf("%d ", amsgq->send_outidx[outidx]);
      }
      printf("\n");
#endif
      // move requests from the back to the free spots
      int back_pos = amsgq->msg_count - 1;
      for (int outidx = 0; outidx < outcount; ++outidx) {
        int done_idx = amsgq->send_outidx[outidx];
        DART_LOG_TRACE("  outidx %d -> done_idx %d", outidx, done_idx);
        while (back_pos > done_idx &&
                amsgq->send_reqs[back_pos] == MPI_REQUEST_NULL) back_pos--;
        if (done_idx >= back_pos) {
          // we met in the middle
          break;
        }

        DART_LOG_TRACE("  Moving back_pos %d to done_idx %d", back_pos, done_idx);
        // copy the request and buffer to the front
        amsgq->send_reqs[done_idx]  = amsgq->send_reqs[back_pos];
        amsgq->send_reqs[back_pos] = MPI_REQUEST_NULL;
        char *tmp = amsgq->send_bufs[done_idx];
        amsgq->send_bufs[done_idx]  = amsgq->send_bufs[back_pos];
        amsgq->send_bufs[back_pos] = tmp;
        back_pos--;
      }
      amsgq->send_tailpos -= outcount;
    }
    DART_LOG_TRACE("  send_tailpos: %d", amsgq->send_tailpos);
  } else {
    // come back later
    return DART_ERR_AGAIN;
  }
  return DART_OK;
}

static
dart_ret_t
dart_amsg_sendrecv_openq(
  size_t         msg_size,
  size_t         msg_count,
  dart_team_t    team,
  struct dart_amsgq_impl_data** queue)
{
  *queue = NULL;

  dart_team_data_t *team_data = dart_adapt_teamlist_get(team);
  if (team_data == NULL) {
    DART_LOG_ERROR("dart_gptr_getaddr ! Unknown team %i", team);
    return DART_ERR_INVAL;
  }
  struct dart_amsgq_impl_data* res = calloc(1, sizeof(*res));
  res->team = team;
  res->comm = team_data->comm;
  dart__base__mutex_init(&res->send_mutex);
  dart__base__mutex_init(&res->processing_mutex);
  res->msg_size = sizeof(struct dart_amsg_header) + msg_size;
  res->msg_count   = msg_count;
  res->send_bufs   = malloc(msg_count*sizeof(*res->send_bufs));
  res->send_reqs   = malloc(msg_count*sizeof(*res->send_reqs));
  res->recv_bufs   = malloc(msg_count*sizeof(*res->recv_bufs));
  res->recv_reqs   = malloc(msg_count*sizeof(*res->recv_reqs));
  res->recv_outidx = malloc(msg_count*sizeof(*res->recv_outidx));
  res->send_outidx = malloc(msg_count*sizeof(*res->send_outidx));
  MPI_Comm_dup(team_data->comm, &res->comm);
  MPI_Comm_rank(res->comm, &res->my_rank);

  res->tag = amsgq_mpi_tag++;

  // post receives
  for (int i = 0; i < msg_count; ++i) {
    res->recv_bufs[i] = malloc(res->msg_size);
    MPI_Irecv(
      res->recv_bufs[i],
      res->msg_size,
      MPI_BYTE,
      MPI_ANY_SOURCE,
      res->tag,
      res->comm,
      &res->recv_reqs[i]);
    res->send_bufs[i] = malloc(res->msg_size);
    res->send_reqs[i] = MPI_REQUEST_NULL;
  }

  MPI_Barrier(res->comm);

  *queue = res;

  return DART_OK;
}

static
dart_ret_t
dart_amsg_sendrecv_trysend(
  dart_team_unit_t              target,
  struct dart_amsgq_impl_data * amsgq,
  dart_task_action_t            fn,
  const void                  * data,
  size_t                        data_size)
{
  dart_global_unit_t unitid;
  uint64_t msg_size = sizeof(struct dart_amsg_header) + data_size;

  DART_ASSERT(msg_size <= amsgq->msg_size);

  dart__base__mutex_lock(&amsgq->send_mutex);

  dart_task_action_t remote_fn_ptr = fn;

  dart_myid(&unitid);


  DART_ASSERT(amsgq->send_tailpos <= amsgq->msg_count);

  // search for a free handle if necessary
  if (amsgq->send_tailpos == amsgq->msg_count) {
    int ret = amsgq_test_sendreqs_unsafe(amsgq);
    if (ret != DART_OK) {
      dart__base__mutex_unlock(&amsgq->send_mutex);
      return ret;
    }
  }
  int idx = amsgq->send_tailpos++;
  DART_LOG_TRACE("Send request idx: %d", idx);

  struct dart_amsg_header header;
  header.remote = unitid;
  header.fn = remote_fn_ptr;
  header.data_size = data_size;

  char *sendbuf = amsgq->send_bufs[idx];

  memcpy(sendbuf, &header, sizeof(header));
  memcpy(sendbuf + sizeof(header), data, data_size);

  // TODO: Issend seems broken on IMPI :(
#ifndef IS_ISSEND_BROKEN
  int ret = MPI_Issend(
#else
  int ret = MPI_Isend(
#endif // IS_ISSEND_BROKEN
              sendbuf, amsgq->msg_size,
              MPI_BYTE, target.id, amsgq->tag, amsgq->comm,
              &amsgq->send_reqs[idx]);

  dart__base__mutex_unlock(&amsgq->send_mutex);

  if (ret != MPI_SUCCESS) {
    DART_LOG_ERROR("Failed to send active message to unit %i", target.id);
    return DART_ERR_AGAIN;
  }

  DART_LOG_INFO("Sent message of size %zu with data size %zu to unit %i",
                    amsgq->msg_size, data_size, target.id);

  return DART_OK;
}

static dart_ret_t
amsg_sendrecv_process_internal(
  struct dart_amsgq_impl_data* amsgq,
  bool                         blocking,
  bool                         has_lock)
{
  uint64_t num_msg;

  DART_ASSERT(amsgq != NULL);

  if (!has_lock) {
    if (!blocking) {
      dart_ret_t ret = dart__base__mutex_trylock(&amsgq->processing_mutex);
      if (ret != DART_OK) {
        return DART_ERR_AGAIN;
      }
    } else {
      dart__base__mutex_lock(&amsgq->processing_mutex);
    }
  }
#if 0
  // process outgoing messages if necessary
  if (amsgq->send_tailpos > 0 &&
    dart__base__mutex_trylock(&amsgq->send_mutex) == DART_OK) {
    amsgq_test_sendreqs_unsafe(amsgq);
    dart__base__mutex_unlock(&amsgq->send_mutex);
  }
#endif
  do {
    num_msg = 0;
    int outcount = 0;
    MPI_Testsome(
      amsgq->msg_count, amsgq->recv_reqs, &outcount,
      amsgq->recv_outidx, MPI_STATUSES_IGNORE);
    if (outcount > 0) {
      DART_LOG_INFO("MPI_Testsome: %d/%d incoming messages available\n",
                    outcount, amsgq->msg_count);
    }
    for (int i = 0; i < outcount; ++i) {
      // pick the message
      int   idx  = amsgq->recv_outidx[i];
      char *dbuf = amsgq->recv_bufs[idx];
      // unpack the message
      struct dart_amsg_header *header = (struct dart_amsg_header *)dbuf;
      void                    *data   = dbuf + sizeof(struct dart_amsg_header);

      // invoke the message
      DART_LOG_INFO("Invoking active message %p from %i on data %p of size %d",
                    header->fn,
                    header->remote.id,
                    data,
                    header->data_size);

      header->fn(data);

      // repost the recv
      MPI_Irecv(
        amsgq->recv_bufs[idx], amsgq->msg_size, MPI_BYTE,
        MPI_ANY_SOURCE, amsgq->tag,
        amsgq->comm, &amsgq->recv_reqs[idx]);

      ++num_msg;
    }

    // repeat until we do not find messages anymore
  } while (blocking && num_msg > 0);

  if (!has_lock) {
    dart__base__mutex_unlock(&amsgq->processing_mutex);
  }
  return DART_OK;
}

static
dart_ret_t
dart_amsg_sendrecv_process(struct dart_amsgq_impl_data* amsgq)
{
  return amsg_sendrecv_process_internal(amsgq, false, false);
}

static
dart_ret_t
dart_amsg_sendrevc_process_blocking(
  struct dart_amsgq_impl_data* amsgq,
  dart_team_t                  team)
{
  MPI_Request req = MPI_REQUEST_NULL;

  dart_team_data_t *team_data = dart_adapt_teamlist_get(team);
  if (team_data == NULL) {
    DART_LOG_ERROR("dart_gptr_getaddr ! Unknown team %i", team);
    return DART_ERR_INVAL;
  }

  dart__base__mutex_lock(&amsgq->processing_mutex);

  int         barrier_flag = 0;
  int         send_flag = 0;
  do {
    amsg_sendrecv_process_internal(amsgq, true, true);
    if (req != MPI_REQUEST_NULL) {
      MPI_Test(&req, &barrier_flag, MPI_STATUS_IGNORE);
      if (barrier_flag) {
        DART_LOG_DEBUG("Finished blocking processing of messages!");
      }
    }
    if (!send_flag) {
      dart__base__mutex_lock(&amsgq->send_mutex);
      MPI_Testall(amsgq->send_tailpos, amsgq->send_reqs,
                  &send_flag, MPI_STATUSES_IGNORE);
      dart__base__mutex_unlock(&amsgq->send_mutex);
      if (send_flag) {
        DART_LOG_DEBUG("MPI_Testall: all %d sent active messages completed!",
                       amsgq->send_tailpos);
        amsgq->send_tailpos = 0;
        MPI_Ibarrier(team_data->comm, &req);
      }
    }
  } while (!(barrier_flag && send_flag));
#ifdef IS_ISSEND_BROKEN
  // if Issend is broken we need another round of synchronization
  MPI_Barrier(team_data->comm);
#endif
  amsg_sendrecv_process_internal(amsgq, true, true);
  // final synchronization
  // TODO: I don't think this is needed here!
  //MPI_Barrier(team_data->comm);

  dart__base__mutex_unlock(&amsgq->processing_mutex);
  return DART_OK;
}

static
dart_ret_t
dart_amsg_sendrecv_closeq(struct dart_amsgq_impl_data* amsgq)
{

  MPI_Comm_free(&amsgq->comm);

  if (amsgq->send_tailpos > 0) {
    DART_LOG_INFO("Waiting for %d active messages to complete",
                  amsgq->send_tailpos);
    MPI_Waitall(amsgq->msg_count, amsgq->send_reqs, MPI_STATUSES_IGNORE);
  }

  int outcount = 0;
  MPI_Testsome(
    amsgq->msg_count, amsgq->recv_reqs, &outcount,
    amsgq->recv_outidx, MPI_STATUSES_IGNORE);

  if (outcount) {
    DART_LOG_WARN("Cowardly refusing to invoke %d unhandled incoming active "
                  "messages upon shutdown!", outcount);
  }


  for (int i = 0; i < amsgq->msg_count; ++i) {
    if (amsgq->recv_reqs[i] != MPI_REQUEST_NULL) {
      MPI_Cancel(&amsgq->recv_reqs[i]);
    }
    free(amsgq->recv_bufs[i]);
    free(amsgq->send_bufs[i]);
  }

  free(amsgq->recv_bufs);
  free(amsgq->send_bufs);
  free(amsgq->recv_reqs);
  free(amsgq->send_reqs);
  free(amsgq->recv_outidx);
  free(amsgq->send_outidx);

  dart__base__mutex_destroy(&amsgq->send_mutex);
  dart__base__mutex_destroy(&amsgq->processing_mutex);

  free(amsgq);

  return DART_OK;
}


/**
 * Flush messages that were sent using \c dart_amsg_buffered_send.
 */
static
dart_ret_t
dart_amsg_sendrecv_flush_buffer(struct dart_amsgq_impl_data* amsgq)
{
  dart__base__mutex_lock(&amsgq->send_mutex);
  MPI_Waitall(amsgq->msg_count, amsgq->send_reqs, MPI_STATUSES_IGNORE);
  amsgq->msg_count = 0;
  dart__base__mutex_unlock(&amsgq->send_mutex);
  return DART_OK;
}

/**
 * Buffer the active message until it is sent out using \c dart_amsg_flush_buffer.
 */
static
dart_ret_t
dart_amsg_sendrecv_buffered_send(
  dart_team_unit_t              target,
  struct dart_amsgq_impl_data * amsgq,
  dart_task_action_t            fn,
  const void                  * data,
  size_t                        data_size)
{
  return dart_amsg_sendrecv_trysend(target, amsgq, fn, data, data_size);
}


dart_ret_t dart_amsg_sendrecv_init(dart_amsgq_impl_t* impl)
{
  impl->openq   = dart_amsg_sendrecv_openq;
  impl->closeq  = dart_amsg_sendrecv_closeq;
  impl->bsend   = dart_amsg_sendrecv_buffered_send;
  impl->flush   = dart_amsg_sendrecv_flush_buffer;
  impl->trysend = dart_amsg_sendrecv_trysend;
  impl->process = dart_amsg_sendrecv_process;
  impl->process_blocking = dart_amsg_sendrevc_process_blocking;
  return DART_OK;
}


