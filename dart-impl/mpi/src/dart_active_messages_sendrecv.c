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


#ifdef DART_AMSGQ_SENDRECV

// 100*512B (512KB) receives posted by default
//#define DEFAULT_MSG_SIZE 256
//#define NUM_MSG 100

#define AMSGQ_MPI_TAG 10001


struct dart_amsgq {
  MPI_Request *recv_reqs;
  char *      *recv_bufs;
  MPI_Request *send_reqs;
  char *      *send_bufs;
  int         *outidx;
  size_t       msg_size;
  int          msg_count;
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

static bool initialized       = false;
static bool needs_translation = false;
static ptrdiff_t *offsets     = NULL;

static inline
uint64_t translate_fnptr(
  dart_task_action_t fnptr,
  dart_team_unit_t   target,
  dart_amsgq_t       amsgq);

static inline dart_ret_t exchange_fnoffsets();

static dart_ret_t
amsg_process_internal(
  dart_amsgq_t amsgq,
  bool         blocking);

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
  *queue = NULL;

  dart_team_data_t *team_data = dart_adapt_teamlist_get(team);
  if (team_data == NULL) {
    DART_LOG_ERROR("dart_gptr_getaddr ! Unknown team %i", team);
    return DART_ERR_INVAL;
  }
  struct dart_amsgq *res = calloc(1, sizeof(struct dart_amsgq));
  res->team = team;
  res->comm = team_data->comm;
  dart__base__mutex_init(&res->send_mutex);
  dart__base__mutex_init(&res->processing_mutex);
  res->msg_size = sizeof(struct dart_amsg_header) + msg_size;
  res->msg_count = msg_count;
  res->send_bufs = malloc(msg_count*sizeof(*res->send_bufs));
  res->send_reqs = malloc(msg_count*sizeof(*res->send_reqs));
  res->recv_bufs = malloc(msg_count*sizeof(*res->recv_bufs));
  res->recv_reqs = malloc(msg_count*sizeof(*res->recv_reqs));
  res->outidx    = malloc(msg_count*sizeof(*res->outidx));
//  MPI_Comm_dup(team_data->comm, &res->comm);
  MPI_Comm_rank(res->comm, &res->my_rank);

  // post receives
  for (int i = 0; i < msg_count; ++i) {
    res->recv_bufs[i] = malloc(res->msg_size);
    MPI_Irecv(
      res->recv_bufs[i],
      res->msg_size,
      MPI_BYTE,
      MPI_ANY_SOURCE,
      AMSGQ_MPI_TAG,
      res->comm,
      &res->recv_reqs[i]);
    res->send_bufs[i] = malloc(res->msg_size);
    res->send_reqs[i] = MPI_REQUEST_NULL;
  }

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
  uint64_t msg_size = sizeof(struct dart_amsg_header) + data_size;
  uint64_t remote_offset = 0;

  DART_ASSERT(msg_size <= amsgq->msg_size);

  dart__base__mutex_lock(&amsgq->send_mutex);

  dart_task_action_t remote_fn_ptr =
                        (dart_task_action_t)translate_fnptr(fn, target, amsgq);

  DART_LOG_DEBUG("dart_amsg_trysend: u:%i t:%i translated fn:%p",
                 target, amsgq->team, remote_fn_ptr);


  dart_myid(&unitid);

  int idx = 0;

  // search for a free handle
  bool found = false;
  for (idx = 0; idx < amsgq->msg_count; ++idx) {
    if (amsgq->send_reqs[idx] == MPI_REQUEST_NULL) {
      // we found one
      found = true;
      break;
    }
  }
  if (!found) {
    int flag;
    MPI_Testany(amsgq->msg_count, amsgq->send_reqs, &idx, &flag, MPI_STATUSES_IGNORE);
    if (flag) {
      if (idx == MPI_UNDEFINED) {
        // there were no active handles -> pick the first
        idx = 0;
      }
    } else {
      dart__base__mutex_unlock(&amsgq->send_mutex);
      // come back later
      return DART_ERR_AGAIN;
    }
  }

  struct dart_amsg_header header;
  header.remote = unitid;
  header.fn = remote_fn_ptr;
  header.data_size = data_size;

  char *sendbuf = amsgq->send_bufs[idx];

