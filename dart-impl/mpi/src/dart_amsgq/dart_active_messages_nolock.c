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
  /*
   * window to which active messages are written
   * the layout is as follows:
   * | < 1 Byte pointer> | <message queue 1> <message queue 2>
   * The 1 Byte pointer is either 0 or 1, depending on whether the first
   * or second message queue is active.
   * Each <message queue has the following layout:
   * | < 4 Byte > | < 4 Byte > | < queue_size Bytes>
   * | <counter>  | < offset > | <messages>...
   *
   * <counter> counts ongoing write accesses to this window
   * <offset>  is the byte-offset of the next free message slot
   * <message> is a set of messages varying sizes
   */
  MPI_Win           queue_win;
  void             *queue_ptr;
  /// the size (in byte) of each message queue
  uint64_t          queue_size;
  dart_team_t       team;
  dart_mutex_t      send_mutex;
  dart_mutex_t      processing_mutex;
  dart_mutex_t      cache_mutex;
  cached_message_t *message_cache;
  char              current_queue;
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
dart_amsg_nolock_openq(
  size_t      msg_size,
  size_t      msg_count,
  dart_team_t team,
  struct dart_amsgq_impl_data** queue)
{
  struct dart_amsgq_impl_data *res = calloc(1, sizeof(struct dart_amsgq_impl_data));
  res->queue_size =
      msg_count * (sizeof(struct dart_amsg_header) + msg_size);
  res->team = team;

  //printf("Allocating queue with queue-size %zu\n", res->queue_size);

  size_t win_size = 2 * (res->queue_size + 2*sizeof(int32_t)) + 1;

  dart__base__mutex_init(&res->send_mutex);
  dart__base__mutex_init(&res->processing_mutex);
  dart__base__mutex_init(&res->cache_mutex);

  dart_team_data_t *team_data = dart_adapt_teamlist_get(team);
  if (team_data == NULL) {
    DART_LOG_ERROR("dart_gptr_getaddr ! Unknown team %i", team);
    return DART_ERR_INVAL;
  }

  /**
   * Allocate the queue
   * We cannot use dart_team_memalloc_aligned because it uses
   * MPI_Win_allocate_shared that cannot be used for window locking.
   */
  MPI_Win_allocate(
    win_size,
    1,
    MPI_INFO_NULL,
    team_data->comm,
    &(res->queue_ptr),
    &(res->queue_win));
  memset(res->queue_ptr, 0, win_size);

  MPI_Win_lock_all(0, res->queue_win);

  MPI_Barrier(team_data->comm);

  *queue = res;

  return DART_OK;
}


