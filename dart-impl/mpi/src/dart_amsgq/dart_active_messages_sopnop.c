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


typedef struct cached_message_s cached_message_t;

struct dart_amsgq_impl_data {
  MPI_Win           queue_win;
  void             *queue_ptr;
  uint64_t          queue_size;
  MPI_Comm          comm;
  dart_mutex_t      send_mutex;
  dart_mutex_t      processing_mutex;
  dart_mutex_t      cache_mutex;
  cached_message_t *message_cache;
  int64_t           prev_tailpos;
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

#define OFFSET_QUEUENUM                 0
#define OFFSET_TAILPOS(q)    (sizeof(int64_t)+q*2*sizeof(int64_t))
#define OFFSET_READYPOS(q)   (OFFSET_TAILPOS(q)+sizeof(int64_t))
#define OFFSET_DATA(q, qs)   (OFFSET_READYPOS(1)+sizeof(int64_t)+q*qs)

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
  res->queue_size =
      msg_count * (sizeof(struct dart_amsg_header) + msg_size);
  MPI_Comm_dup(team_data->comm, &res->comm);

  //printf("Allocating queue with queue-size %zu\n", res->queue_size);

  size_t win_size = 2*(res->queue_size + 2*sizeof(int64_t)) + sizeof(int64_t);

  dart__base__mutex_init(&res->send_mutex);
  dart__base__mutex_init(&res->processing_mutex);
  dart__base__mutex_init(&res->cache_mutex);

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

  MPI_Win_lock_all(0, res->queue_win);

  MPI_Barrier(res->comm);

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

  dart__base__mutex_lock(&amsgq->send_mutex);

  DART_LOG_DEBUG("dart_amsg_trysend: u:%i ds:%zu",
                 target.id, data_size);

  int64_t msg_size = data_size;
  int64_t offset;
  int64_t queuenum;

  do {

    // fetch queue number
    MPI_Fetch_and_op(
      NULL,
      &queuenum,
      MPI_INT64_T,
      target.id,
      OFFSET_QUEUENUM,
      MPI_NO_OP,
      amsgq->queue_win);

    // atomically fetch and update the writer offset
    MPI_Fetch_and_op(
      &msg_size,
      &offset,
      MPI_INT64_T,
      target.id,
      OFFSET_TAILPOS(queuenum),
      MPI_SUM,
      amsgq->queue_win);
    MPI_Win_flush_local(target.id, amsgq->queue_win);

    if (offset >= 0 && (offset + msg_size) <= amsgq->queue_size) break;

    // the queue is full, reset the offset
    int64_t neg_msg_size = -msg_size;
    MPI_Accumulate(&neg_msg_size, 1, MPI_INT64_T, target.id,
                   OFFSET_TAILPOS(queuenum), 1, MPI_INT64_T,
                   MPI_SUM, amsgq->queue_win);
    MPI_Win_flush(target.id, amsgq->queue_win);

    dart__base__mutex_unlock(&amsgq->send_mutex);
    return DART_ERR_AGAIN;
  } while (1);

  DART_LOG_TRACE("MPI_Fetch_and_op returned offset %ld at unit %i",
                 offset, target.id);

  // Write our payload

  MPI_Put(
    data,
    data_size,
    MPI_BYTE,
    target.id,
    offset,
    data_size,
    MPI_BYTE,
    amsgq->queue_win);
  // we have to flush here because MPI has no ordering guarantees
  MPI_Win_flush(target.id, amsgq->queue_win);

  DART_LOG_TRACE("Updating readypos in queue %ld at unit %i",
                 queuenum, target.id);

