
#include <mpi.h>

#include <dash/dart/if/dart_active_messages.h>
#include <dash/dart/if/dart_initialization.h>
#include <dash/dart/if/dart_team_group.h>
#include <dash/dart/base/env.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/mpi/dart_active_messages_priv.h>
#include <dash/dart/mpi/dart_team_private.h>

/**
 * Name of the environment variable controlling the active message queue
 * implementation to use.
 *
 * Possible values: 'dualwin', 'singlewin', 'nolock', 'sendrecv'
 *
 * The default is 'nolock'.
 */
#define DART_AMSGQ_IMPL_ENVSTR "DART_AMSGQ_IMPL"

static bool initialized = false;
static bool needs_translation = false;
static intptr_t *offsets = NULL;

struct dart_amsgq {
  struct dart_amsgq_impl_data* impl;
  dart_team_t                  team;
};

static dart_amsgq_impl_t amsgq_impl = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};

static inline
uint64_t translate_fnptr(
  dart_task_action_t fnptr,
  dart_team_unit_t   target,
  dart_amsgq_t       amsgq);

static inline dart_ret_t exchange_fnoffsets();

enum {
  DART_AMSGQ_TWOWIN,
  DART_AMSGQ_SINGLEWIN,
  DART_AMSGQ_NOLOCK,
  DART_AMSGQ_SENDRECV
};

static struct dart_env_str2int env_vals[] = {
  {"dualwin",   DART_AMSGQ_TWOWIN},
  {"singlewin", DART_AMSGQ_SINGLEWIN},
  {"nolock",    DART_AMSGQ_NOLOCK},
  {"sendrecv",  DART_AMSGQ_SENDRECV},
  {NULL, 0}
};

dart_ret_t
dart_amsg_init()
{
  dart_ret_t res;

  int impl = dart__base__env__str2int(DART_AMSGQ_IMPL_ENVSTR,
                                      env_vals, DART_AMSGQ_NOLOCK);

  switch(impl) {
    case DART_AMSGQ_TWOWIN:
      res = dart_amsg_dualwin_init(&amsgq_impl);
      DART_LOG_INFO("Using dual-window active message queue");
      break;
    case DART_AMSGQ_SINGLEWIN:
      res = dart_amsg_singlewin_init(&amsgq_impl);
      DART_LOG_INFO("Using single-window active message queue");
      break;
    case DART_AMSGQ_NOLOCK:
      res = dart_amsg_nolock_init(&amsgq_impl);
      DART_LOG_INFO("Using nolock single-window active message queue");
      break;
    case DART_AMSGQ_SENDRECV:
      res = dart_amsg_sendrecv_init(&amsgq_impl);
      DART_LOG_INFO("Using send/recv-based active message queue");
      break;
    default:
      DART_LOG_ERROR("UNKNOWN active message queue implementation: %d", impl);
      dart_abort(-1);
  }

  if (res != DART_OK) {
    return res;
  }

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
  *queue = malloc(sizeof(struct dart_amsgq));
  (*queue)->team = team;
  return amsgq_impl.openq(msg_size, msg_count, team, &(*queue)->impl);
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

  DART_LOG_DEBUG("dart_amsg_trysend: u:%i t:%i translated fn:%p",
                 target.id, amsgq->team, remote_fn_ptr);

  return amsgq_impl.trysend(target, amsgq->impl, remote_fn_ptr, data, data_size);
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
      dart_team_unit_t team_unit = DART_TEAM_UNIT_ID(i);
      dart_task_action_t remote_fn_ptr =
                        (dart_task_action_t)translate_fnptr(fn, team_unit, amsgq);
      dart_ret_t ret = amsgq_impl.trysend(
                        team_unit, amsgq->impl, remote_fn_ptr,
                        data, data_size);
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
  dart_task_action_t remote_fn_ptr =
                        (dart_task_action_t)translate_fnptr(fn, target, amsgq);
  // not all implementations have to offer a buffered send
  if (amsgq_impl.bsend != NULL) {
    return amsgq_impl.bsend(target, amsgq->impl, remote_fn_ptr, data, data_size);
  } else {
    return amsgq_impl.trysend(target, amsgq->impl, remote_fn_ptr, data, data_size);
  }
}

dart_ret_t
dart_amsg_nolock_flush_buffer(dart_amsgq_t amsgq)
{
  // not all implementations have to offer buffered send
  if (amsgq_impl.flush) {
    return amsgq_impl.flush(amsgq->impl);
  }
  return DART_OK;
}

dart_ret_t
dart_amsg_process(dart_amsgq_t amsgq)
{
  return amsgq_impl.process(amsgq->impl);
}

dart_ret_t
dart_amsg_process_blocking(dart_amsgq_t amsgq, dart_team_t team)
{
  return amsgq_impl.process_blocking(amsgq->impl, team);
}

dart_ret_t
dart_amsg_closeq(dart_amsgq_t amsgq)
{
  dart_ret_t ret = amsgq_impl.closeq(amsgq->impl);
  amsgq->impl = NULL;
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
    dart_team_unit_l2g(amsgq->team, target, &global_target_id);
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
