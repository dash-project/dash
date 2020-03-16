
#include <mpi.h>
#include <pthread.h>
#include <alloca.h>

#include <dash/dart/if/dart_active_messages.h>
#include <dash/dart/if/dart_initialization.h>
#include <dash/dart/if/dart_team_group.h>
#include <dash/dart/base/env.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/base/atomic.h>
#include <dash/dart/base/assert.h>
#include <dash/dart/base/mutex.h>
#include <dash/dart/mpi/dart_active_messages_priv.h>
#include <dash/dart/mpi/dart_team_private.h>

/**
 * Name of the environment variable controlling the active message queue
 * implementation to use.
 *
 * Possible values: 'singlewin', 'sendrecv', 'sopnop'
 *
 * The default is 'sendrecv'.
 */
#define DART_AMSGQ_IMPL_ENVSTR "DART_AMSGQ_IMPL"

/**
 * Name of the environment variable specifying the number of entries in the
 * active message queue. This value overrides any values passed to the
 * constructor of the message queue.
 *
 * Type: integral value with optional B, K, M, G qualifier.
 */
#define DART_AMSGQ_SIZE_ENVSTR  "DART_AMSGQ_SIZE"

/**
 * Name of the environment variable specifying the maximum size of a message
 * sent in the active message queue.
 *
 * Type: integral value with optional B, K, M, G qualifier.
 * Default: 4096B
 */
#define DART_AMSGQ_MSGSIZE_ENVSTR  "DART_AMSGQ_MSGSIZE"

#define DEFAULT_MSGCACHE_SIZE (4*1024)

static bool initialized          = false;
static bool needs_translation    = false;
static intptr_t *offsets         = NULL;
static size_t msgq_size_override = 0;
static size_t msgq_msgsize = DEFAULT_MSGCACHE_SIZE;

typedef struct cached_message_s cached_message_t;
typedef struct message_cache_s  message_cache_t;

struct cached_message_s
{
  struct dart_amsg_header header;  // header containing function and data-size
  uint8_t                 data[];
};

struct message_cache_s
{
  pthread_rwlock_t        mutex;
  int                     pos;
  int8_t                  buffer[];
};

struct dart_amsgq {
  struct dart_amsgq_impl_data  *impl;
  dart_mutex_t                  mutex;
  size_t                        team_size;
  dart_team_t                   team;
  message_cache_t             **message_cache;
  struct dart_flush_info       *flush_info;
};

static dart_amsgq_impl_t amsgq_impl = {NULL, NULL, NULL, NULL, NULL};

static inline
uint64_t translate_fnptr(
  dart_task_action_t fnptr,
  dart_team_unit_t   target,
  dart_amsgq_t       amsgq);

static inline dart_ret_t exchange_fnoffsets();

enum {
  DART_AMSGQ_SINGLEWIN,
  DART_AMSGQ_SOPNOP,
  DART_AMSGQ_SOPNOP2,
  DART_AMSGQ_SOPNOP3,
  DART_AMSGQ_SOPNOP4,
  DART_AMSGQ_SOPNOP5,
  DART_AMSGQ_SOPNOP6,
  DART_AMSGQ_SENDRECV,
  DART_AMSGQ_DUALWIN
};

static struct dart_env_str2int env_vals[] = {
  {"singlewin", DART_AMSGQ_SINGLEWIN},
  {"sopnop",    DART_AMSGQ_SOPNOP},
  {"sopnop2",    DART_AMSGQ_SOPNOP2},
  {"sopnop3",    DART_AMSGQ_SOPNOP3},
  {"sopnop4",    DART_AMSGQ_SOPNOP4},
  {"sopnop5",    DART_AMSGQ_SOPNOP5},
  {"sopnop6",    DART_AMSGQ_SOPNOP6},
  {"sendrecv",  DART_AMSGQ_SENDRECV},
  {"dualwin",  DART_AMSGQ_DUALWIN},
  {NULL, 0}
};

#ifdef DART_ENABLE_LOGGING
static uint32_t msgcnt = 0;
#endif // DART_ENABLE_LOGGING


static
dart_ret_t
flush_buffer_all(dart_amsgq_t amsgq, bool blocking);