static
dart_ret_t
dart_amsg_nolock_sendbuf(
  dart_team_unit_t              target,
  struct dart_amsgq_impl_data * amsgq,
  const void                  * data,
  size_t                        data_size)
{

  dart__base__mutex_lock(&amsgq->send_mutex);

  DART_LOG_DEBUG("dart_amsg_trysend: u:%i t:%i ds:%zu",
                 target.id, amsgq->team, data_size);


  int32_t  writecnt;
  size_t   base_offset;
  const int32_t  increment =  1;
  const int32_t  decrement = -1;
  int32_t msg_size = data_size;
  uint32_t remote_offset = 0;

  do {

    // 1. atomically query the window to use (0 or 1)
    char queue_num;
    MPI_Fetch_and_op(
      NULL,
      &queue_num,
      MPI_BYTE,
      target.id,
      0,
      MPI_NO_OP,
      amsgq->queue_win);
    MPI_Win_flush_local(target.id, amsgq->queue_win);

    //printf("Writing %uB to queue %i at unit %i\n", msg_size, queue_num, target.id);

    base_offset = 1 + queue_num * (amsgq->queue_size + 2*sizeof(int32_t));

    // 2. register as a writer
    MPI_Fetch_and_op(
      &increment,
      &writecnt,
      MPI_INT32_T,
      target.id,
      base_offset,
      MPI_SUM,
      amsgq->queue_win);
    MPI_Win_flush(target.id, amsgq->queue_win);

    //printf("Writing to queue %d: writecnt %d\n", queue_num, writecnt);

    // repeat if the target has swapped windows in between
  } while (writecnt < 0);


  // 3. update offset
  MPI_Fetch_and_op(
    &msg_size,
    &remote_offset,
    MPI_INT32_T,
    target.id,
    base_offset + sizeof(int32_t),
    MPI_SUM,
    amsgq->queue_win);
  MPI_Win_flush(target.id, amsgq->queue_win);
  if (remote_offset + msg_size > amsgq->queue_size) {
    DART_LOG_TRACE("Not enough space for message of size %i at unit %i "
                   "(current offset %u of %lu, writecnt: %i)",
                   msg_size, target.id, remote_offset, amsgq->queue_size,
                    writecnt);
    // deregister
    // TODO: use MPI_Accumulate here
    int32_t neg_msg_size = -msg_size;
    int32_t tmp;
    MPI_Fetch_and_op(
      &neg_msg_size,
      &tmp,
      MPI_INT32_T,
      target.id,
      base_offset + sizeof(int32_t),
      MPI_SUM,
      amsgq->queue_win);
    MPI_Win_flush(target.id, amsgq->queue_win);

    MPI_Fetch_and_op(
      &decrement,
      &writecnt,
      MPI_INT32_T,
      target.id,
      base_offset,
      MPI_SUM,
      amsgq->queue_win);
    MPI_Win_flush(target.id, amsgq->queue_win);
    //printf("  Full queue: remote_offset %d (roll-back to offset %d, witecnt %d)\n", remote_offset, tmp, writecnt);

    dart__base__mutex_unlock(&amsgq->send_mutex);

    // come back later
    return DART_ERR_AGAIN;
  }


  DART_LOG_TRACE("MPI_Fetch_and_op returned offset %u at unit %i",
                 remote_offset, target.id);
  //printf("  MPI_Fetch_and_op returned offset %u (%X) at unit %i\n",
  //               remote_offset, remote_offset, target.id);

  // 4. Write our payload

  size_t offset = base_offset + 2*sizeof(int32_t) + remote_offset;
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

  // 5. Deregister as a writer
  //    Use int64_t here, we cannot perform 64-bit substraction using only
  //    32bit lower part

#if 0
  MPI_Fetch_and_op(
    &dec,
    &atomic_result,
    MPI_UINT64_T,
    target.id,
    base_offset,
    MPI_SUM,
    amsgq->queue_win);
  MPI_Win_flush(target.id, amsgq->queue_win);
  /*printf("  Deregistering as writer: writecnt %d (%X), offset %d (%X) [%p]\n",
         atomic_result.writecnt, atomic_result.writecnt,
         atomic_result.offset, atomic_result.offset, *(void**)&atomic_result);
  */

  // DEBUG
  MPI_Fetch_and_op(
    NULL,
    &atomic_result,
    MPI_UINT64_T,
    target.id,
    base_offset,
    MPI_NO_OP,
    amsgq->queue_win);
  MPI_Win_flush(target.id, amsgq->queue_win);
  //printf("  Exit state: writecnt %d, offset %d [%p]\n", atomic_result.writecnt, atomic_result.offset, *(void**)&atomic_result);
  // DEBUG
#endif

#if 0
  MPI_Fetch_and_op(
    &decrement,
    &writecnt,
    MPI_INT32_T,
    target.id,
    base_offset,
    MPI_SUM,
    amsgq->queue_win);
  MPI_Win_flush(target.id, amsgq->queue_win);
  printf("  Deregistering as writer: writecnt %d\n", writecnt);
#endif

  MPI_Accumulate(&decrement, 1, MPI_INT32_T, target.id,
                 base_offset, 1, MPI_INT32_T, MPI_SUM, amsgq->queue_win);
  // local flush is sufficient, just make sure we can return
  MPI_Win_flush_local(target.id, amsgq->queue_win);

  dart__base__mutex_unlock(&amsgq->send_mutex);

  DART_LOG_INFO("Sent message of size %u with payload %zu to unit "
                "%d starting at offset %u",
                msg_size, data_size, target.id, remote_offset);

  return DART_OK;
}

