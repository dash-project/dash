#include <stdlib.h>
#include <unistd.h>
#include <mpi.h>
#include <errno.h>
#include <pthread.h>
#include <stdbool.h>

#include <dash/dart/base/mutex.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/if/dart_active_messages.h>
#include <dash/dart/if/dart_communication.h>
#include <dash/dart/if/dart_globmem.h>
#include <dash/dart/mpi/dart_team_private.h>
#include <dash/dart/mpi/dart_globmem_priv.h>
#include <dash/dart/mpi/dart_active_messages_priv.h>


#ifdef DART_AMSGQ_LOCKFREE

struct dart_amsgq {
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
  MPI_Win      queue_win;
  void        *queue_ptr;
  /// the size (in byte) of each message queue
  uint64_t     queue_size;
  dart_team_t  team;
  dart_mutex_t send_mutex;
  dart_mutex_t processing_mutex;
};

struct dart_amsg_header {
  dart_task_action_t fn;
  dart_global_unit_t remote;
  size_t             data_size;
};

static bool initialized = false;
static bool needs_translation = false;
static intptr_t *offsets = NULL;

static inline
uint64_t translate_fnptr(
  dart_task_action_t fnptr,
  dart_team_unit_t   target,
  dart_amsgq_t       amsgq);

static inline dart_ret_t exchange_fnoffsets();

/**
 * Initialize the active messaging subsystem, mainly to determine the
 * offsets of function pointers between different units.
 * This has to be done only once in a collective global operation.
 *
 * We assume that there is a single offset for all function pointers.
 */
dart_ret_t
dart_amsg_init()
{
  if (initialized) return DART_OK;

  int ret = exchange_fnoffsets();
  if (ret != DART_OK) return ret;

  initialized = true;

  return DART_OK;
}

