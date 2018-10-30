#include <stdlib.h>
#include <unistd.h>
#include <mpi.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>

#include <dash/dart/base/mutex.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/base/assert.h>
#include <dash/dart/if/dart_active_messages.h>
#include <dash/dart/if/dart_communication.h>
#include <dash/dart/if/dart_globmem.h>
#include <dash/dart/mpi/dart_team_private.h>
#include <dash/dart/mpi/dart_globmem_priv.h>
#include <dash/dart/mpi/dart_active_messages_priv.h>

#define OFFSET_TAILPOS  0
#define OFFSET_READYPOS sizeof(int64_t)
#define OFFSET_DATA     2*sizeof(int64_t)

typedef struct cached_message_s cached_message_t;

struct dart_amsgq_impl_data {
  MPI_Win           queue_win;
  void             *queue_ptr;
  int64_t           queue_size;
  MPI_Comm          comm;
  dart_mutex_t      send_mutex;
  dart_mutex_t      processing_mutex;
  dart_mutex_t      cache_mutex;
  cached_message_t *message_cache;
  void             *process_buffer;
};

struct dart_amsg_header {
  dart_task_action_t fn;
  dart_global_unit_t remote;
  uint32_t           data_size;
};

struct cached_message_s
{
  cached_message_t       *next;
  dart_team_unit_t        target;
  struct dart_amsg_header header;  // header containing function and data-size
  char                    data[];  // the data
};

static dart_ret_t
dart_amsg_atomic_openq(
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
  res->queue_size =
      msg_count * (sizeof(struct dart_amsg_header) + msg_size);
  MPI_Comm_dup(team_data->comm, &res->comm);
  res->process_buffer = malloc(res->queue_size);

  //printf("Allocating queue with queue-size %zu\n", res->queue_size);

  size_t win_size = res->queue_size + 2*sizeof(int64_t);

  dart__base__mutex_init(&res->send_mutex);
  dart__base__mutex_init(&res->processing_mutex);
  dart__base__mutex_init(&res->cache_mutex);

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

  MPI_Win_lock_all(0, res->queue_win);

  MPI_Barrier(team_data->comm);

  *queue = res;

  return DART_OK;
}


static
dart_ret_t
dart_amsg_atomic_sendbuf(
  dart_team_unit_t              target,
  struct dart_amsgq_impl_data * amsgq,
  const void                  * data,
  size_t                        data_size)
{

  dart__base__mutex_lock(&amsgq->send_mutex);

  DART_LOG_DEBUG("dart_amsg_trysend: u:%i ds:%zu",
                 target.id, data_size);

  int64_t msg_size = data_size;
  int64_t offset;

  do {

    // 1. atomically fetch and update the writer offset
    MPI_Fetch_and_op(
      &msg_size,
      &offset,
      MPI_INT64_T,
      target.id,
      OFFSET_TAILPOS,
      MPI_SUM,
      amsgq->queue_win);
    MPI_Win_flush_local(target.id, amsgq->queue_win);

    printf("msg_size %ld, offset %ld queue_size %ld\n", msg_size, offset, amsgq->queue_size);

    if (offset >= 0 && (offset + msg_size) <= amsgq->queue_size) break;
    else {
      // deregister our failed writing attempt
      int64_t tmp;
      int64_t neg_msg_size = -msg_size;
      MPI_Fetch_and_op(
        &neg_msg_size,
        &tmp,
        MPI_INT64_T,
        target.id,
        OFFSET_TAILPOS,
        MPI_SUM,
        amsgq->queue_win);
      MPI_Win_flush(target.id, amsgq->queue_win);
      printf("Deregistered failed writing attempt: neg_msg_size %ld, offset %ld, tmp %ld\n",
             neg_msg_size, offset, tmp);

      do {
        // the queue is full, wait for it to be cleared
        MPI_Fetch_and_op(
          NULL,
          &offset,
          MPI_INT64_T,
          target.id,
          OFFSET_TAILPOS,
          MPI_NO_OP,
          amsgq->queue_win);
        MPI_Win_flush_local(target.id, amsgq->queue_win);
      } while (offset < 0 || (offset + msg_size) > amsgq->queue_size);
    }

  } while (1);

  DART_LOG_TRACE("MPI_Fetch_and_op returned offset %ld at unit %i",
                 offset, target.id);

  printf("  MPI_Fetch_and_op returned offset %ld at unit %i\n", offset, target.id);

  // 4. Write our payload

  MPI_Put(
    data,
    data_size,
    MPI_BYTE,
    target.id,
    OFFSET_DATA+offset,
    data_size,
    MPI_BYTE,
    amsgq->queue_win);
  // we have to flush here because MPI has no ordering guarantees
  MPI_Win_flush(target.id, amsgq->queue_win);

  // signal completion
  MPI_Accumulate(&msg_size, 1, MPI_INT64_T, target.id,
                 OFFSET_READYPOS, 1, MPI_INT64_T, MPI_SUM, amsgq->queue_win);
  // remote flush required, otherwise the message might never make it through
  MPI_Win_flush(target.id, amsgq->queue_win);

  dart__base__mutex_unlock(&amsgq->send_mutex);

  DART_LOG_INFO("Sent message of size %zu with payload %zu to unit "
                "%d starting at offset %ld",
                msg_size, data_size, target.id, offset);

  return DART_OK;
}

