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
#include <dash/dart/tasking/dart_tasking_priv.h>


#ifdef DART_AMSGQ_SENDRECV

// 100*512B (512KB) receives posted by default
#define DEFAULT_MSG_SIZE 256
#define NUM_MSG 100

struct dart_amsgq {
  MPI_Request  requests[NUM_MSG];
//  MPI_Status   status[NUM_MSG];
  char         msg_bufs[NUM_MSG][DEFAULT_MSG_SIZE];
  char *       sendbuf;
  size_t       sendbuf_size;
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

static inline void clear_sendbuf();

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

  // attach a send buffer
  size_t bufsize = DEFAULT_MSG_SIZE * NUM_MSG + MPI_BSEND_OVERHEAD;
  void *sendbuf   = malloc(bufsize);
  MPI_Buffer_attach(sendbuf, bufsize);

  initialized = true;

  return DART_OK;
}

dart_ret_t
dart_amsgq_fini()
{
  free(offsets);
  offsets = NULL;
  void *sendbuf;
  int bufsize;
  MPI_Buffer_detach(&sendbuf, &bufsize);
  free(sendbuf);
  initialized = false;
}

dart_ret_t
dart_amsg_openq(
  size_t         msg_size,
  size_t         msg_count,
  dart_team_t    team,
  dart_amsgq_t * queue)
{
  dart_team_unit_t unitid;
  *queue = NULL;

  DART_ASSERT(
    (DEFAULT_MSG_SIZE - sizeof(struct dart_amsg_header)) >= msg_size);


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
//  MPI_Comm_dup(team_data->comm, &res->comm);
  MPI_Comm_rank(res->comm, &res->my_rank);

  // post receives
  for (int i = 0; i < NUM_MSG; ++i) {
    MPI_Irecv(
      res->msg_bufs[i],
      DEFAULT_MSG_SIZE,
      MPI_BYTE,
      MPI_ANY_SOURCE,
      MPI_ANY_TAG,
      res->comm,
      &res->requests[i]);
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
  uint64_t msg_size = (sizeof(struct dart_amsg_header) + data_size);
  uint64_t remote_offset = 0;

  DART_ASSERT(
    (DEFAULT_MSG_SIZE - sizeof(struct dart_amsg_header)) >= data_size);

  dart__base__mutex_lock(&amsgq->send_mutex);

  dart_task_action_t remote_fn_ptr =
                        (dart_task_action_t)translate_fnptr(fn, target, amsgq);

  DART_LOG_DEBUG("dart_amsg_trysend: u:%i t:%i translated fn:%p",
                 target, amsgq->team, remote_fn_ptr);

  char buf[DEFAULT_MSG_SIZE];

  dart_myid(&unitid);


  struct dart_amsg_header header;
  header.remote = unitid;
  header.fn = remote_fn_ptr;
  header.data_size = data_size;

  memcpy(buf, &header, sizeof(header));
  memcpy(buf + sizeof(header), data, data_size);

  int ret = MPI_Bsend(buf, DEFAULT_MSG_SIZE, MPI_BYTE, target.id, 0, amsgq->comm);
  if (ret != MPI_SUCCESS) {
    // retry after clearing the recv and send buffer
    amsg_process_internal(amsgq, false);
    clear_sendbuf();
    ret = MPI_Bsend(buf, DEFAULT_MSG_SIZE, MPI_BYTE, target.id, 0, amsgq->comm);
    if (ret != MPI_SUCCESS) {
      dart__base__mutex_unlock(&amsgq->send_mutex);
      return DART_ERR_AGAIN;
    }
  }

  dart__base__mutex_unlock(&amsgq->send_mutex);

  DART_LOG_INFO("Sent message of size %zu with payload %zu to unit "
                "%i starting at offset %li",
                    msg_size, data_size, target, remote_offset);

  return DART_OK;
}


static dart_ret_t
amsg_process_internal(
  dart_amsgq_t amsgq,
  bool         blocking)
{
  uint64_t num_msg;

  dart_team_data_t *team_data = dart_adapt_teamlist_get(amsgq->team);

  if (!blocking) {
    dart_ret_t ret = dart__base__mutex_trylock(&amsgq->processing_mutex);
    if (ret != DART_OK) {
      return DART_ERR_AGAIN;
    }
  } else {
    dart__base__mutex_lock(&amsgq->processing_mutex);
  }

//  int flag;
//  MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, amsgq->comm, &flag, MPI_STATUS_IGNORE);

  do {
    num_msg = 0;
    int outcount;
    int outidx[NUM_MSG];
    MPI_Testsome(
      NUM_MSG, amsgq->requests, &outcount, outidx, MPI_STATUSES_IGNORE);
    for (int i = 0; i < outcount; ++i) {
      // pick the message
      int idx = outidx[i];
      char *dbuf = amsgq->msg_bufs[idx];
      // unpack the message
      struct dart_amsg_header *header = (struct dart_amsg_header *)dbuf;
      void                    *data   = dbuf + sizeof(struct dart_amsg_header);

      // invoke the message
      DART_LOG_INFO("Invoking active message %p from %i on data %p of "
                    "size %i starting from tailpos %i",
                    header->fn,
                    header->remote,
                    data,
                    header->data_size,
                    startpos);
      header->fn(data);

      // repost the recv
      MPI_Irecv(
        amsgq->msg_bufs[idx], DEFAULT_MSG_SIZE, MPI_BYTE,
        MPI_ANY_SOURCE, MPI_ANY_TAG,
        amsgq->comm, &amsgq->requests[idx]);

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
dart_amsg_process_blocking(dart_amsgq_t amsgq)
{
  int         flag = 0;
  MPI_Request req;

  MPI_Ibarrier(amsgq->comm, &req);
  do {
    clear_sendbuf();
    amsg_process_internal(amsgq, true);
    MPI_Test(&req, &flag, MPI_STATUSES_IGNORE);
  } while (!flag);
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

static inline void clear_sendbuf()
{
  void *buf;
  int size;
  MPI_Buffer_detach(&buf, &size);
  MPI_Buffer_attach(buf, size);
}

#endif // DART_AMSGQ_LOCKFREE