dart_ret_t
dart_amsg_init()
{
  dart_ret_t res;

  int impl = dart__base__env__str2int(DART_AMSGQ_IMPL_ENVSTR,
                                      env_vals,  -1);

  switch(impl) {
    case DART_AMSGQ_SINGLEWIN:
      res = dart_amsg_singlewin_init(&amsgq_impl);
      DART_LOG_TRACE("Using single-window active message queue");
      break;
    case DART_AMSGQ_SOPNOP:
      res = dart_amsg_sopnop_init(&amsgq_impl);
      DART_LOG_TRACE("Using same-op-no-op single-window active message queue");
      break;
    case DART_AMSGQ_SOPNOP2:
      res = dart_amsg_sopnop2_init(&amsgq_impl);
      DART_LOG_TRACE("Using same-op-no-op single-window active message queue");
      break;
    case DART_AMSGQ_SOPNOP3:
      res = dart_amsg_sopnop3_init(&amsgq_impl);
      DART_LOG_TRACE("Using same-op-no-op single-window active message queue");
      break;
    case DART_AMSGQ_SOPNOP4:
      res = dart_amsg_sopnop4_init(&amsgq_impl);
      DART_LOG_TRACE("Using same-op-no-op single-window active message queue");
      break;
    case DART_AMSGQ_SOPNOP5:
      res = dart_amsg_sopnop5_init(&amsgq_impl);
      DART_LOG_TRACE("Using same-op-no-op single-window active message queue");
      break;
    case DART_AMSGQ_SOPNOP6:
      res = dart_amsg_sopnop6_init(&amsgq_impl);
      DART_LOG_TRACE("Using same-op-no-op single-window active message queue");
      break;
    case -1:
      DART_LOG_TRACE("Unknown active message queue: %s",
                     dart__base__env__string(DART_AMSGQ_IMPL_ENVSTR));
      /* fall-through */
    case DART_AMSGQ_SENDRECV:
      res = dart_amsg_sendrecv_init(&amsgq_impl);
      DART_LOG_TRACE("Using send/recv-based active message queue");
      break;
    case DART_AMSGQ_DUALWIN:
      res = dart_amsg_dualwin_init(&amsgq_impl);
      DART_LOG_TRACE("Using send/recv-based active message queue");
      break;
    default:
      DART_LOG_ERROR("UNKNOWN active message queue implementation: %d", impl);
      dart_abort(-1);
  }

  if (res != DART_OK) {
    if (res == DART_ERR_INVAL) {
      DART_LOG_WARN("Falling back to send/recv-based active message queue");
      res = dart_amsg_sendrecv_init(&amsgq_impl);
    } else {
      return res;
    }
  }

  msgq_size_override = dart__base__env__size(DART_AMSGQ_SIZE_ENVSTR, 0);
  msgq_msgsize       = dart__base__env__size(DART_AMSGQ_MSGSIZE_ENVSTR, DEFAULT_MSGCACHE_SIZE);

  res = exchange_fnoffsets();

  return res;
}

dart_ret_t
dart_amsg_openq(
  size_t      msg_size,
  size_t      msg_count,
  dart_team_t team,
  dart_amsgq_t * queue)
{
  dart_ret_t ret;
  size_t team_size;
  dart_team_size(team, &team_size);
  dart_amsgq_t q = malloc(sizeof(struct dart_amsgq));
  q->team = team;
  q->team_size = team_size;
  q->message_cache = calloc(team_size, sizeof(message_cache_t*));
  dart__base__mutex_init(&q->mutex);

  ret = amsgq_impl.openq(
                  msgq_msgsize,
                  msgq_size_override ? msgq_size_override : msg_count,
                  team,
                  &(q->impl));

  if (amsgq_impl.trysend_all) {
    q->flush_info = malloc(team_size*sizeof(struct dart_flush_info));
  } else {
    q->flush_info = NULL;
  }

  *queue = q;

  return ret;
}