static
dart_ret_t
dart_amsg_atomic_trysend(
  dart_team_unit_t              target,
  struct dart_amsgq_impl_data * amsgq,
  dart_task_action_t            fn,
  const void                  * data,
  size_t                        data_size)
{
  dart_global_unit_t unitid;

  dart__base__mutex_lock(&amsgq->send_mutex);

  dart_myid(&unitid);

  int64_t offset;
  int64_t msg_size = (sizeof(struct dart_amsg_header) + data_size);

  do {

    // 1. atomically fetch and update the writer offset
    MPI_Fetch_and_op(
      &msg_size,
      &offset,
      MPI_INT64_T,
      target.id,
      OFFSET_TAILPOS,
      MPI_SUM,
      amsgq->queue_win);
    MPI_Win_flush_local(target.id, amsgq->queue_win);

    if (offset >= 0 && (offset + msg_size) <= amsgq->queue_size) break;
    else {
      // deregister our failed writing attempt
      int64_t tmp;
      int64_t neg_msg_size = -msg_size;
      MPI_Fetch_and_op(
        &neg_msg_size,
        &tmp,
        MPI_INT64_T,
        target.id,
        OFFSET_TAILPOS,
        MPI_SUM,
        amsgq->queue_win);
      MPI_Win_flush(target.id, amsgq->queue_win);
      printf("Deregistered failed writing attempt: neg_msg_size %ld, offset %ld, tmp %ld\n",
             neg_msg_size, offset, tmp);

      do {
        // the queue is full, wait for it to be cleared
        MPI_Fetch_and_op(
          NULL,
          &offset,
          MPI_INT64_T,
          target.id,
          OFFSET_TAILPOS,
          MPI_NO_OP,
          amsgq->queue_win);
        MPI_Win_flush_local(target.id, amsgq->queue_win);
      } while (offset < 0 || (offset + msg_size) > amsgq->queue_size);
    }

  } while (1);

  DART_LOG_TRACE("MPI_Fetch_and_op returned offset %ld at unit %i",
                 offset, target.id);

  printf("  MPI_Fetch_and_op returned offset %ld at unit %i\n", offset, target.id);

  // write our payload
  struct dart_amsg_header header;
  header.remote    = unitid;
  header.fn        = fn;
  header.data_size = data_size;
  MPI_Put(
    &header,
    sizeof(header),
    MPI_BYTE,
    target.id,
    OFFSET_DATA+offset,
    sizeof(header),
    MPI_BYTE,
    amsgq->queue_win);
  offset += sizeof(header);
  MPI_Put(
    data,
    data_size,
    MPI_BYTE,
    target.id,
    OFFSET_DATA+offset,
    data_size,
    MPI_BYTE,
    amsgq->queue_win);

  // we have to flush here because MPI has no ordering guarantees
  MPI_Win_flush(target.id, amsgq->queue_win);

  // signal completion
  MPI_Accumulate(&msg_size, 1, MPI_INT64_T, target.id,
                 OFFSET_READYPOS, 1, MPI_INT64_T, MPI_SUM, amsgq->queue_win);
  // local flush is sufficient, just make sure we can return
  MPI_Win_flush(target.id, amsgq->queue_win);

  dart__base__mutex_unlock(&amsgq->send_mutex);

  DART_LOG_INFO("Sent message of size %zu with payload %zu to unit "
                "%d starting at offset %ld",
                msg_size, data_size, target.id, offset);

  return DART_OK;
}