dart_ret_t
dart_amsg_openq(
  size_t      msg_size,
  size_t      msg_count,
  dart_team_t team,
  dart_amsgq_t * queue)
{
  dart_team_unit_t unitid;
  struct dart_amsgq *res = calloc(1, sizeof(struct dart_amsgq));
  res->queue_size = 2*sizeof(int32_t) +
      msg_count * (sizeof(struct dart_amsg_header) + msg_size);
  res->team = team;

  size_t win_size = 2 * res->queue_size + 1;

  dart__base__mutex_init(&res->send_mutex);
  dart__base__mutex_init(&res->processing_mutex);

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


dart_ret_t
dart_amsg_trysend(
  dart_team_unit_t    target,
  dart_amsgq_t        amsgq,
  dart_task_action_t  fn,
  const void         *data,
  size_t              data_size)
{
  dart_global_unit_t unitid;

  dart__base__mutex_lock(&amsgq->send_mutex);

  dart_task_action_t remote_fn_ptr =
                        (dart_task_action_t)translate_fnptr(fn, target, amsgq);

  DART_LOG_DEBUG("dart_amsg_trysend: u:%i t:%i translated fn:%p",
                 target, amsgq->team, remote_fn_ptr);

  dart_myid(&unitid);

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
  MPI_Win_flush(target.id, amsgq->queue_win);

  size_t base_offset = 1 + queue_num * amsgq->queue_size;

  // 2.1 increment the writer-counter
  int32_t increment =  1;
  int32_t decrement = -1;
  int32_t writecnt;

  MPI_Fetch_and_op(
    &increment,
    &writecnt,
    MPI_INTEGER,
    target.id,
    base_offset,
    MPI_SUM,
    amsgq->queue_win);

  // 2.2 increment the message queue offset
  uint32_t msg_size = (sizeof(struct dart_amsg_header) + data_size);
  uint32_t remote_offset = 0;
  MPI_Fetch_and_op(
    &msg_size,
    &remote_offset,
    MPI_INTEGER,
    target.id,
    base_offset + sizeof(int32_t),
    MPI_SUM,
    amsgq->queue_win);

  // 2.3 Wait for the two previous operations to finish
  MPI_Win_flush(target.id, amsgq->queue_win);


  DART_LOG_TRACE("MPI_Fetch_and_op returned offset %llu at unit %i",
                 remote_offset, target);


  // 3. check whether the queue is usable for us
  //    a writecnt <0 means that the queue is currently being processed
  //    if the returned offset is larger than the queue size we have to wait
  //    for the queue to be processed
  if (writecnt < 0 || remote_offset > amsgq->queue_size) {

    DART_LOG_TRACE("Not enough space for message of size %i at unit %i "
                   "(current offset %llu of %llu, writecnt: %i)",
                   msg_size, target, remote_offset, amsgq->size, writecnt);

    // reset position
    uint32_t tmp;
    MPI_Fetch_and_op(
      &remote_offset,
      &tmp,
      MPI_UINT32_T,
      target.id,
      base_offset + sizeof(int32_t),
      MPI_REPLACE,
      amsgq->queue_win);
    MPI_Win_flush(target.id, amsgq->queue_win);

    // cannot write to the queue atm, deregister as a writer
    MPI_Fetch_and_op(
      &decrement,
      &writecnt,
      MPI_INTEGER,
      target.id,
      base_offset,
      MPI_SUM,
      amsgq->queue_win);

    MPI_Win_flush(target.id, amsgq->queue_win);

    // try again
    return DART_ERR_AGAIN;
  }

  // 4. Write our payload

  struct dart_amsg_header header;
  header.remote = unitid;
  header.fn = remote_fn_ptr;
  header.data_size = data_size;
  size_t offset = base_offset + remote_offset + 2*sizeof(int32_t);
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

  // 5. Deregister as a writer
  MPI_Fetch_and_op(
    &decrement,
    &writecnt,
    MPI_INTEGER,
    target.id,
    base_offset,
    MPI_SUM,
    amsgq->queue_win);

  MPI_Win_flush(target.id, amsgq->queue_win);


  DART_LOG_INFO("Sent message of size %zu with payload %zu to unit "
                "%i starting at offset %li",
                    msg_size, data_size, target, remote_offset);

  return DART_OK;
}

dart_ret_t
dart_amsg_bcast(
  dart_team_t         team,
  dart_amsgq_t        amsgq,
  dart_task_action_t  fn,
  const void         *data,
  size_t              data_size)
{
  size_t size;
  dart_team_unit_t myid;
  dart_team_size(team, &size);
  dart_team_myid(team, &myid);

  // This is a quick and dirty approach.
  // TODO: try to overlap multiple transfers!
  for (size_t i = 0; i < size; i++) {
    if (i == myid.id) continue;
    do {
      dart_ret_t ret = dart_amsg_trysend(
                        DART_TEAM_UNIT_ID(i), amsgq, fn, data, data_size);
      if (ret == DART_OK) {
        break;
      } else if (ret == DART_ERR_AGAIN) {
        // just try again
        continue;
      } else {
        return ret;
      }
    } while (1);
  }

  return DART_OK;
}



static dart_ret_t
amsg_process_internal(
  dart_amsgq_t amsgq,
  bool         blocking)
{
  dart_team_unit_t unitid;
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
    dart_team_myid(amsgq->team, &unitid);

    char queuenum;
    char tmpc;

    // get the current queue number
    MPI_Get(
      &queuenum,
      1,
      MPI_BYTE,
      unitid.id,
      0,
      1,
      MPI_BYTE,
      amsgq->queue_win);

    MPI_Win_flush_local(unitid.id, amsgq->queue_win);

    int32_t base_offset = queuenum * amsgq->queue_size;

    //check whether there are active messages available

    MPI_Get(
      &tailpos,
      1,
      MPI_INT32_T,
      unitid.id,
      base_offset + sizeof(int32_t),
      1,
      MPI_INT32_T,
      amsgq->queue_win);
    MPI_Win_flush_local(unitid.id, amsgq->queue_win);

    if (tailpos != 0) {

      int32_t res;
      const int32_t zero = 0;
      const int32_t neg  = INT32_MIN;

      // reset the old queue
      MPI_Put(
        &zero,
        1,
        MPI_INT32_T,
        unitid.id,
        1, 1, MPI_INT32_T,
        amsgq->queue_win);

      MPI_Put(
        &zero,
        1,
        MPI_INT32_T,
        unitid.id,
        1 + sizeof(int32_t), 1, MPI_INT32_T,
        amsgq->queue_win);

      // swap the current queuenumber
      char newqueuenum = (queuenum) ? 0 : 1;
      MPI_Fetch_and_op(
        &newqueuenum,
        &tmpc,
        MPI_BYTE,
        unitid.id,
        0,
        MPI_REPLACE,
        amsgq->queue_win);

      // wait for all current writers to finish

      do {
        MPI_Compare_and_swap(
          &neg,
          &zero,
          &res,
          MPI_INT32_T,
          unitid.id,
          base_offset,
          amsgq->queue_win);
        MPI_Win_flush(unitid.id, amsgq->queue_win);
      } while (res == 0);


      // process the messages by invoking the functions on the data supplied
      uint64_t pos = 0;
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
                      header->remote,
                      data,
                      header->data_size,
                      startpos);
        header->fn(data);
      }

    }
  } while (blocking && tailpos > 0);
  dart__base__mutex_unlock(&amsgq->processing_mutex);
  return DART_OK;
}

