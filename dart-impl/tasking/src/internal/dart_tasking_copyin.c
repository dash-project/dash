
#include <dash/dart/if/dart_communication.h>

#include <dash/dart/base/env.h>
#include <dash/dart/tasking/dart_tasking_copyin.h>
#include <dash/dart/tasking/dart_tasking_remote.h>
#include <dash/dart/tasking/dart_tasking_datadeps.h>

enum dart_copyin_t {
  COPYIN_GET,
  COPYIN_SENDRECV,
  COPYIN_UNDEFINED
};

static const struct dart_env_str2int env_vals[] = {
  {"GET",             COPYIN_GET},
  {"COPYIN_GET",      COPYIN_GET},
  {"SENDRECV",        COPYIN_SENDRECV},
  {"COPYIN_SENDRECV", COPYIN_SENDRECV},
  {NULL, 0}
};

/**
 *
 * Functionality for pre-fetching data asynchronously, to be used in a COPYIN
 * dependency.
 *
 * TODO: use correct types for copyin.
 */

struct copyin_taskdata {
  dart_gptr_t   src;         // the global pointer to send from/get from
  void        * dst;         // the local pointer to recv into
  size_t        num_bytes;   // number of bytes
  dart_unit_t   unit;        // global unit ID to send to / recv from
  int           tag;         // a tag to use in case of send/recv
};

/**
 * A task action to prefetch data in a COPYIN dependency.
 * The argument should be a pointer to an object of type
 * struct copyin_taskdata.
 */
static void
dart_tasking_copyin_recv_taskfn(void *data);

/**
 * A task action to send data in a COPYIN dependency
 * (potentially required by the recv_taskfn if two-sided pre-fetch is required).
 * The argument should be a pointer to an object of type
 * struct copyin_taskdata.
 */
static void
dart_tasking_copyin_send_taskfn(void *data);

/**
 * A task action to prefetch data in a COPYIN dependency using dart_get.
 * The argument should be a pointer to an object of type
 * struct copyin_taskdata.
 */
static void
dart_tasking_copyin_get_taskfn(void *data);



static dart_ret_t
dart_tasking_copyin_create_task_sendrecv(
  const dart_task_dep_t * dep,
        dart_gptr_t       dest_gptr,
        taskref           local_task)
{
  static int global_tag_counter = 0; // next tag for pre-fetch communication

  // a) send a request for sending to the target
  int tag = 0;

  dart_global_unit_t myid;
  dart_myid(&myid);

  struct copyin_taskdata arg;
  arg.src = DART_GPTR_NULL;

  dart_global_unit_t send_unit;
  dart_team_unit_l2g(dep->gptr.teamid,
                     DART_TEAM_UNIT_ID(dep->gptr.unitid),
                     &send_unit);

  if (myid.id != send_unit.id) {
    tag = global_tag_counter++;
    DART_LOG_TRACE("Copyin: sendrequest with tag %d for task %p to unit %d in phase %d",
                   tag, local_task.local, send_unit.id, dep->phase);
    dart_tasking_remote_sendrequest(
      send_unit, dep->copyin.gptr, dep->copyin.size, tag, dep->phase);
  } else {
    // make it a local copy
    dart_gptr_t src_gptr = dart_tasking_datadeps_localize_gptr(dep->copyin.gptr);
    arg.src = src_gptr;
  }
  // b) add the receive to the destination

  arg.tag       = tag;
  arg.dst       = dep->copyin.dest;
  arg.num_bytes = dep->copyin.size;
  arg.unit      = send_unit.id;

  dart_task_dep_t out_dep;
  out_dep.type  = DART_DEP_OUT;
  out_dep.phase = dep->phase;
  out_dep.gptr  = dest_gptr;

  DART_LOG_TRACE("Copyin: creating task to recv from unit %d with tag %d in phase %d",
                 arg.unit, tag, dep->phase);
  dart_task_create(&dart_tasking_copyin_recv_taskfn, &arg, sizeof(arg),
                   &out_dep, 1, DART_PRIO_HIGH);

  return DART_OK;
}

static dart_ret_t
dart_tasking_copyin_create_task_get(
  const dart_task_dep_t *dep,
        dart_gptr_t      dest_gptr,
        taskref          local_task)
{
  // unused
  (void)local_task;

  dart_task_dep_t deps[2];
  deps[0].type  = DART_DEP_IN;
  deps[0].phase = dep->phase;
  deps[0].gptr  = dep->copyin.gptr;

  deps[1].type  = DART_DEP_OUT;
  deps[1].phase = dep->phase;
  deps[1].gptr  = dest_gptr;

  struct copyin_taskdata arg;
  arg.tag       = 0; // not needed
  arg.src       = dep->copyin.gptr;
  arg.dst       = dep->copyin.dest;
  arg.num_bytes = dep->copyin.size;
  arg.unit      = 0; // not needed

  dart_task_create(&dart_tasking_copyin_get_taskfn, &arg, sizeof(arg),
                   deps, 2, DART_PRIO_HIGH);
}