  // signal completion
  MPI_Accumulate(&msg_size, 1, MPI_INT64_T, target.id,
                 OFFSET_READYPOS(queuenum), 1, MPI_INT64_T,
                 MPI_SUM, amsgq->queue_win);
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
dart_amsg_sopnop_trysend(
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
  int64_t queuenum;

  DART_LOG_DEBUG("dart_amsg_trysend: u:%i ds:%zu",
                 target.id, data_size);

  do {

    // fetch queue number
    MPI_Fetch_and_op(
      NULL,
      &queuenum,
      MPI_INT64_T,
      target.id,
      OFFSET_QUEUENUM,
      MPI_NO_OP,
      amsgq->queue_win);

    // atomically fetch and update the writer offset
    MPI_Fetch_and_op(
      &msg_size,
      &offset,
      MPI_INT64_T,
      target.id,
      OFFSET_TAILPOS(queuenum),
      MPI_SUM,
      amsgq->queue_win);
    MPI_Win_flush_local(target.id, amsgq->queue_win);

    if (offset >= 0 && (offset + msg_size) <= amsgq->queue_size) break;

    // the queue is full, reset the offset
    int64_t neg_msg_size = -msg_size;
    MPI_Accumulate(&neg_msg_size, 1, MPI_INT64_T, target.id,
                   OFFSET_TAILPOS(queuenum), 1, MPI_INT64_T,
                   MPI_SUM, amsgq->queue_win);
    MPI_Win_flush(target.id, amsgq->queue_win);

    dart__base__mutex_unlock(&amsgq->send_mutex);
    return DART_ERR_AGAIN;
  } while (1);

  DART_LOG_TRACE("MPI_Fetch_and_op returned offset %ld at unit %i",
                 offset, target.id);

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
    OFFSET_DATA(queuenum, amsgq->queue_size) + offset,
    sizeof(header),
    MPI_BYTE,
    amsgq->queue_win);
  offset += sizeof(header);
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

  DART_LOG_TRACE("Updating readypos in queue %ld at unit %i",
                 queuenum, target.id);

  // signal completion
  MPI_Accumulate(&msg_size, 1, MPI_INT64_T, target.id,
                 OFFSET_READYPOS(queuenum), 1, MPI_INT64_T,
                 MPI_SUM, amsgq->queue_win);
  // remote flush required, otherwise the message might never make it through
  MPI_Win_flush(target.id, amsgq->queue_win);

  dart__base__mutex_unlock(&amsgq->send_mutex);

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

    //printf("Reading from queue %i\n", queuenum);

    //check whether there are active messages available
    MPI_Fetch_and_op(
      NULL,
      &tailpos,
      MPI_INT64_T,
      unitid,
      OFFSET_TAILPOS(queuenum),
      MPI_NO_OP,
      amsgq->queue_win);
    MPI_Win_flush_local(unitid, amsgq->queue_win);