static
dart_ret_t
dart_amsg_nolock_trysend(
  dart_team_unit_t              target,
  struct dart_amsgq_impl_data * amsgq,
  dart_task_action_t            fn,
  const void                  * data,
  size_t                        data_size)
{
  dart_global_unit_t unitid;

  dart__base__mutex_lock(&amsgq->send_mutex);

  dart_myid(&unitid);

  int32_t  writecnt;
  size_t   base_offset;
  const int32_t  increment =  1;
  const int32_t  decrement = -1;
  int32_t msg_size = (sizeof(struct dart_amsg_header) + data_size);
  uint32_t remote_offset = 0;

  do {

    // 1. atomically query the window to use (0 or 1)
    char queue_num;
    MPI_Fetch_and_op(
      NULL,
      &queue_num,
      MPI_BYTE,
      target.id,
      0,
      MPI_NO_OP,
      amsgq->queue_win);
    MPI_Win_flush_local(target.id, amsgq->queue_win);

    //printf("Writing %uB to queue %i at unit %i\n", msg_size, queue_num, target.id);

    base_offset = 1 + queue_num * (amsgq->queue_size + 2*sizeof(int32_t));

    // 2. register as a writer
    MPI_Fetch_and_op(
      &increment,
      &writecnt,
      MPI_INT32_T,
      target.id,
      base_offset,
      MPI_SUM,
      amsgq->queue_win);
    MPI_Win_flush(target.id, amsgq->queue_win);

    //printf("Writing to queue %d: writecnt %d\n", queue_num, remote_offset);

    /*printf("  Writecnt %i (%X), offset %i (%X) [%p]\n",
           atomic_result.writecnt, atomic_result.writecnt,
           atomic_result.offset, atomic_result.offset, *(void**)&atomic_result);
    */

    // repeat if the target has swapped windows in between
  } while (writecnt < 0);


  // 3. update offset
  MPI_Fetch_and_op(
    &msg_size,
    &remote_offset,
    MPI_INT32_T,
    target.id,
    base_offset + sizeof(int32_t),
    MPI_SUM,
    amsgq->queue_win);
  MPI_Win_flush(target.id, amsgq->queue_win);
  if (remote_offset + msg_size > amsgq->queue_size) {
    DART_LOG_TRACE("Not enough space for message of size %i at unit %i "
                   "(current offset %u of %lu, writecnt: %i)",
                   msg_size, target.id, remote_offset, amsgq->queue_size,
                    writecnt);
    //printf("  Full queue: remote_offset %d\n", remote_offset);
    // deregister
    // TODO: use MPI_Accumulate here
    int32_t neg_msg_size = -msg_size;
    MPI_Fetch_and_op(
      &neg_msg_size,
      &remote_offset,
      MPI_INT32_T,
      target.id,
      base_offset + sizeof(int32_t),
      MPI_SUM,
      amsgq->queue_win);
    MPI_Win_flush(target.id, amsgq->queue_win);

    MPI_Fetch_and_op(
      &decrement,
      &writecnt,
      MPI_INT32_T,
      target.id,
      base_offset,
      MPI_SUM,
      amsgq->queue_win);
    MPI_Win_flush(target.id, amsgq->queue_win);

    dart__base__mutex_unlock(&amsgq->send_mutex);

    // come back later
    return DART_ERR_AGAIN;
  }


  DART_LOG_TRACE("MPI_Fetch_and_op returned offset %u at unit %i",
                 remote_offset, target.id);
  //printf("  MPI_Fetch_and_op returned offset %u (%X) at unit %i [%p]\n",
  //               remote_offset, remote_offset, target.id, *(void**)&atomic_result);

  // 4. Write our payload

  struct dart_amsg_header header;
  header.remote    = unitid;
  header.fn        = fn;
  header.data_size = data_size;
  size_t offset    = base_offset + remote_offset + 2*sizeof(int32_t);
  MPI_Put(
    &header,
    sizeof(header),
    MPI_BYTE,
    target.id,
    offset,
    sizeof(header),
    MPI_BYTE,
    amsgq->queue_win);
  offset += sizeof(header);
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

  // 5. Deregister as a writer
  //    Use int64_t here, we cannot perform 64-bit substraction using only
  //    32bit lower part

#if 0
  MPI_Fetch_and_op(
    &dec,
    &atomic_result,
    MPI_UINT64_T,
    target.id,
    base_offset,
    MPI_SUM,
    amsgq->queue_win);
  MPI_Win_flush(target.id, amsgq->queue_win);
  /*printf("  Deregistering as writer: writecnt %d (%X), offset %d (%X) [%p]\n",
         atomic_result.writecnt, atomic_result.writecnt,
         atomic_result.offset, atomic_result.offset, *(void**)&atomic_result);
  */

  // DEBUG
  MPI_Fetch_and_op(
    NULL,
    &atomic_result,
    MPI_UINT64_T,
    target.id,
    base_offset,
    MPI_NO_OP,
    amsgq->queue_win);
  MPI_Win_flush(target.id, amsgq->queue_win);
  //printf("  Exit state: writecnt %d, offset %d [%p]\n", atomic_result.writecnt, atomic_result.offset, *(void**)&atomic_result);
  // DEBUG
#endif

#if 0
  MPI_Fetch_and_op(
    &decrement,
    &writecnt,
    MPI_INT32_T,
    target.id,
    base_offset,
    MPI_SUM,
    amsgq->queue_win);
  MPI_Win_flush(target.id, amsgq->queue_win);
  printf("  Deregistering as writer: writecnt %d\n", writecnt);
#endif

  MPI_Accumulate(&decrement, 1, MPI_INT32_T, target.id,
                 base_offset, 1, MPI_INT32_T, MPI_SUM, amsgq->queue_win);
  // local flush is sufficient, just make sure we can return
  MPI_Win_flush_local(target.id, amsgq->queue_win);

  dart__base__mutex_unlock(&amsgq->send_mutex);

  DART_LOG_INFO("Sent message of size %u with payload %zu to unit "
                "%d starting at offset %u",
                msg_size, data_size, target.id, remote_offset);

  return DART_OK;
}



