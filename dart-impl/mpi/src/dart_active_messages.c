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

/**
 * TODO:
 *  1) Ensure proper locking of parallel threads!
 *  2) Should we allow for return values to be passed back?
 *  3) Use a distributed double buffer to allow for overlapping read/writes
 */

struct dart_amsgq {
  MPI_Win      tailpos_win;
  uint64_t    *tailpos_ptr;
  MPI_Win      queue_win;
  char        *queue_ptr;
  char        *dbuf;      // a double buffer used during message processing to shorten lock times
  uint64_t     size;      // the size (in byte) of the message queue
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

static inline uint64_t translate_fnptr(dart_task_action_t fnptr, dart_team_unit_t target, dart_amsgq_t amsgq);
static inline dart_ret_t exchange_fnoffsets();

/**
 * Initialize the active messaging subsystem, mainly to determine the offsets of function pointers
 * between different units. This has to be done only once in a collective global operation.
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

dart_amsgq_t
dart_amsg_openq(size_t msg_size, size_t msg_count, dart_team_t team)
{
  dart_team_unit_t unitid;
  struct dart_amsgq *res = calloc(1, sizeof(struct dart_amsgq));
  res->size = msg_count * (sizeof(struct dart_amsg_header) + msg_size);
  res->dbuf = malloc(res->size);
  res->team = team;

  dart_team_myid (team, &unitid);

  dart_mutex_init(&res->send_mutex);
  dart_mutex_init(&res->processing_mutex);

  // allocate the queue
  /**
   * We cannot use dart_team_memalloc_aligned because it uses MPI_Win_allocate_shared that
   * cannot be used for window locking.
   */
  /*
  if (dart_team_memalloc_aligned(team, size, &(res->queue)) != DART_OK) {
    DART_LOG_ERROR("Failed to allocate %i byte for queue", size);
  }
  DART_GPTR_COPY(queue, res->queue);
  queue.unitid = unitid;
  dart_gptr_getaddr (queue, (void**)&addr);
  memset(addr, 0, size);

  // allocate the tailpos pointer
  dart_team_memalloc_aligned(team, sizeof(int), &(res->tailpos));
  DART_GPTR_COPY(tailpos, res->tailpos);
  tailpos.unitid = unitid;
  dart_gptr_getaddr (tailpos, (void**)&addr);
  *addr = 0;
  */

  dart_team_data_t *team_data = dart_adapt_teamlist_get(team);
  if (team_data == NULL) {
    DART_LOG_ERROR("dart_gptr_getaddr ! Unknown team %i", team);
    return DART_ERR_INVAL;
  }

  MPI_Win_allocate(
    sizeof(uint64_t),
    1,
    MPI_INFO_NULL,
    team_data->comm,
    (void*)&(res->tailpos_ptr),
    &(res->tailpos_win));
  *(res->tailpos_ptr) = 0;
  MPI_Win_flush(unitid.id, res->tailpos_win);
  MPI_Win_allocate(
    res->size,
    1,
    MPI_INFO_NULL,
    team_data->comm,
    (void*)&(res->queue_ptr),
    &(res->queue_win));
  memset(res->queue_ptr, 0, res->size);
  MPI_Win_fence(0, res->queue_win);

  return res;
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
  uint64_t msg_size = (sizeof(struct dart_amsg_header) + data_size);
  uint64_t remote_offset = 0;

  dart_mutex_lock(&amsgq->send_mutex);

  dart_task_action_t remote_fn_ptr = (dart_task_action_t)translate_fnptr(fn, target, amsgq);

  dart_myid(&unitid);

  //lock the tailpos window
  MPI_Win_lock(MPI_LOCK_EXCLUSIVE, target.id, 0, amsgq->tailpos_win);

  // Add the size of the message to the tailpos at the target
  if (MPI_Fetch_and_op(&msg_size, &remote_offset, MPI_UINT64_T, target.id, 0, MPI_SUM, amsgq->tailpos_win) != MPI_SUCCESS) {
    DART_LOG_ERROR("MPI_Fetch_and_op failed!");
    dart_mutex_unlock(&amsgq->send_mutex);
    return DART_ERR_NOTINIT;
  }

  MPI_Win_flush(target.id, amsgq->tailpos_win);

  DART_LOG_INFO("MPI_Fetch_and_op returned offset %llu at unit %i", remote_offset, target);

  if (remote_offset >= amsgq->size) {
    DART_LOG_ERROR("Received offset larger than message queue size from unit %i (%llu but expected < %llu)", target.id, remote_offset, amsgq->size);
    dart_mutex_unlock(&amsgq->send_mutex);
    return DART_ERR_INVAL;
  }