    if (tailpos > 0) {
      DART_LOG_TRACE("Queue %ld has tailpos %ld", queuenum, tailpos);
      const int64_t zero = 0;

      int64_t tmp = 0;
      int64_t newqueue = (queuenum == 0) ? 1 : 0;

      // wait for possible late senders on the new queue to finish
      // NOTE: this is a poor-man's CAS
      do {
        MPI_Fetch_and_op(
          NULL,
          &tmp,
          MPI_INT64_T,
          unitid,
          OFFSET_TAILPOS(newqueue),
          MPI_NO_OP,
          amsgq->queue_win);
        MPI_Win_flush_local(unitid, amsgq->queue_win);
      } while (tmp != amsgq->prev_tailpos);

      // reset tailpos of new queue
      MPI_Fetch_and_op(
        &zero,
        &tmp,
        MPI_INT64_T,
        unitid,
        OFFSET_TAILPOS(newqueue),
        MPI_REPLACE,
        amsgq->queue_win);
      MPI_Win_flush(unitid, amsgq->queue_win);

      // swap the queue number
      MPI_Fetch_and_op(
        &newqueue,
        &tmp,
        MPI_INT64_T,
        unitid,
        OFFSET_QUEUENUM,
        MPI_REPLACE,
        amsgq->queue_win);
      MPI_Win_flush(unitid, amsgq->queue_win);


      // set the tailpos to a large negative number to signal the start of
      // processing
      // Any later attempt to write to this queue will return a negative offset
      // and cause the writer to switch to the new queue
      int64_t readypos = 0;
      int64_t prev_tailpos =  tailpos;
      int64_t tailpos_sub  = -tailpos - INT32_MAX;
      MPI_Fetch_and_op(
        &tailpos_sub,
        &tailpos,
        MPI_INT64_T,
        unitid,
        OFFSET_TAILPOS(queuenum),
        MPI_SUM,
        amsgq->queue_win);

      // NOTE: deferred flush

      // wait for all active writers to finish
      // NOTE: This is a poor-man's CAS
      do {

        MPI_Fetch_and_op(
          NULL,
          &readypos,
          MPI_INT64_T,
          unitid,
          OFFSET_READYPOS(queuenum),
          MPI_NO_OP,
          amsgq->queue_win);

        // we have to requiry the tail pos and possibly adjust it
        int64_t tmp;
        MPI_Fetch_and_op(
          NULL,
          &tmp,
          MPI_INT64_T,
          unitid,
          OFFSET_TAILPOS(queuenum),
          MPI_NO_OP,
          amsgq->queue_win);
        MPI_Win_flush_local(unitid, amsgq->queue_win);

        tailpos = tmp + (-tailpos_sub);

        DART_ASSERT(readypos <= tailpos);

      } while (readypos != tailpos);

      // remember the actual value of tailpos so we can wait for it later
      amsgq->prev_tailpos = tailpos_sub + tailpos;

      // reset readypos
      // NOTE: using MPI_REPLACE here is valid as no-one else will write to it
      //       at this time.
      MPI_Fetch_and_op(
        &zero,
        &readypos,
        MPI_INT64_T,
        unitid,
        OFFSET_READYPOS(queuenum),
        MPI_REPLACE,
        amsgq->queue_win);
      MPI_Win_flush(unitid, amsgq->queue_win);

      // NOTE: deferred flush

      // process the messages by invoking the functions on the data supplied
      uint64_t pos      = 0;
      int      num_msg  = 0;
      void    *dbuf     = (void*)((intptr_t)amsgq->queue_ptr +
                                      OFFSET_DATA(queuenum, amsgq->queue_size));

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
dart_amsg_sopnop_process(struct dart_amsgq_impl_data * amsgq)
{
  return amsg_sopnop_process_internal(amsgq, false);
}

static
dart_ret_t
dart_amsg_sopnop_flush_buffer(struct dart_amsgq_impl_data * amsgq)
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
      ret = dart_amsg_sopnop_sendbuf(target, amsgq, msgbuf, pos);
      if (DART_ERR_AGAIN == ret) {
        // try to process our messages while waiting for the other side
        amsg_sopnop_process_internal(amsgq, false);
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
dart_amsg_sopnop_process_blocking(
  struct dart_amsgq_impl_data * amsgq, dart_team_t team)
{
  int         flag = 0;
  MPI_Request req;

  // flush our buffer
  dart_amsg_sopnop_flush_buffer(amsgq);

  // keep processing until all incoming messages have been dealt with
  MPI_Ibarrier(amsgq->comm, &req);
  do {
    amsg_sopnop_process_internal(amsgq, true);
    MPI_Test(&req, &flag, MPI_STATUSES_IGNORE);
  } while (!flag);
  amsg_sopnop_process_internal(amsgq, true);
  MPI_Barrier(amsgq->comm);
  return DART_OK;
}


static dart_ret_t
dart_amsg_sopnop_bsend(
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
dart_amsg_sopnop_closeq(struct dart_amsgq_impl_data* amsgq)
{
  // check for late messages
  uint32_t tailpos;
  int      unitid;
  int64_t queuenum = *(int64_t*)amsgq->queue_ptr;

  MPI_Comm_rank(amsgq->comm, &unitid);

  MPI_Fetch_and_op(
    NULL,
    &tailpos,
    MPI_INT32_T,
    unitid,
    OFFSET_TAILPOS(queuenum),
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
dart_amsg_sopnop_init(dart_amsgq_impl_t* impl)
{
  impl->openq   = dart_amsg_sopnop_openq;
  impl->closeq  = dart_amsg_sopnop_closeq;
  //impl->bsend   = dart_amsg_sopnop_bsend;
  impl->bsend   = dart_amsg_sopnop_trysend;
  impl->trysend = dart_amsg_sopnop_trysend;
  impl->flush   = dart_amsg_sopnop_flush_buffer;
  impl->process = dart_amsg_sopnop_process;
  impl->process_blocking = dart_amsg_sopnop_process_blocking;
  return DART_OK;
}