static dart_ret_t
amsg_process_nolock_internal(
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

    //printf("Reading from queue %i\n", queuenum);

    //check whether there are active messages available
    MPI_Fetch_and_op(
      NULL,
      &tailpos,
      MPI_INT64_T,
      unitid,
      OFFSET_TAILPOS,
      MPI_NO_OP,
      amsgq->queue_win);
    MPI_Win_flush_local(unitid, amsgq->queue_win);

    DART_ASSERT(tailpos >= 0);

    if (tailpos > 0) {

      int64_t tmp;

      // set the tailpos to INT_MIN to signal the start of processing
      const int64_t intmin = INT32_MIN;
      int64_t readypos = 0;
      MPI_Fetch_and_op(
        &intmin,
        &tailpos,
        MPI_INT64_T,
        unitid,
        OFFSET_TAILPOS,
        MPI_REPLACE,
        amsgq->queue_win);
      MPI_Win_flush(unitid, amsgq->queue_win);

      int64_t first_tailpos = tailpos;
      int64_t final_neg_tailpos;

      // TODO this may be a dealbreaker here! How do we know that we have read a consistent tailpos?
      //      Other processes may continue to increase/decrease the tailpos
      do {
        MPI_Fetch_and_op(
          NULL,
          &tmp,
          MPI_INT64_T,
          unitid,
          OFFSET_TAILPOS,
          MPI_NO_OP,
          amsgq->queue_win);

        MPI_Fetch_and_op(
          NULL,
          &readypos,
          MPI_INT64_T,
          unitid,
          OFFSET_READYPOS,
          MPI_NO_OP,
          amsgq->queue_win);
        MPI_Win_flush_local(unitid, amsgq->queue_win);
        tailpos = first_tailpos + tmp - INT32_MIN;
        final_neg_tailpos = -tmp;
        printf("tailpos %ld, tmp-INT32_MIN %ld, readypos %ld, final_neg_tailpos %ld\n",
               tailpos, tmp-INT32_MIN, readypos, final_neg_tailpos);
      } while (readypos != tailpos);

      DART_ASSERT(tailpos > 0 && tailpos <= amsgq->queue_size);

      // copy the data out of the window
      memcpy(amsgq->process_buffer,
             (void*)((intptr_t)amsgq->queue_ptr+2*sizeof(int64_t)),
             tailpos);

      printf("Done copying tailpos %ld bytes from window\n", tailpos);

      // release the window
      const int64_t zero = 0;
      const int64_t neg_readypos = -readypos;

      // reset the ready-pos
      MPI_Accumulate(&neg_readypos, 1, MPI_INT64_T, unitid,
                    OFFSET_READYPOS, 1, MPI_INT64_T, MPI_SUM, amsgq->queue_win);

      /*
       * TODO: we cannot just add a value to get to zero as some other process
       *       may have attempted to write in the meantime
      MPI_Fetch_and_op(
        &final_neg_tailpos,
        &tmp,
        MPI_INT64_T,
        unitid.id,
        OFFSET_TAILPOS,
        MPI_SUM,
        amsgq->queue_win);
        */

      // TODO: This is problematic for two reasons:
      // a) MPI only allows for same_op_no_op so we cannot mix MPI_Sum and CAS
      // b) it is not clear how long this will take if multiple processes keep
      //    trying to send us data
      int64_t expected_tailpos = tmp;
      do {

        MPI_Compare_and_swap(&zero, &expected_tailpos, &tmp, MPI_INT64_T,
                             unitid, OFFSET_TAILPOS, amsgq->queue_win);

        MPI_Win_flush(unitid, amsgq->queue_win);
      } while (expected_tailpos != tmp);

      printf("Released queue for writing final_neg_tailpos %ld, tmp %ld\n",
             final_neg_tailpos, tmp);

      DART_ASSERT(tmp == -final_neg_tailpos);

      // process the messages by invoking the functions on the data supplied
      uint64_t pos      = 0;
      int      num_msg  = 0;
      void    *dbuf     = amsgq->process_buffer;

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

        DART_ASSERT_MSG(pos <= tailpos,
                        "Message out of bounds (expected %ld but saw %lu)\n",
                         tailpos, pos);

        // invoke the message
        DART_LOG_INFO("Invoking active message %p from %i on data %p of "
                      "size %i starting from tailpos %i",
                      header->fn,
                      header->remote.id,
                      data,
                      header->data_size,
                      startpos);
        header->fn(data);
        num_msg++;
      }
    }
  } while (blocking && tailpos > 0);
  dart__base__mutex_unlock(&amsgq->processing_mutex);
  return DART_OK;
}

static
dart_ret_t
dart_amsg_atomic_process(struct dart_amsgq_impl_data * amsgq)
{
  return amsg_process_nolock_internal(amsgq, false);
}