dart_ret_t
dart_amsg_trysend(
  dart_team_unit_t    target,
  dart_amsgq_t        amsgq,
  dart_task_action_t  fn,
  const void         *data,
  size_t              data_size)
{
  dart_task_action_t remote_fn_ptr =
                        (dart_task_action_t)translate_fnptr(fn, target, amsgq);

  size_t msg_size = (sizeof(struct dart_amsg_header) + data_size);

  // assemble the message on the stack
  cached_message_t *msg = alloca(msg_size);

  // assemble the message
  msg->header.fn        = remote_fn_ptr;
  msg->header.data_size = data_size;
#ifdef DART_ENABLE_LOGGING
  dart_myid(&msg->header.remote);
  msg->header.msgid     = DART_FETCH_AND_INC32(&msgcnt);
#endif
  memcpy(msg->data, data, data_size);

  DART_LOG_DEBUG("dart_amsg_trysend: u:%i t:%i translated fn:%p id=%d",
                 target.id, amsgq->team, remote_fn_ptr, msg->header.msgid);

  return amsgq_impl.trysend(target, amsgq->impl, msg, msg_size);
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

  size_t msg_size = (sizeof(struct dart_amsg_header) + data_size);


  // assemble the message on the stack
  cached_message_t *msg = alloca(msg_size);

  // assemble the message
  msg->header.data_size = data_size;
#ifdef DART_ENABLE_LOGGING
  dart_myid(&msg->header.remote);
  msg->header.msgid     = DART_FETCH_AND_INC32(&msgcnt);
#endif
  memcpy(msg->data, data, data_size);

  // This is a quick and dirty approach.
  // TODO: try to overlap multiple transfers!
  for (size_t i = 0; i < size; i++) {
    if (i == myid.id) continue;
    do {
      dart_team_unit_t team_unit = DART_TEAM_UNIT_ID(i);
      dart_task_action_t remote_fn_ptr =
                        (dart_task_action_t)translate_fnptr(fn, team_unit, amsgq);
      msg->header.fn = remote_fn_ptr;
      dart_team_unit_t target = {i};

      dart_ret_t ret = amsgq_impl.trysend(target, amsgq->impl, msg , msg_size);

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

dart_ret_t
dart_amsg_buffered_send(
  dart_team_unit_t              target,
  dart_amsgq_t                  amsgq,
  dart_task_action_t            fn,
  const void                  * data,
  size_t                        data_size)
{
  message_cache_t *cache = NULL;
  if (amsgq->message_cache[target.id] == NULL) {
    dart__base__mutex_lock(&amsgq->mutex);
    if (amsgq->message_cache[target.id] == NULL) {
      cache = malloc(sizeof(message_cache_t) + msgq_msgsize);
      cache->pos           = 0;
      pthread_rwlock_init(&cache->mutex, NULL);
      amsgq->message_cache[target.id] = cache;
    }
    dart__base__mutex_unlock(&amsgq->mutex);
  }
  if (cache == NULL) {
    cache = amsgq->message_cache[target.id];
  }

  dart_task_action_t remote_fn_ptr =
                        (dart_task_action_t)translate_fnptr(fn, target, amsgq);

  int size_required = sizeof(cached_message_t) + data_size;
  pthread_rwlock_rdlock(&cache->mutex);
  int pos = DART_FETCH_AND_ADD32(&cache->pos, size_required);
  while ((pos + size_required) > msgq_msgsize) {
    // revert reservation
    DART_FETCH_AND_ADD32(&cache->pos, -size_required);
    pthread_rwlock_unlock(&cache->mutex);

    // if the implementation provides an efficient way to flush the whole buffer
    // we flush all buffers to avoid serializing flushes
    if (amsgq->flush_info != NULL) {
      if (DART_OK != flush_buffer_all(amsgq, false)) {
        // either we or another thread is currently flushing -> send as single message
        // NOTE: flush_buffer_all may call process() internally so this may be recursive
        dart_ret_t ret;
        DART_LOG_TRACE("Sending single message to %d", target.id);
        ret = dart_amsg_trysend(target, amsgq, fn, data, data_size);
        if (ret == DART_OK) {
          DART_LOG_TRACE("Sent single message to %d!", target.id);
          return DART_OK;
        } else {
          // try to process once, then retry to resend as buffered
          amsgq_impl.process(amsgq->impl);
        }
      }
    } else {
      // try to get a writelock
      pthread_rwlock_wrlock(&cache->mutex);
      // check whether we still need to flush
      if (cache->pos + size_required > msgq_msgsize) {
        // we got a write-lock, go to flush the buffer
        dart_ret_t ret;
        do {
          DART_LOG_TRACE("Flushing buffer to %d", target.id);
          ret = amsgq_impl.trysend(target, amsgq->impl, cache->buffer, cache->pos);
          if (DART_ERR_AGAIN == ret) {
            // try to process our messages while waiting for the other side
            amsgq_impl.process(amsgq->impl);
          } else if (ret != DART_OK) {
            pthread_rwlock_unlock(&cache->mutex);
            DART_LOG_ERROR("Failed to flush message cache!");
            return ret;
          }
        } while (ret != DART_OK);
        // reset position
        cache->pos = 0;
      }
      // release write lock and take the readlock again
      pthread_rwlock_unlock(&cache->mutex);
    }
    pthread_rwlock_rdlock(&cache->mutex);
    pos = DART_FETCH_AND_ADD32(&cache->pos, size_required);
  }

  cached_message_t *msg = (cached_message_t *)(cache->buffer + pos);
  msg->header.fn        = remote_fn_ptr;
  msg->header.data_size = data_size;
#ifdef DART_ENABLE_LOGGING
  dart_myid(&msg->header.remote);
  msg->header.msgid     = DART_FETCH_AND_INC32(&msgcnt);
#endif
  memcpy(msg->data, data, data_size);
  DART_LOG_TRACE("Cached message: fn=%p, r=%d, ds=%d, id=%d", msg->header.fn,
                 msg->header.remote.id, msg->header.data_size, msg->header.msgid);
  pthread_rwlock_unlock(&cache->mutex);
  return DART_OK;
}

static
dart_ret_t
flush_buffer_all(dart_amsgq_t amsgq, bool blocking)
{
  int comm_size = amsgq->team_size;

  // prevent other threads from interfering
  // don't block on the mutex though, might deadlock (due to processing)
  if (DART_OK != dart__base__mutex_trylock(&amsgq->mutex)) return DART_PENDING;

  struct dart_flush_info *flush_info = amsgq->flush_info;
  int num_info = 0;

  for (int target = 0; target < comm_size; ++target) {
    message_cache_t *cache = amsgq->message_cache[target];
    if (cache != NULL) {
      if (cache->pos > 0 || blocking) {
        pthread_rwlock_wrlock(&cache->mutex);

        if (cache->pos == 0) {
          // mpf, someone else processed that cache already
          pthread_rwlock_unlock(&cache->mutex);
          continue;
        }

        flush_info[num_info].data = cache->buffer;
        flush_info[num_info].size = cache->pos;
        flush_info[num_info].target = target;
        ++num_info;
      }
    }
  }

  while (num_info > 0) {
    dart_ret_t ret;
    ret = amsgq_impl.trysend_all(amsgq->impl, flush_info, num_info);

    int num_active = num_info;
    for (int i = 0; i < num_info; ++i) {
      message_cache_t *cache = amsgq->message_cache[flush_info[i].target];
      if (flush_info[i].completed) {
        --num_active;
        cache->pos = 0;
      }
      // unlock the cache, processing below may cause writing to the cache!
      pthread_rwlock_unlock(&cache->mutex);
    }

    // stop if called in non-blocking mode or all is done
    if (!blocking || num_active == 0) break;

    // progress incoming messages and try again
    // NOTE: do not get rid of this process(), otherwise all processes may 
    //       end up trying to flush!
    amsgq_impl.process(amsgq->impl);

    for (int i = 0; i < num_info; ++i) {
      if (!flush_info[i].completed) {
        message_cache_t *cache = amsgq->message_cache[flush_info[i].target];
        // retake lock on the cache
        if (cache->pos > 0) {
          pthread_rwlock_wrlock(&cache->mutex);

          if (cache->pos == 0) {
            // mpf, someone else processed that cache already
            pthread_rwlock_unlock(&cache->mutex);
            --num_active;
            flush_info[i].completed = true;
            continue;
          }
        }
        // move the entry to the front if necessary
        for (int j = 0; j < i; ++j) {
          if (flush_info[j].completed) {
            // this one is complete, so re-use its slot
            flush_info[j] = flush_info[i];
            break;
          }
        }
      }
    }
    num_info = num_active;
    DART_ASSERT(num_info >= 0);
  }

  dart__base__mutex_unlock(&amsgq->mutex);
  return DART_OK;
}


dart_ret_t
flush_buffer(dart_amsgq_t amsgq, bool blocking)
{
  if (amsgq->flush_info != NULL) {
    return flush_buffer_all(amsgq, blocking);
  }

  int comm_size = amsgq->team_size;

  for (int target = 0; target < comm_size; ++target) {
    message_cache_t *cache = amsgq->message_cache[target];
    if (cache != NULL) {
      if (cache->pos > 0 || blocking) {
        pthread_rwlock_wrlock(&cache->mutex);

        if (cache->pos == 0) {
          // mpf, someone else processed that cache already
          pthread_rwlock_unlock(&cache->mutex);
          continue;
        }

        dart_ret_t ret;
        dart_team_unit_t t = {target};
        do {
          ret = amsgq_impl.trysend(t, amsgq->impl, cache->buffer, cache->pos);
          if (DART_ERR_AGAIN == ret) {
            // TODO: this has the potential to deadlock as a message may trigger outgoing messages!
            amsgq_impl.process(amsgq->impl);
          } else if (ret != DART_OK) {
            DART_LOG_ERROR("Failed to flush message cache!");
            dart_abort(DART_EXIT_ASSERT);
          }
        } while (ret != DART_OK);

        cache->pos = 0;
        pthread_rwlock_unlock(&cache->mutex);
      }
    }
  }

  return DART_OK;
}


dart_ret_t
dart_amsg_flush_buffer(dart_amsgq_t amsgq)
{
  flush_buffer(amsgq, false);
}

dart_ret_t
dart_amsg_process(dart_amsgq_t amsgq)
{
  return amsgq_impl.process(amsgq->impl);
}

dart_ret_t
dart_amsg_process_blocking(dart_amsgq_t amsgq, dart_team_t team)
{
  size_t size;
  dart_team_size(team, &size);
  if (size == 1) {
    // nothing to be done here
    return DART_OK;
  }

  // flush our buffer and make sure all is sent
  flush_buffer(amsgq, true);

  return amsgq_impl.process_blocking(amsgq->impl, team);
}

dart_ret_t
dart_amsg_closeq(dart_amsgq_t amsgq)
{
  dart_ret_t ret = amsgq_impl.closeq(amsgq->impl);
  amsgq->impl = NULL;
  for (int target = 0; target < amsgq->team_size; ++target) {
    message_cache_t *cache = amsgq->message_cache[target];
    if (cache != NULL) {
      pthread_rwlock_destroy(&cache->mutex);
      free(cache);
      amsgq->message_cache[target] = NULL;
    }
  }
  free(amsgq->message_cache);
  free(amsgq);
  return ret;
}

dart_ret_t
dart_amsgq_fini()
{
  free(offsets);
  offsets = NULL;
  initialized = false;
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
    global_target_id.id = target.id;
    //dart_team_unit_l2g(amsgq->team, target, &global_target_id);
    remote_fn_offset = offsets[global_target_id.id];
    remote_fnptr += remote_fn_offset;
    DART_LOG_TRACE("Translated function pointer %p into %p on unit %i",
                   fnptr, (void*)remote_fnptr, global_target_id.id);
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
      DART_LOG_DEBUG("Using base pointer offsets for active messages "
                     "(%#lx against %#lx on unit %zu).", base, bases[i], i);
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
      DART_LOG_TRACE("   %zu: %#lx", i, offsets[i]);
    }
  }

  free(bases);
  return DART_OK;
}

/**
 * Function called from implementations to process the queue
 */
void
dart__amsgq__process_buffer(
  void   *dbuf,
  size_t  tailpos)
{

  // process the messages by invoking the functions on the data supplied
  int64_t  pos      = 0;
  int      num_msg  = 0;

  while (pos < tailpos) {
#ifdef DART_ENABLE_LOGGING
    int64_t startpos = pos;
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
    DART_LOG_TRACE("Invoking active message %p id=%d from %i on data %p of "
                  "size %i starting from tailpos %ld",
                  header->fn,
                  header->msgid,
                  header->remote.id,
                  data,
                  header->data_size,
                  startpos);
    header->fn(data);
    num_msg++;
  }
}