dart_ret_t
dart_amsg_process(dart_amsgq_t amsgq)
{
  return amsg_process_internal(amsgq, false);
}

dart_ret_t
dart_amsg_process_blocking(dart_amsgq_t amsgq, dart_team_t team)
{
  int         flag = 0;
  MPI_Request req;
  dart_team_data_t *team_data = dart_adapt_teamlist_get(amsgq->team);
  if (team_data == NULL) {
    DART_LOG_ERROR("dart_gptr_getaddr ! Unknown team %i", amsgq->team);
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

dart_team_t
dart_amsg_team(const dart_amsgq_t amsgq)
{
  return amsgq->team;
}

dart_ret_t
dart_amsg_sync(dart_amsgq_t amsgq)
{
  dart_barrier(amsgq->team);
  return dart_amsg_process(amsgq);
}

dart_ret_t
dart_amsg_closeq(dart_amsgq_t amsgq)
{
  amsgq->queue_ptr = NULL;
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


/**
 * Private functions
 */


/**
 * Translate the function pointer to make it suitable for the target rank
 * using a static translation table. We do the translation everytime we send
 * a message as it saves space.
 */
static inline
uint64_t translate_fnptr(
  dart_task_action_t fnptr,
  dart_team_unit_t target,
  dart_amsgq_t amsgq) {
  uintptr_t remote_fnptr = (uintptr_t)fnptr;
  if (needs_translation) {
    intptr_t  remote_fn_offset;
    dart_global_unit_t global_target_id;
    dart_team_unit_l2g(amsgq->team, target, &global_target_id);
    remote_fn_offset = offsets[global_target_id.id];
    remote_fnptr += remote_fn_offset;
    DART_LOG_TRACE("Translated function pointer %p into %p on unit %i",
                   fnptr, remote_fnptr, global_target_id.id);
  }
  return remote_fnptr;
}

static inline dart_ret_t exchange_fnoffsets() {

  size_t numunits;
  dart_size(&numunits);
  uint64_t  base  = (uint64_t)&dart_amsg_openq;
  uint64_t *bases = calloc(numunits, sizeof(uint64_t));
  if (!bases) {
    return DART_ERR_INVAL;
  }

  DART_LOG_TRACE("Exchanging offsets (dart_amsg_openq = %p)",
                 &dart_amsg_openq);
  if (MPI_Allgather(
        &base,
        1,
        MPI_UINT64_T,
        bases,
        1,
        MPI_UINT64_T,
        DART_COMM_WORLD) != MPI_SUCCESS) {
    DART_LOG_ERROR("Failed to exchange base pointer offsets!");
    return DART_ERR_NOTINIT;
  }

  // check whether we need to use offsets at all
  for (size_t i = 0; i < numunits; i++) {
    if (bases[i] != base) {
      needs_translation = true;
      DART_LOG_INFO("Using base pointer offsets for active messages "
                    "(%p against %p on unit %i).", base, bases[i], i);
      break;
    }
  }

  if (needs_translation) {
    offsets = malloc(numunits * sizeof(intptr_t));
    if (!offsets) {
      return DART_ERR_INVAL;
    }
    DART_LOG_TRACE("Active message function offsets:");
    for (size_t i = 0; i < numunits; i++) {
      offsets[i] = bases[i] - ((uint64_t)&dart_amsg_openq);
      DART_LOG_TRACE("   %i: %lli", i, offsets[i]);
    }
  }

  free(bases);
  return DART_OK;
}

#endif // DART_AMSGQ_LOCKFREE