static
dart_ret_t
dart_amsg_atomic_flush_buffer(struct dart_amsgq_impl_data * amsgq)
{
  char *msgbuf = NULL;
  size_t msgbuf_size = 0;
  dart__base__mutex_lock(&amsgq->cache_mutex);
  while (amsgq->message_cache != NULL) {
    // start from the front and accumulate all messages to the same unit
    cached_message_t *msg = amsgq->message_cache;
    dart_team_unit_t target = msg->target;
    // first, count number of messages to this target
    int cnt = 0;
    size_t data_size = 0;
    for (cached_message_t *m = msg; m != NULL; m = m->next) {
      if (m->target.id == target.id) {
        cnt++;
        data_size += m->header.data_size;
      }
    }
    // allocate a suitable buffer
    size_t total_size = data_size + cnt * sizeof(struct dart_amsg_header);
    if (msgbuf_size < total_size) {
      free(msgbuf);
      msgbuf = malloc(total_size);
      msgbuf_size = total_size;
    }

    // copy messages into the buffer and delete them from the list
    cached_message_t *prev = NULL;
    size_t pos = 0;
    while (msg != NULL) {
      cached_message_t *next = msg->next;
      if (msg->target.id == target.id) {
        size_t to_copy = sizeof(struct dart_amsg_header) + msg->header.data_size;
        if (pos + to_copy > amsgq->queue_size) {
          // break if the buffer would not fit into the queue on the other side
          break;
        }
        memcpy(&msgbuf[pos], &msg->header, to_copy);
        pos += to_copy;
        if (prev == NULL) {
          amsgq->message_cache = msg->next;
        } else {
          prev->next = msg->next;
        }
        free(msg);
      } else {
        prev = msg;
      }
      msg = next;
    }

    //printf("Flushing %d messages (%zu) to target %d\n", cnt, pos, target.id);

    // send out the buffer at once, to one target at a time
    // TODO: can we overlap this somehow?
    dart_ret_t ret;
    do {
      ret = dart_amsg_atomic_sendbuf(target, amsgq, msgbuf, pos);
      if (DART_ERR_AGAIN == ret) {
        // try to process our messages while waiting for the other side
        amsg_process_nolock_internal(amsgq, false);
      } else if (ret != DART_OK) {
        DART_LOG_ERROR("Failed to flush message cache!");
        free(msgbuf);
        return ret;
      }
    } while (ret != DART_OK);
  }
  dart__base__mutex_unlock(&amsgq->cache_mutex);

  free(msgbuf);
  return DART_OK;
}

static
dart_ret_t
dart_amsg_atomic_process_blocking(
  struct dart_amsgq_impl_data * amsgq, dart_team_t team)
{
  int         flag = 0;
  MPI_Request req;
  // flush our buffer
  dart_amsg_atomic_flush_buffer(amsgq);

  // keep processing until all incoming messages have been dealt with
  MPI_Ibarrier(amsgq->comm, &req);
  do {
    amsg_process_nolock_internal(amsgq, true);
    MPI_Test(&req, &flag, MPI_STATUSES_IGNORE);
  } while (!flag);
  amsg_process_nolock_internal(amsgq, true);
  MPI_Barrier(amsgq->comm);
  return DART_OK;
}


static dart_ret_t
dart_amsg_atomic_bsend(
  dart_team_unit_t              target,
  struct dart_amsgq_impl_data * amsgq,
  dart_task_action_t            fn,
  const void                  * data,
  size_t                        data_size)
{
  cached_message_t *msg = malloc(sizeof(*msg) + data_size);
  msg->header.data_size = data_size;
  msg->header.fn        = fn;
  msg->target           = target;
  dart_myid(&msg->header.remote);
  memcpy(msg->data, data, data_size);
  dart__base__mutex_lock(&amsgq->cache_mutex);
  msg->next            = amsgq->message_cache;
  amsgq->message_cache = msg;
  dart__base__mutex_unlock(&amsgq->cache_mutex);
  return DART_OK;
}

static dart_ret_t
dart_amsg_atomic_closeq(struct dart_amsgq_impl_data* amsgq)
{
  // check for late messages
  uint32_t tailpos;
  int      unitid;
  MPI_Comm_rank(amsgq->comm, &unitid);
  MPI_Fetch_and_op(
    NULL,
    &tailpos,
    MPI_INT32_T,
    unitid,
    0,
    MPI_NO_OP,
    amsgq->queue_win);
  MPI_Win_flush_local(unitid, amsgq->queue_win);
  if (tailpos > 0) {
    DART_LOG_WARN("Cowardly refusing to invoke unhandled incoming active "
                  "messages upon shutdown (tailpos %d)!", tailpos);
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
dart_amsg_atomic_init(dart_amsgq_impl_t* impl)
{
  impl->openq   = dart_amsg_atomic_openq;
  impl->closeq  = dart_amsg_atomic_closeq;
  impl->bsend   = dart_amsg_atomic_bsend;
  impl->trysend = dart_amsg_atomic_trysend;
  impl->flush   = dart_amsg_atomic_flush_buffer;
  impl->process = dart_amsg_atomic_process;
  impl->process_blocking = dart_amsg_atomic_process_blocking;
  return DART_OK;
}