static dart_ret_t
amsg_process_nolock_internal(
  struct dart_amsgq_impl_data * amsgq,
  bool                          blocking)
{
  dart_team_unit_t unitid;
  uint32_t         tailpos;

  if (!blocking) {
    dart_ret_t ret = dart__base__mutex_trylock(&amsgq->processing_mutex);
    if (ret != DART_OK) {
      return DART_ERR_AGAIN;
    }
  } else {
    dart__base__mutex_lock(&amsgq->processing_mutex);
  }

  do {
    dart_team_myid(amsgq->team, &unitid);

    char queuenum = amsgq->current_queue;
    int32_t base_offset = 1 + queuenum * (amsgq->queue_size + 2*sizeof(int32_t));

    //printf("Reading from queue %i\n", queuenum);

    //check whether there are active messages available
    MPI_Fetch_and_op(
      NULL,
      &tailpos,
      MPI_INT32_T,
      unitid.id,
      base_offset + sizeof(int32_t),
      MPI_NO_OP,
      amsgq->queue_win);
    MPI_Win_flush_local(unitid.id, amsgq->queue_win);

    /*printf("  Writecnt %i (%X), offset %i (%X) [%p]\n",
           atomic_value.writecnt, atomic_value.writecnt,
           atomic_value.offset, atomic_value.offset, *(void**)&atomic_value);
    */

    if (tailpos > 0) {

      // swap the current queuenumber
      char newqueuenum = (queuenum + 1) % 2;
      amsgq->current_queue = newqueuenum;
      char tmpc;
      MPI_Fetch_and_op(
        &newqueuenum,
        &tmpc,
        MPI_BYTE,
        unitid.id,
        0,
        MPI_REPLACE,
        amsgq->queue_win);
      MPI_Win_flush(unitid.id, amsgq->queue_win);

      //printf("Tailpos on queue %d: %d\n", queuenum, tailpos);

      // wait for all active writers to finish
      // and set writecnt to INT_MIN to signal the swap
      int32_t writecnt;
      do {
        const int32_t zero   = 0;
        const int32_t minval = INT32_MIN;
        MPI_Compare_and_swap(
          &minval,
          &zero,
          &writecnt,
          MPI_INT32_T,
          unitid.id,
          base_offset,
          amsgq->queue_win);
        MPI_Win_flush(unitid.id, amsgq->queue_win);
        //printf("  Active writers on queue %d: writecnt %d\n", queuenum, writecnt);
      } while (writecnt > 0);

      MPI_Fetch_and_op(
        NULL,
        &tailpos,
        MPI_INT32_T,
        unitid.id,
        base_offset + sizeof(int32_t),
        MPI_NO_OP,
        amsgq->queue_win);
      MPI_Win_flush_local(unitid.id, amsgq->queue_win);

      //printf("  Tailpos on queue %d: %d\n", queuenum, tailpos);

      // at this point we can safely process the queue as all pending writers
      // are finished and new writers write to the other queue

      // process the messages by invoking the functions on the data supplied
      uint64_t pos = 0;
      int num_msg  = 0;
      void *dbuf = amsgq->queue_ptr + base_offset + 2*sizeof(int32_t);

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
                        "Message out of bounds (expected %u but saw %lu)\n",
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

      //printf("  Processed %d messages\n", num_msg);

      // Finally: reset the old queue for the next swap
      uint64_t zero = 0;
      MPI_Put(
        &zero,
        1,
        MPI_UINT64_T,
        unitid.id,
        base_offset,
        1,
        MPI_UINT64_T,
        amsgq->queue_win);
      MPI_Win_flush(unitid.id, amsgq->queue_win);
    }
  } while (blocking && tailpos > 0);
  dart__base__mutex_unlock(&amsgq->processing_mutex);
  return DART_OK;
}

