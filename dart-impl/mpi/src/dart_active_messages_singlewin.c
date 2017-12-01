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

#if defined(DART_AMSGQ_SINGLEWIN)

struct dart_amsgq {
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

struct dart_amsg_header {
  dart_task_action_t fn;
  size_t             data_size;
#ifdef DART_ENABLE_LOGGING
  dart_global_unit_t remote;
#endif // DART_ENABLE_LOGGING
};

static bool initialized       = false;
static bool needs_translation = false;
static ptrdiff_t *offsets     = NULL;

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
dart_amsgq_fini()
{
  free(offsets);
  offsets = NULL;
  initialized = false;
  return DART_OK;
}

dart_ret_t
dart_amsg_openq(
  size_t         msg_size,
  size_t         msg_count,
  dart_team_t    team,
  dart_amsgq_t * queue)
{
  struct dart_amsgq *res = calloc(1, sizeof(struct dart_amsgq));
  res->size = msg_count * (sizeof(struct dart_amsg_header) + msg_size);
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
  res->comm = team_data->comm;
//  MPI_Comm_dup(team_data->comm, &res->comm);
  MPI_Comm_rank(res->comm, &res->my_rank);
  /**
   * Allocate the queue
   * We cannot use dart_team_memalloc_aligned because it uses
   * MPI_Win_allocate_shared that cannot be used for window locking.
   */
  MPI_Win_allocate(
    res->size,
    1,
    MPI_INFO_NULL,
    res->comm,
    (void*)&(res->tailpos_ptr),
    &(res->win));
  *(res->tailpos_ptr) = 0;

  MPI_Barrier(res->comm);

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
  uint64_t msg_size = (sizeof(struct dart_amsg_header) + data_size);
  uint64_t remote_offset = 0;

  dart__base__mutex_lock(&amsgq->send_mutex);

  dart_task_action_t remote_fn_ptr =
                        (dart_task_action_t)translate_fnptr(fn, target, amsgq);

  DART_LOG_DEBUG("dart_amsg_trysend: u:%i t:%i translated fn:%p",
                 target, amsgq->team, remote_fn_ptr);

  dart_myid(&unitid);

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
    dart__base__mutex_unlock(&amsgq->send_mutex);
    return DART_ERR_INVAL;
  }

  if ((remote_offset + msg_size) >= amsgq->size) {
#ifdef DART_AMSGQ_ATOMICS
    uint64_t tmp;
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
    DART_LOG_TRACE("Not enough space for message of size %i at unit %i "
                   "(current offset %llu of %llu)",
                   msg_size, target, remote_offset, amsgq->size);
    dart__base__mutex_unlock(&amsgq->send_mutex);
    return DART_ERR_AGAIN;
  }

  struct dart_amsg_header header;
#ifdef DART_ENABLE_LOGGING
  header.remote = unitid;
#endif //DART_ENABLE_LOGGING
  header.fn = remote_fn_ptr;
  header.data_size = data_size;

  // we now have a slot in the message queue
  MPI_Aint queue_disp = remote_offset + sizeof(*(amsgq->tailpos_ptr));
  // put the header
  MPI_Put(
      &header,
      sizeof(header),
      MPI_BYTE,
      target.id,
      queue_disp, sizeof
      (header),
      MPI_BYTE,
      amsgq->win);
  queue_disp += sizeof(header);
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
  uint64_t         tailpos;

  dart_team_data_t *team_data = dart_adapt_teamlist_get(amsgq->team);

  // trigger progress
  // XXX: if we do not do that, the application hangs with Open MPI 3.0.0. Bug?
  int flag;
  MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, amsgq->comm, &flag, MPI_STATUS_IGNORE);

  if (!blocking) {
    // XXX: we cannot shortcut because we have to trigger the MPI progress
    //      engine! Shortcutting will cause epic pain in the lowest circles of
    //      the debug hell!
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
      uint64_t   zero = 0;
      DART_LOG_INFO("Checking for new active messages (tailpos=%i)", tailpos);
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
                      header->remote,
                      data,
                      header->data_size,
                      startpos);
        header->fn(data);
      }
    } else {
      MPI_Win_unlock(amsgq->my_rank, amsgq->win);
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

dart_ret_t
dart_amsg_sync(dart_amsgq_t amsgq)
{
  MPI_Barrier(amsgq->comm);
  return dart_amsg_process(amsgq);
}

dart_ret_t
dart_amsg_closeq(dart_amsgq_t amsgq)
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
  intptr_t remote_fnptr = (intptr_t)fnptr;
  if (needs_translation) {
    ptrdiff_t  remote_fn_offset;
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
    offsets = malloc(numunits * sizeof(ptrdiff_t));
    if (!offsets) {
      return DART_ERR_INVAL;
    }
    DART_LOG_TRACE("Active message function offsets:");
    for (size_t i = 0; i < numunits; i++) {
      offsets[i] = bases[i] - ((uintptr_t)&dart_amsg_openq);
      DART_LOG_TRACE("   %i: %lli", i, offsets[i]);
    }
  }

  free(bases);
  return DART_OK;
}

#endif // DART_AMSGQ_LOCKFREE