  if ((remote_offset + msg_size) >= amsgq->size) {
    uint64_t tmp;
    // if not, revert the operation and free the lock to try again.
    MPI_Fetch_and_op(&remote_offset, &tmp, MPI_UINT64_T, target.id, 0, MPI_REPLACE, amsgq->tailpos_win);
    MPI_Win_unlock(target.id, amsgq->tailpos_win);
    DART_LOG_INFO("Not enough space for message of size %i at unit %i (current offset %llu of %llu)", msg_size, target, remote_offset, amsgq->size);
    dart_mutex_unlock(&amsgq->send_mutex);
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

  dart_mutex_unlock(&amsgq->send_mutex);

  DART_LOG_INFO("Sent message of size %zu with payload %zu to unit %i starting at offset %li",
                    msg_size, data_size, target, remote_offset);

  return DART_OK;
}


static dart_ret_t
amsg_process_internal(dart_amsgq_t amsgq, bool blocking)
{
  dart_team_unit_t unitid;
  uint64_t         tailpos;

  if (!blocking) {
    dart_ret_t ret = dart_mutex_trylock(&amsgq->processing_mutex);
    if (ret != DART_OK) {
      return DART_ERR_AGAIN;
    }
  } else {
    dart_mutex_lock(&amsgq->processing_mutex);
  }

  do {
    char *dbuf = amsgq->dbuf;

    dart_team_myid(amsgq->team, &unitid);

    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, unitid.id, 0, amsgq->tailpos_win);

    MPI_Get(
        &tailpos,
        1,
        MPI_UINT64_T,
        unitid.id,
        0,
        1,
        MPI_UINT64_T,
        amsgq->tailpos_win);

    MPI_Win_flush_local(unitid.id, amsgq->tailpos_win);

    if (tailpos > 0) {
      uint64_t   zero = 0;
      DART_LOG_INFO("Checking for new active messages (tailpos=%i)", tailpos);
      // lock the tailpos window
      MPI_Win_lock(MPI_LOCK_EXCLUSIVE, unitid.id, 0, amsgq->queue_win);
      // copy the content of the queue for processing
      MPI_Get(
          dbuf,
          tailpos,
          MPI_BYTE,
          unitid.id,
          0,
          tailpos,
          MPI_BYTE,
          amsgq->queue_win);
      MPI_Win_unlock(unitid.id, amsgq->queue_win);

      // reset the tailpos and release the lock on the message queue
      MPI_Put(
          &zero,
          1,
          MPI_INT64_T,
          unitid.id,
          0,
          1,
          MPI_INT64_T,
          amsgq->tailpos_win);
      MPI_Win_unlock(unitid.id, amsgq->tailpos_win);

      // process the messages by invoking the functions on the data supplied
      uint64_t pos = 0;
      while (pos < tailpos) {
  #ifdef DART_ENABLE_LOGGING
        int startpos = pos;
  #endif
        // unpack the message
        struct dart_amsg_header *header = (struct dart_amsg_header *)(dbuf + pos);
        pos += sizeof(struct dart_amsg_header);
        void *data     = dbuf + pos;
        pos += header->data_size;

        if (pos > tailpos) {
          DART_LOG_ERROR("Message out of bounds (expected %i but saw %i)\n",
                         tailpos,
                         pos);
          dart_mutex_unlock(&amsgq->processing_mutex);
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
    } else {
      MPI_Win_unlock(unitid.id, amsgq->tailpos_win);
    }
  } while (blocking && tailpos > 0);
  dart_mutex_unlock(&amsgq->processing_mutex);
  return DART_OK;
}

dart_ret_t
dart_amsg_process(dart_amsgq_t amsgq)
{
  return amsg_process_internal(amsgq, false);
}

dart_ret_t
dart_amsg_process_blocking(dart_amsgq_t amsgq)
{
  return amsg_process_internal(amsgq, true);
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
  free(amsgq->dbuf);
  amsgq->dbuf = NULL;
  amsgq->queue_ptr = NULL;
  MPI_Win_free(&(amsgq->tailpos_win));
  MPI_Win_free(&(amsgq->queue_win));

  /*
  dart_team_memfree(amsgq->team, amsgq->queue_win);
  dart_team_memfree(amsgq->team, amsgq->tailpos_win);
  */
  free(amsgq);

  dart_mutex_destroy(&amsgq->send_mutex);
  dart_mutex_destroy(&amsgq->processing_mutex);

  return DART_OK;
}


/**
 * Private functions
 */


/**
 * Translate the function pointer to make it suitable for the target rank using a static translation table.
 * we do the translation everytime we send a message as it saves space
 * TODO: would it be more efficient to store the translation data per message queue?
 */
static inline uint64_t translate_fnptr(dart_task_action_t fnptr, dart_team_unit_t target, dart_amsgq_t amsgq) {
  uintptr_t remote_fnptr = (uintptr_t)fnptr;
  if (needs_translation) {
    intptr_t  remote_fn_offset;
    dart_global_unit_t global_target_id;
    dart_team_unit_l2g(amsgq->team, target, &global_target_id);
    remote_fn_offset = offsets[global_target_id.id];
    remote_fnptr += remote_fn_offset;
    DART_LOG_TRACE("Translated function pointer %p into %p on unit %i", fnptr, remote_fnptr, global_target_id.id);
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

  DART_LOG_TRACE("Exchanging offsets (dart_amsg_openq = %p)", &dart_amsg_openq);
  if (MPI_Allgather(&base, 1, MPI_UINT64_T, bases, 1, MPI_UINT64_T, DART_COMM_WORLD) != MPI_SUCCESS) {
    DART_LOG_ERROR("Failed to exchange base pointer offsets!");
    return DART_ERR_NOTINIT;
  }

  // check whether we need to use offsets at all
  for (size_t i = 0; i < numunits; i++) {
    if (bases[i] != base) {
      needs_translation = true;
      DART_LOG_INFO("Using base pointer offsets for active messages (%p against %p on unit %i).", base, bases[i], i);
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