dart_ret_t
dart_tasking_copyin_create_task(
  const dart_task_dep_t *dep,
        dart_gptr_t      dest_gptr,
        taskref          local_task)
{
  static enum dart_copyin_t impl = COPYIN_UNDEFINED;
  if (impl == COPYIN_UNDEFINED) {
    impl = dart__base__env__str2int(DART_COPYIN_IMPL_ENVSTR, env_vals, COPYIN_GET);
    DART_LOG_INFO("Using copyin implementation %s",
                  impl == COPYIN_GET ? "GET" : "SENDRECV");
  }

  dart_ret_t ret;
  if (impl == COPYIN_SENDRECV) {
    ret =  dart_tasking_copyin_create_task_sendrecv(dep, dest_gptr, local_task);
  } else if (impl == COPYIN_GET) {
    ret =  dart_tasking_copyin_create_task_get(dep, dest_gptr, local_task);
  } else {
    // just in case...
    DART_ASSERT(impl == COPYIN_GET || impl == COPYIN_SENDRECV);
  }

  return ret;
}


void
dart_tasking_copyin_sendrequest(
  dart_gptr_t            src_gptr,
  int32_t                num_bytes,
  dart_taskphase_t       phase,
  int                    tag,
  dart_global_unit_t     unit)
{
  struct copyin_taskdata arg;
  arg.src       = dart_tasking_datadeps_localize_gptr(src_gptr);
  arg.dst       = NULL;
  arg.num_bytes = num_bytes;
  arg.unit      = unit.id;
  arg.tag       = tag;

  dart_task_dep_t in_dep;
  in_dep.type   = DART_DEP_DELAYED_IN;
  in_dep.phase  = phase;
  in_dep.gptr   = src_gptr;

  DART_LOG_TRACE("Copyin: creating task to send to unit %d with tag %d in phase %d",
                 unit.id, tag, phase);

  dart_task_create(&dart_tasking_copyin_send_taskfn, &arg, sizeof(arg),
                   &in_dep, 1, DART_PRIO_HIGH);
}

static void
dart_tasking_copyin_send_taskfn(void *data)
{
  struct copyin_taskdata *td = (struct copyin_taskdata*) data;

  //printf("Posting send to unit %d (tag %d, size %zu)\n", td->unit, td->tag, td->num_bytes);
  dart_handle_t handle;
  dart_send_handle(td->src.addr_or_offs.addr, td->num_bytes, DART_TYPE_BYTE,
                   td->tag, DART_GLOBAL_UNIT_ID(td->unit), &handle);
  int32_t flag = 0;
  dart_test_local(&handle, &flag);
  // lower task priority to better overlap communication/computation
  dart_task_t *task = dart__tasking__current_task();
  task->prio = DART_PRIO_LOW;
  while (!flag) {
    dart_task_yield(-1);
    dart_test_local(&handle, &flag);
  }

  //printf("Send to unit %d completed (tag %d)\n", td->unit, td->tag);
}

static void
dart_tasking_copyin_recv_taskfn(void *data)
{
  struct copyin_taskdata *td = (struct copyin_taskdata*) data;

  if (DART_GPTR_ISNULL(td->src)) {
    //printf("Posting recv from unit %d (tag %d, size %zu)\n",
    //       td->unit, td->tag, td->num_bytes);

    dart_handle_t handle;
    dart_recv_handle(td->dst, td->num_bytes, DART_TYPE_BYTE,
                    td->tag, DART_GLOBAL_UNIT_ID(td->unit), &handle);
    int flag = 0;
    dart_test_local(&handle, &flag);
    // lower task priority to better overlap communication/computation
    dart_task_t *task = dart__tasking__current_task();
    task->prio = DART_PRIO_LOW;
    while (!flag) {
      dart_task_yield(-1);
      dart_test_local(&handle, &flag);
    }
    //printf("Recv from unit %d completed (tag %d)\n", td->unit, td->tag);

  } else {
    //printf("Local memcpy of size %zu: %p -> %p\n", td->num_bytes, td->src, td->dst);
    memcpy(td->dst, td->src.addr_or_offs.addr, td->num_bytes);
  }

}


static void
dart_tasking_copyin_get_taskfn(void *data)
{
  struct copyin_taskdata *td = (struct copyin_taskdata*) data;

  int flag = 0;
  dart_handle_t handle;
  dart_get_handle(td->dst, td->src, td->num_bytes, DART_TYPE_BYTE, &handle);
  dart_test_local(&handle, &flag);
  // lower task priority to better overlap communication/computation
  dart_task_t *task = dart__tasking__current_task();
  task->prio = DART_PRIO_LOW;
  while (!flag) {
    dart_task_yield(-1);
    dart_test_local(&handle, &flag);
  }
}