  memcpy(sendbuf, &header, sizeof(header));
  memcpy(sendbuf + sizeof(header), data, data_size);

  int ret = MPI_Isend(
              sendbuf, amsgq->msg_size,
              MPI_BYTE, target.id, AMSGQ_MPI_TAG, amsgq->comm,
              &amsgq->send_reqs[idx]);
  dart__base__mutex_unlock(&amsgq->send_mutex);

  if (ret != MPI_SUCCESS) {
    return DART_ERR_AGAIN;
  }

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
  uint64_t num_msg;

  if (!blocking) {
    dart_ret_t ret = dart__base__mutex_trylock(&amsgq->processing_mutex);
    if (ret != DART_OK) {
      return DART_ERR_AGAIN;
    }
  } else {
    dart__base__mutex_lock(&amsgq->processing_mutex);
  }

  do {
    num_msg = 0;
    int outcount;
    MPI_Testsome(
      amsgq->msg_count, amsgq->recv_reqs, &outcount,
      amsgq->outidx, MPI_STATUSES_IGNORE);
    for (int i = 0; i < outcount; ++i) {
      // pick the message
      int   idx  = amsgq->outidx[i];
      char *dbuf = amsgq->recv_bufs[idx];
      // unpack the message
      struct dart_amsg_header *header = (struct dart_amsg_header *)dbuf;
      void                    *data   = dbuf + sizeof(struct dart_amsg_header);

      // invoke the message
      DART_LOG_INFO("Invoking active message %p from %i on data %p of size %i",
                    header->fn,
                    header->remote,
                    data,
                    header->data_size);

      header->fn(data);

      // repost the recv
      MPI_Irecv(
        amsgq->recv_bufs[idx], amsgq->msg_size, MPI_BYTE,
        MPI_ANY_SOURCE, AMSGQ_MPI_TAG,
        amsgq->comm, &amsgq->recv_reqs[idx]);

      ++num_msg;
    }

    // repeat until we do not find messages anymore
  } while (blocking && num_msg > 0);
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
  MPI_Request req;

  dart_team_data_t *team_data = dart_adapt_teamlist_get(team);
  if (team_data == NULL) {
    DART_LOG_ERROR("dart_gptr_getaddr ! Unknown team %i", team);
    return DART_ERR_INVAL;
  }

  int         barrier_flag = 0;
  int         send_flag = 0;
  MPI_Ibarrier(team_data->comm, &req);
  do {
    amsg_process_internal(amsgq, true);
    if (!barrier_flag) {
      MPI_Test(&req, &barrier_flag, MPI_STATUS_IGNORE);
    }
    if (!send_flag) {
      MPI_Testall(amsgq->msg_count, amsgq->send_reqs,
                  &send_flag, MPI_STATUSES_IGNORE);
    }
  } while (!barrier_flag && !send_flag);
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

//  MPI_Comm_free(&amsgq->comm);

  for (int i = 0; i < amsgq->msg_count; ++i) {
    MPI_Cancel(&amsgq->recv_reqs[i]);
    free(amsgq->recv_bufs[i]);
    free(amsgq->send_bufs[i]);
  }

  free(amsgq->recv_bufs);
  free(amsgq->send_bufs);
  free(amsgq->recv_reqs);
  free(amsgq->send_reqs);
  free(amsgq->outidx);
  free(amsgq);

  dart__base__mutex_destroy(&amsgq->send_mutex);
  dart__base__mutex_destroy(&amsgq->processing_mutex);

  return DART_OK;
}


/**
 * Flush messages that were sent using \c dart_amsg_buffered_send.
 */
dart_ret_t
dart_amsg_flush_buffer(dart_amsgq_t amsgq)
{

  MPI_Waitall(amsgq->msg_count, amsgq->send_reqs, MPI_STATUSES_IGNORE);

  return DART_OK;
}

/**
 * Buffer the active message until it is sent out using \c dart_amsg_flush_buffer.
 */
dart_ret_t
dart_amsg_buffered_send(
  dart_team_unit_t    target,
  dart_amsgq_t        amsgq,
  dart_task_action_t  fn,
  const void         *data,
  size_t              data_size)
{
  return dart_amsg_trysend(target, amsgq, fn, data, data_size);
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