static
dart_ret_t
dart_amsg_nolock_process(struct dart_amsgq_impl_data * amsgq)
{
  return amsg_process_nolock_internal(amsgq, false);
}

static
dart_ret_t
dart_amsg_nolock_flush_buffer(struct dart_amsgq_impl_data * amsgq)
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
      ret = dart_amsg_nolock_sendbuf(target, amsgq, msgbuf, pos);
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
dart_amsg_nolock_process_blocking(
  struct dart_amsgq_impl_data * amsgq, dart_team_t team)
{
  int         flag = 0;
  MPI_Request req;
  dart_team_data_t *team_data = dart_adapt_teamlist_get(amsgq->team);
  if (team_data == NULL) {
    DART_LOG_ERROR("dart_gptr_getaddr ! Unknown team %i", amsgq->team);
    return DART_ERR_INVAL;
  }

  // flush our buffer
  dart_amsg_nolock_flush_buffer(amsgq);

  // keep processing until all incoming messages have been dealt with
  MPI_Ibarrier(team_data->comm, &req);
  do {
    amsg_process_nolock_internal(amsgq, true);
    MPI_Test(&req, &flag, MPI_STATUSES_IGNORE);
  } while (!flag);
  amsg_process_nolock_internal(amsgq, true);
  MPI_Barrier(team_data->comm);
  return DART_OK;
}


static dart_ret_t
dart_amsg_nolock_bsend(
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
dart_amsg_nolock_closeq(struct dart_amsgq_impl_data* amsgq)
{
  amsgq->queue_ptr = NULL;
  MPI_Win_unlock_all(amsgq->queue_win);
  MPI_Win_free(&(amsgq->queue_win));

  /*
  dart_team_memfree(amsgq->team, amsgq->queue_win);
  dart_team_memfree(amsgq->team, amsgq->tailpos_win);
  */
  free(amsgq);

  dart__base__mutex_destroy(&amsgq->send_mutex);
  dart__base__mutex_destroy(&amsgq->processing_mutex);

  return DART_OK;
}


dart_ret_t
dart_amsg_nolock_init(dart_amsgq_impl_t* impl)
{
  impl->openq   = dart_amsg_nolock_openq;
  impl->closeq  = dart_amsg_nolock_closeq;
  impl->bsend   = dart_amsg_nolock_bsend;
  impl->trysend = dart_amsg_nolock_trysend;
  impl->flush   = dart_amsg_nolock_flush_buffer;
  impl->process = dart_amsg_nolock_process;
  impl->process_blocking = dart_amsg_nolock_process_blocking;
  return DART_OK;
}
