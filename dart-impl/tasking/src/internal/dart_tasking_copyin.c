
#include <dash/dart/if/dart_communication.h>

#include <dash/dart/tasking/dart_tasking_copyin.h>
#include <dash/dart/tasking/dart_tasking_remote.h>
#include <dash/dart/tasking/dart_tasking_datadeps.h>

/**
 *
 * Functionality for pre-fetching data asynchronously, to be used in a COPYIN
 * dependency.
 *
 * TODO: use correct types for copyin.
 */

struct copyin_taskdata {
  void        * src;         // the local pointer to send from
  void        * dst;         // the local pointer to recv into
  size_t        num_bytes;   // number of bytes
  dart_unit_t   unit;        // global unit ID to send to / recv from
  int           tag;         // a tag to use in case of send/recv
};

static int global_tag_counter = 0; // next tag for pre-fetch communication

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

void
dart_tasking_copyin_sendrequest(
  dart_gptr_t            src_gptr,
  int32_t                num_bytes,
  dart_taskphase_t       phase,
  int                    tag,
  dart_global_unit_t     unit)
{
  struct copyin_taskdata arg;
  dart_gptr_getaddr(src_gptr, &arg.src);
  arg.num_bytes = num_bytes;
  arg.unit      = unit.id;
  arg.tag       = tag;

  dart_task_dep_t in_dep;
  in_dep.type   = DART_DEP_DELAYED_IN;
  in_dep.phase  = phase;
  in_dep.gptr   = src_gptr;

  dart_task_create(&dart_tasking_copyin_send_taskfn, &arg, sizeof(arg),
                   &in_dep, 1, DART_PRIO_LOW);
}

dart_ret_t
dart_tasking_copyin_create_task(
  dart_task_dep_t *dep,
  dart_gptr_t      dest_gptr,
  taskref          local_task)
{
  // a) send a request for sending to the target
  int tag = 0;

  dart_global_unit_t myid;
  dart_myid(&myid);

  struct copyin_taskdata arg;

  dart_global_unit_t send_unit;
  dart_team_unit_l2g(dep->gptr.teamid,
                     DART_TEAM_UNIT_ID(dep->gptr.unitid),
                     &send_unit);

  if (myid.id != send_unit.id) {
    arg.src = NULL;
    tag = global_tag_counter++;
    dart_tasking_remote_sendrequest(
      send_unit, dep->copyin.gptr, dep->copyin.size, tag, dep->phase);
  } else {
    // make it a local copy
    dart_gptr_t src_gptr = dart_tasking_datadeps_localize_gptr(dep->copyin.gptr);
    arg.src = src_gptr.addr_or_offs.addr;
  }
  // b) add the receive to the destination

  arg.tag       = tag;
  arg.dst       = dep->copyin.dest;
  arg.num_bytes = dep->copyin.size;
  arg.unit      = dep->copyin.gptr.unitid;

  dart_task_dep_t out_dep;
  out_dep.type  = DART_DEP_OUT;
  out_dep.phase = dep->phase;
  out_dep.gptr  = dest_gptr;

  dart_task_create(&dart_tasking_copyin_recv_taskfn, &arg, sizeof(arg),
                   &out_dep, 1, DART_PRIO_LOW);

  return DART_OK;
}


static void
dart_tasking_copyin_send_taskfn(void *data)
{
  struct copyin_taskdata *td = (struct copyin_taskdata*) data;

  //printf("Posting send to unit %d (tag %d, size %zu)\n", td->unit, td->tag, td->num_bytes);
  dart_handle_t handle;
  dart_send_handle(td->src, td->num_bytes, DART_TYPE_BYTE,
                   td->tag, DART_GLOBAL_UNIT_ID(td->unit), &handle);
  int32_t flag;
  dart_test_local(&handle, &flag);
  while (!flag) {
    dart_task_yield(-1);
    dart_test_local(&handle, &flag);
  } while (!flag);

  //printf("Send to unit %d completed (tag %d)\n", td->unit, td->tag);
}

static void
dart_tasking_copyin_recv_taskfn(void *data)
{
  struct copyin_taskdata *td = (struct copyin_taskdata*) data;

  if (td->src == NULL) {
    //printf("Posting recv from unit %d (tag %d, size %zu)\n",
    //       td->unit, td->tag, td->num_bytes);

    dart_handle_t handle;
    dart_recv_handle(td->dst, td->num_bytes, DART_TYPE_BYTE,
                    td->tag, DART_GLOBAL_UNIT_ID(td->unit), &handle);
    int flag;
    dart_test_local(&handle, &flag);
    while (!flag) {
      dart_task_yield(-1);
      dart_test_local(&handle, &flag);
    } while (!flag);
    //printf("Recv from unit %d completed (tag %d)\n", td->unit, td->tag);

  } else {
    //printf("Local memcpy of size %zu: %p -> %p\n", td->num_bytes, td->src, td->dst);
    memcpy(td->dst, td->src, td->num_bytes);
  }

}
