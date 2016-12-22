/**
 * \file dart_communication.c
 *
 * Implementations of all the dart communication operations.
 *
 * All the following functions are implemented with the underling *MPI-3*
 * one-sided runtime system.
 */

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_initialization.h>
#include <dash/dart/if/dart_globmem.h>
#include <dash/dart/if/dart_team_group.h>
#include <dash/dart/if/dart_communication.h>

#include <dash/dart/mpi/dart_communication_priv.h>
#include <dash/dart/mpi/dart_team_private.h>
#include <dash/dart/mpi/dart_mem.h>
#include <dash/dart/mpi/dart_mpi_util.h>
#include <dash/dart/mpi/dart_segment.h>
#include <dash/dart/mpi/dart_globmem_priv.h>

#include <dash/dart/base/logging.h>
#include <dash/dart/base/math.h>

#include <stdio.h>
#include <mpi.h>
#include <string.h>
#include <limits.h>
#include <math.h>


static int unit_g2l(
  uint16_t             index,
  dart_global_unit_t   abs_id,
  dart_team_unit_t   * rel_id)
{
  if (index == 0) {
    rel_id->id = abs_id.id;
  }
  else {
    MPI_Comm comm;
    MPI_Group group, group_all;
    comm = dart_team_data[index].comm;
    MPI_Comm_group(comm, &group);
    MPI_Comm_group(MPI_COMM_WORLD, &group_all);
    int mpi_rel_id;
    MPI_Group_translate_ranks (group_all, 1, &abs_id.id, group, &mpi_rel_id);
    rel_id->id = mpi_rel_id;
  }
  return 0;
}

#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
static dart_ret_t get_shared_mem(dart_team_data_t * team_data,
                          void             * dest,
                          dart_gptr_t        gptr,
                          size_t             nelem,
                          dart_datatype_t    dtype)
{
  int16_t      seg_id            = gptr.segid;
  uint64_t     offset            = gptr.addr_or_offs.offset;
  DART_LOG_DEBUG("dart_get: shared windows enabled");
  dart_team_unit_t luid = DART_TEAM_UNIT_ID(gptr.unitid);
  char * baseptr;
  /*
   * Use memcpy if the target is in the same node as the calling unit:
   */
  DART_LOG_DEBUG("dart_get: shared memory segment, seg_id:%d",
                 seg_id);
  if (seg_id) {
    if (dart_segment_get_baseptr(seg_id, luid, &baseptr) != DART_OK) {
      DART_LOG_ERROR("dart_get ! "
                     "dart_adapt_transtable_get_baseptr failed");
      return DART_ERR_INVAL;
    }
  } else {
    baseptr = dart_sharedmem_local_baseptr_set[luid.id];
  }
  baseptr += offset;
  DART_LOG_DEBUG("dart_get: memcpy %zu bytes", nelem * dart_mpi_sizeof_datatype(dtype));
  memcpy((char*)dest, baseptr, nelem * dart_mpi_sizeof_datatype(dtype));
  return DART_OK;
}
#endif // !defined(DART_MPI_DISABLE_SHARED_WINDOWS)

dart_ret_t dart_get(
  void            * dest,
  dart_gptr_t       gptr,
  size_t            nelem,
  dart_datatype_t   dtype)
{
  MPI_Aint     disp_s,
               disp_rel;
  MPI_Win      win;
  MPI_Datatype mpi_dtype         = dart_mpi_datatype(dtype);
  dart_global_unit_t  target_unitid_abs = DART_GLOBAL_UNIT_ID(gptr.unitid);
  dart_team_unit_t    target_unitid_rel = DART_TEAM_UNIT_ID(target_unitid_abs.id);
  uint64_t     offset            = gptr.addr_or_offs.offset;
  int16_t      seg_id            = gptr.segid;

  /*
   * MPI uses offset type int, do not copy more than INT_MAX elements:
   */
  if (nelem > INT_MAX) {
    DART_LOG_ERROR("dart_get ! failed: nelem > INT_MAX");
    return DART_ERR_INVAL;
  }

  uint16_t index;
  if (dart_segment_get_teamidx(seg_id, &index) != DART_OK) {
    DART_LOG_ERROR("dart_get ! failed: Unknown segment %i!", seg_id);
    return DART_ERR_INVAL;
  }

  dart_team_data_t *team_data = &dart_team_data[index];

  if (seg_id) {
    unit_g2l(index, target_unitid_abs, &target_unitid_rel);
  }

  DART_LOG_DEBUG("dart_get() uid_abs:%d uid_rel:%d "
                 "o:%"PRIu64" s:%d i:%u nelem:%zu",
                 target_unitid_abs, target_unitid_rel,
                 offset, seg_id, index, nelem);

#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
  DART_LOG_DEBUG("dart_get: shared windows enabled");
  if (seg_id >= 0 && team_data->sharedmem_tab[gptr.unitid].id >= 0) {
    return get_shared_mem(team_data, dest, gptr, nelem, dtype);
  }
#else
  DART_LOG_DEBUG("dart_get: shared windows disabled");
#endif // !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
  /*
   * MPI shared windows disabled or target and calling unit are on different
   * nodes, use MPI_Get:
   */
  if (seg_id) {
    if (dart_segment_get_disp(
          seg_id,
          target_unitid_rel,
          &disp_s) != DART_OK) {
      return DART_ERR_INVAL;
    }
    win = team_data->window;
    disp_rel = disp_s + offset;
    DART_LOG_TRACE("dart_get:  nelem:%zu "
                   "source (coll.): win:%"PRIu64" unit:%d disp:%"PRId64" "
                   "-> dest:%p",
                   nelem, (unsigned long)win, target_unitid_rel, disp_rel, dest);
  } else {
    win      = dart_win_local_alloc;
    disp_rel = offset;
    DART_LOG_TRACE("dart_get:  nelem:%zu "
                   "source (local): win:%"PRIu64" unit:%d disp:%"PRId64" "
                   "-> dest:%p",
                   nelem, (unsigned long)win, target_unitid_rel, disp_rel, dest);
  }
  DART_LOG_TRACE("dart_get:  MPI_Get");
  if (MPI_Get(dest,
              nelem,
              mpi_dtype,
              target_unitid_rel.id,
              disp_rel,
              nelem,
              mpi_dtype,
              win)
      != MPI_SUCCESS) {
    DART_LOG_ERROR("dart_get ! MPI_Rget failed");
    return DART_ERR_INVAL;
  }

  DART_LOG_DEBUG("dart_get > finished");
  return DART_OK;
}

dart_ret_t dart_put(
  dart_gptr_t       gptr,
  const void      * src,
  size_t            nelem,
  dart_datatype_t   dtype)
{
  MPI_Aint     disp_s,
               disp_rel;
  MPI_Win      win;
  MPI_Datatype mpi_dtype = dart_mpi_datatype(dtype);
  dart_global_unit_t target_unitid_abs = DART_GLOBAL_UNIT_ID(gptr.unitid);
  uint64_t offset   = gptr.addr_or_offs.offset;
  int16_t  seg_id   = gptr.segid;

  /*
   * MPI uses offset type int, do not copy more than INT_MAX elements:
   */
  if (nelem > INT_MAX) {
    DART_LOG_ERROR("dart_put ! failed: nelem > INT_MAX");
    return DART_ERR_INVAL;
  }

  if (seg_id) {

    uint16_t index;
    if (dart_segment_get_teamidx(seg_id, &index) != DART_OK) {
      DART_LOG_ERROR("dart_put ! failed: Unknown segment %i!", seg_id);
      return DART_ERR_INVAL;
    }

    dart_team_unit_t target_unitid_rel;
    win = dart_team_data[index].window;
    unit_g2l(index, target_unitid_abs, &target_unitid_rel);
    if (dart_segment_get_disp(
          seg_id,
          target_unitid_rel,
          &disp_s) != DART_OK) {
      return DART_ERR_INVAL;
    }

    disp_rel = disp_s + offset;
    MPI_Put(
      src,
      nelem,
      mpi_dtype,
      target_unitid_rel.id,
      disp_rel,
      nelem,
      mpi_dtype,
      win);
    DART_LOG_DEBUG("dart_put: nelem:%zu (from collective allocation) "
                   "target unit: %d offset: %"PRIu64"",
                   nelem, target_unitid_abs.id, offset);
  } else {
    win = dart_win_local_alloc;
    MPI_Put(
      src,
      nelem,
      mpi_dtype,
      target_unitid_abs.id,
      offset,
      nelem,
      mpi_dtype,
      win);
    DART_LOG_DEBUG("dart_put: nelem:%zu (from local allocation) "
                   "target unit: %d offset: %"PRIu64"",
                   nelem, target_unitid_abs.id, offset);
  }
  return DART_OK;
}

dart_ret_t dart_accumulate(
  dart_gptr_t      gptr,
  const void     * values,
  size_t           nelem,
  dart_datatype_t  dtype,
  dart_operation_t op,
  dart_team_t      team)
{
  MPI_Aint     disp_s,
               disp_rel;
  MPI_Datatype mpi_dtype;
  MPI_Op       mpi_op;
  dart_global_unit_t  target_unitid_abs = DART_GLOBAL_UNIT_ID(gptr.unitid);
  uint64_t offset   = gptr.addr_or_offs.offset;
  int16_t  seg_id   = gptr.segid;
  mpi_dtype         = dart_mpi_datatype(dtype);
  mpi_op            = dart_mpi_op(op);

  (void)(team); // To prevent compiler warning from unused parameter.

  DART_LOG_DEBUG("dart_accumulate() nelem:%zu dtype:%d op:%d unit:%d",
                 nelem, dtype, op, target_unitid_abs);

  /*
   * MPI uses offset type int, do not copy more than INT_MAX elements:
   */
  if (nelem > INT_MAX) {
    DART_LOG_ERROR("dart_accumulate ! failed: nelem > INT_MAX");
    return DART_ERR_INVAL;
  }

  if (seg_id) {
    dart_team_unit_t target_unitid_rel;

    uint16_t index;
    if (dart_segment_get_teamidx(seg_id, &index) != DART_OK) {
      DART_LOG_ERROR("dart_accumulate ! failed: Unknown segment %i!", seg_id);
      return DART_ERR_INVAL;
    }

    MPI_Win win = dart_team_data[index].window;
    unit_g2l(index,
             target_unitid_abs,
             &target_unitid_rel);
    if (dart_segment_get_disp(
          seg_id,
          target_unitid_rel,
          &disp_s) != DART_OK) {
      DART_LOG_ERROR("dart_accumulate ! "
                     "dart_adapt_transtable_get_disp failed");
      return DART_ERR_INVAL;
    }
    disp_rel = disp_s + offset;
    MPI_Accumulate(
      values,            // Origin address
      nelem,             // Number of entries in buffer
      mpi_dtype,         // Data type of each buffer entry
      target_unitid_rel.id, // Rank of target
      disp_rel,          // Displacement from start of window to beginning
                         // of target buffer
      nelem,             // Number of entries in target buffer
      mpi_dtype,         // Data type of each entry in target buffer
      mpi_op,            // Reduce operation
      win);
    DART_LOG_TRACE("dart_accumulate:  nelem:%zu (from collective allocation) "
                   "target unit: %d offset: %"PRIu64"",
                   nelem, target_unitid_abs.id, offset);
  } else {
    MPI_Win win = dart_win_local_alloc;
    MPI_Accumulate(
      values,            // Origin address
      nelem,             // Number of entries in buffer
      mpi_dtype,         // Data type of each buffer entry
      target_unitid_abs.id, // Rank of target
      offset,            // Displacement from start of window to beginning
                         // of target buffer
      nelem,             // Number of entries in target buffer
      mpi_dtype,         // Data type of each entry in target buffer
      mpi_op,            // Reduce operation
      win);
    DART_LOG_TRACE("dart_accumulate:  nelem:%zu (from local allocation) "
                   "target unit: %d offset: %"PRIu64"",
                   nelem, target_unitid_abs.id, offset);
  }
  DART_LOG_DEBUG("dart_accumulate > finished");
  return DART_OK;
}

dart_ret_t dart_fetch_and_op(
  dart_gptr_t      gptr,
  void *           value,
  void *           result,
  dart_datatype_t  dtype,
  dart_operation_t op,
  dart_team_t      team)
{
  MPI_Aint     disp_s,
               disp_rel;
  MPI_Win      win;
  MPI_Datatype mpi_dtype;
  MPI_Op       mpi_op;
  dart_global_unit_t  target_unitid_abs = DART_GLOBAL_UNIT_ID(gptr.unitid);
  uint64_t offset   = gptr.addr_or_offs.offset;
  int16_t  seg_id   = gptr.segid;
  mpi_dtype         = dart_mpi_datatype(dtype);
  mpi_op            = dart_mpi_op(op);

  (void)(team); // To prevent compiler warning from unused parameter.

  DART_LOG_DEBUG("dart_fetch_and_op() dtype:%d op:%d unit:%d",
                 dtype, op, target_unitid_abs.id);
  if (seg_id) {
    dart_team_unit_t target_unitid_rel;

    uint16_t index;
    if (dart_segment_get_teamidx(seg_id, &index) != DART_OK) {
      DART_LOG_ERROR("dart_fetch_and_op ! failed: Unknown segment %i!",
                     seg_id);
      return DART_ERR_INVAL;
    }

    unit_g2l(index,
             target_unitid_abs,
             &target_unitid_rel);
    if (dart_segment_get_disp(
          seg_id,
          target_unitid_rel,
          &disp_s) != DART_OK) {
      DART_LOG_ERROR("dart_fetch_and_op ! "
                     "dart_adapt_transtable_get_disp failed");
      return DART_ERR_INVAL;
    }
    disp_rel = disp_s + offset;
    win = dart_team_data[index].window;
    MPI_Fetch_and_op(
      value,             // Origin address
      result,            // Result address
      mpi_dtype,         // Data type of each buffer entry
      target_unitid_rel.id, // Rank of target
      disp_rel,          // Displacement from start of window to beginning
                         // of target buffer
      mpi_op,            // Reduce operation
      win);
    DART_LOG_TRACE("dart_fetch_and_op:  (from coll. allocation) "
                   "target unit: %d offset: %"PRIu64"",
                   target_unitid_abs.id, offset);
  } else {
    win = dart_win_local_alloc;
    MPI_Fetch_and_op(
      value,             // Origin address
      result,            // Result address
      mpi_dtype,         // Data type of each buffer entry
      target_unitid_abs.id, // Rank of target
      offset,            // Displacement from start of window to beginning
                         // of target buffer
      mpi_op,            // Reduce operation
      win);
    DART_LOG_TRACE("dart_fetch_and_op:  (from local allocation) "
                   "target unit: %d offset: %"PRIu64"",
                   target_unitid_abs.id, offset);
  }
  DART_LOG_DEBUG("dart_fetch_and_op > finished");
  return DART_OK;
}

/* -- Non-blocking dart one-sided operations -- */

dart_ret_t dart_get_handle(
  void          * dest,
  dart_gptr_t     gptr,
  size_t          nelem,
  dart_datatype_t dtype,
  dart_handle_t * handle)
{
  MPI_Request  mpi_req;
  MPI_Aint     disp_s,
               disp_rel;
  MPI_Datatype mpi_type = dart_mpi_datatype(dtype);
  MPI_Win      win;
  dart_global_unit_t  target_unitid_abs = DART_GLOBAL_UNIT_ID(gptr.unitid);
  dart_team_unit_t    target_unitid_rel = DART_TEAM_UNIT_ID(target_unitid_abs.id);
  int          mpi_ret;
  uint64_t     offset = gptr.addr_or_offs.offset;
  int16_t      seg_id = gptr.segid;

  *handle = NULL;

  /*
   * MPI uses offset type int, do not copy more than INT_MAX elements:
   */
  if (nelem > INT_MAX) {
    DART_LOG_ERROR("dart_get_handle ! failed: nelem > INT_MAX");
    return DART_ERR_INVAL;
  }

  uint16_t index;
  if (dart_segment_get_teamidx(seg_id, &index) != DART_OK) {
    DART_LOG_ERROR("dart_get_handle ! failed: Unknown segment %i!", seg_id);
    return DART_ERR_INVAL;
  }

  dart_team_data_t *team_data = &dart_team_data[index];

  *handle = (dart_handle_t) malloc(sizeof(struct dart_handle_struct));

  if (seg_id > 0) {
    unit_g2l(index, target_unitid_abs, &target_unitid_rel);
  }
  DART_LOG_DEBUG("dart_get_handle() uid_abs:%d uid_rel:%d "
                 "o:%"PRIu64" s:%d i:%d, nelem:%zu",
                 target_unitid_abs, target_unitid_rel,
                 offset, seg_id, index, nelem);
  DART_LOG_TRACE("dart_get_handle:  allocated handle:%p", (void *)(*handle));

#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
  DART_LOG_DEBUG("dart_get_handle: shared windows enabled");

  if (seg_id >= 0 && team_data->sharedmem_tab[gptr.unitid].id >= 0) {
    dart_ret_t ret = get_shared_mem(team_data, dest, gptr, nelem, dtype);

    /*
     * Mark request as completed:
     */
    (*handle)->request = MPI_REQUEST_NULL;
    if (seg_id != 0) {
      (*handle)->dest = target_unitid_rel.id;
      (*handle)->win = team_data->window;
    } else {
      (*handle)->dest = target_unitid_abs.id;
      (*handle)->win  = dart_win_local_alloc;
    }
    return ret;
  }
#else
  DART_LOG_DEBUG("dart_get_handle: shared windows disabled");
#endif /* !defined(DART_MPI_DISABLE_SHARED_WINDOWS) */
  /*
   * MPI shared windows disabled or target and calling unit are on different
   * nodes, use MPI_RGet:
   */
  if (seg_id != 0) {
    /*
     * The memory accessed is allocated with collective allocation.
     */
    DART_LOG_TRACE("dart_get_handle:  collective, segment:%d", seg_id);
    win = team_data->window;
    /* Translate local unitID (relative to teamid) into global unitID
     * (relative to DART_TEAM_ALL).
     *
     * Note: target_unitid should not be the global unitID but rather the
     * local unitID relative to the team associated with the specified win
     * object.
     */
    if (dart_segment_get_disp(
          seg_id,
          target_unitid_rel,
          &disp_s) != DART_OK) {
      DART_LOG_ERROR(
        "dart_get_handle ! dart_adapt_transtable_get_disp failed");
      return DART_ERR_INVAL;
    }
    disp_rel = disp_s + offset;
    DART_LOG_TRACE("dart_get_handle:  -- disp_s:%"PRId64" disp_rel:%"PRId64"",
                   disp_s, disp_rel);

    /* TODO: Check if
     *    MPI_Rget_accumulate(
     *      NULL, 0, MPI_BYTE, dest, nbytes, MPI_BYTE,
     *      target_unitid, disp_rel, nbytes, MPI_BYTE, MPI_NO_OP, win,
     *      &mpi_req)
     *  ... could be an better alternative?
     */
    DART_LOG_DEBUG("dart_get_handle:  -- %d elements (collective allocation) "
                   "from %d at offset %"PRIu64"",
                   nelem, target_unitid_rel.id, offset);
    DART_LOG_DEBUG("dart_get_handle:  -- MPI_Rget");
    mpi_ret = MPI_Rget(
                dest,              // origin address
                nelem,             // origin count
                mpi_type,          // origin data type
                target_unitid_rel.id, // target rank
                disp_rel,          // target disp in window
                nelem,             // target count
                mpi_type,          // target data type
                win,               // window
                &mpi_req);
    if (mpi_ret != MPI_SUCCESS) {
      DART_LOG_ERROR("dart_get_handle ! MPI_Rget failed");
      return DART_ERR_INVAL;
    }
    (*handle)->dest = target_unitid_rel.id;
  } else {
    /*
     * The memory accessed is allocated with local allocation.
     */
    DART_LOG_TRACE("dart_get_handle:  -- local, segment:%d", seg_id);
    DART_LOG_DEBUG("dart_get_handle:  -- %d elements (local allocation) "
                   "from %d at offset %"PRIu64"",
                   nelem, target_unitid_abs, offset);
    win     = dart_win_local_alloc;
    DART_LOG_DEBUG("dart_get_handle:  -- MPI_Rget");
    mpi_ret = MPI_Rget(
                dest,              // origin address
                nelem,             // origin count
                mpi_type,          // origin data type
                target_unitid_abs.id, // target rank
                offset,            // target disp in window
                nelem,             // target count
                mpi_type,          // target data type
                win,               // window
                &mpi_req);
    if (mpi_ret != MPI_SUCCESS) {
      DART_LOG_ERROR("dart_get_handle ! MPI_Rget failed");
      return DART_ERR_INVAL;
    }
    (*handle)->dest = target_unitid_abs.id;
  }
  (*handle)->request = mpi_req;
  (*handle)->win     = win;
  DART_LOG_TRACE("dart_get_handle > handle(%p) dest:%d win:%"PRIu64" req:%ld",
                 (void*)(*handle), (*handle)->dest,
                 (unsigned long)win, (long)mpi_req);
  return DART_OK;
}

dart_ret_t dart_put_handle(
  dart_gptr_t       gptr,
  const void      * src,
  size_t            nelem,
  dart_datatype_t   dtype,
  dart_handle_t   * handle)
{
  MPI_Request  mpi_req;
  MPI_Datatype mpi_type = dart_mpi_datatype(dtype);
  MPI_Aint     disp_s,
               disp_rel;
  dart_global_unit_t  target_unitid_abs = DART_GLOBAL_UNIT_ID(gptr.unitid);
  uint64_t     offset   = gptr.addr_or_offs.offset;
  int16_t      seg_id   = gptr.segid;
  MPI_Win      win;

  *handle = NULL;

  /*
   * MPI uses offset type int, do not copy more than INT_MAX elements:
   */
  if (nelem > INT_MAX) {
    DART_LOG_ERROR("dart_put_handle ! failed: nelem > INT_MAX");
    return DART_ERR_INVAL;
  }

  *handle = (dart_handle_t) malloc(sizeof(struct dart_handle_struct));

  if (seg_id != 0) {

    uint16_t index;
    if (dart_segment_get_teamidx(seg_id, &index) != DART_OK) {
      DART_LOG_ERROR("dart_put_handle ! failed: Unknown segment %i!", seg_id);
      return DART_ERR_INVAL;
    }

    dart_team_unit_t target_unitid_rel;
    win = dart_team_data[index].window;
    unit_g2l(index, target_unitid_abs, &target_unitid_rel);
    if (dart_segment_get_disp(
          seg_id,
          target_unitid_rel,
          &disp_s) != DART_OK) {
      return DART_ERR_INVAL;
    }
    disp_rel = disp_s + offset;
    /**
     * TODO: Check if
     *   MPI_Raccumulate(
     *     src, nbytes, MPI_BYTE, target_unitid,
     *     disp_rel, nbytes, MPI_BYTE,
     *     REPLACE, win, &mpi_req)
     * ... could be a better alternative?
     */
    DART_LOG_DEBUG("dart_put_handle: MPI_RPut");
    MPI_Rput(
      src,
      nelem,
      mpi_type,
      target_unitid_rel.id,
      disp_rel,
      nelem,
      mpi_type,
      win,
      &mpi_req);
    (*handle) -> dest = target_unitid_rel.id;
    DART_LOG_DEBUG("dart_put_handle: nelem:%zu dtype:%d"
                   "(from collective allocation) "
                   "target_unit:%d offset:%"PRIu64"",
                   nelem, dtype, target_unitid_abs.id, offset);
  } else {
    DART_LOG_DEBUG("dart_put_handle: MPI_RPut");
    win = dart_win_local_alloc;
    MPI_Rput(
      src,
      nelem,
      mpi_type,
      target_unitid_abs.id,
      offset,
      nelem,
      mpi_type,
      win,
      &mpi_req);
    DART_LOG_DEBUG("dart_put_handle: nlem:%zu dtype:%d"
                   "(from local allocation) "
                   "target_unit:%d offset:%"PRIu64"",
                   nelem, dtype, target_unitid_abs, offset);
    (*handle) -> dest = target_unitid_abs.id;
  }
  (*handle) -> request = mpi_req;
  (*handle) -> win     = win;
  return DART_OK;
}

/* -- Blocking dart one-sided operations -- */

/**
 * \todo Check if MPI_Get_accumulate (MPI_NO_OP) yields better performance
 */
dart_ret_t dart_put_blocking(
  dart_gptr_t     gptr,
  const void    * src,
  size_t          nelem,
  dart_datatype_t dtype)
{
  MPI_Win      win;
  MPI_Aint     disp_s,
               disp_rel;
  MPI_Datatype mpi_dtype         = dart_mpi_datatype(dtype);
  dart_global_unit_t  target_unitid_abs = DART_GLOBAL_UNIT_ID(gptr.unitid);
  dart_team_unit_t    target_unitid_rel = DART_TEAM_UNIT_ID(gptr.unitid);
  uint64_t     offset = gptr.addr_or_offs.offset;
  int16_t      seg_id = gptr.segid;

  /*
   * MPI uses offset type int, do not copy more than INT_MAX elements:
   */
  if (nelem > INT_MAX) {
    DART_LOG_ERROR("dart_put_blocking ! failed: nelem > INT_MAX");
    return DART_ERR_INVAL;
  }

  uint16_t index;
  if (dart_segment_get_teamidx(seg_id, &index) != DART_OK) {
    DART_LOG_ERROR("dart_put_blocking ! failed: Unknown segment %i!", seg_id);
    return DART_ERR_INVAL;
  }


  if (seg_id > 0) {
    unit_g2l(index, target_unitid_abs, &target_unitid_rel);
  }

  DART_LOG_DEBUG("dart_put_blocking() uid_abs:%d uid_rel:%d "
                 "o:%"PRIu64" s:%d i:%d, nelem:%zu",
                 target_unitid_abs, target_unitid_rel,
                 offset, seg_id, index, nelem);

#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
  DART_LOG_DEBUG("dart_put_blocking: shared windows enabled");
  if (seg_id >= 0) {
    /*
     * Use memcpy if the target is in the same node as the calling unit:
     * The value of i will be the target's relative ID in teamid.
     */
    dart_team_unit_t luid = dart_team_data[index].sharedmem_tab[gptr.unitid];
    if (luid.id >= 0) {
      char * baseptr;
      DART_LOG_DEBUG("dart_put_blocking: shared memory segment, seg_id:%d",
                     seg_id);
      if (seg_id) {
        if (dart_segment_get_baseptr(seg_id, luid, &baseptr) != DART_OK) {
          DART_LOG_ERROR("dart_put_blocking ! "
                         "dart_adapt_transtable_get_baseptr failed");
          return DART_ERR_INVAL;
        }
      } else {
        baseptr = dart_sharedmem_local_baseptr_set[luid.id];
      }
      baseptr += offset;
      DART_LOG_DEBUG("dart_put_blocking: memcpy %zu bytes", nelem * dart_mpi_sizeof_datatype(dtype));
      memcpy(baseptr, (char*)src, nelem * dart_mpi_sizeof_datatype(dtype));
      return DART_OK;
    }
  }
#else
  DART_LOG_DEBUG("dart_put_blocking: shared windows disabled");
#endif /* !defined(DART_MPI_DISABLE_SHARED_WINDOWS) */
  /*
   * MPI shared windows disabled or target and calling unit are on different
   * nodes, use MPI_Rput:
   */
  if (seg_id) {
    if (dart_segment_get_disp(
          seg_id,
          target_unitid_rel,
          &disp_s) != DART_OK) {
      DART_LOG_ERROR("dart_put_blocking ! "
                     "dart_adapt_transtable_get_disp failed");
      return DART_ERR_INVAL;
    }
    win = dart_team_data[index].window;
    disp_rel = disp_s + offset;
    DART_LOG_DEBUG("dart_put_blocking:  nelem:%zu "
                   "target (coll.): win:%"PRIu64" unit:%d offset:%"PRIu64" "
                   "<- source: %p",
                   nelem, (unsigned long)win, target_unitid_rel.id,
                   (unsigned long)disp_rel, src);
  } else {
    win      = dart_win_local_alloc;
    disp_rel = offset;
    DART_LOG_DEBUG("dart_put_blocking:  nelem:%zu "
                   "target (local): win:%"PRIu64" unit:%d offset:%"PRIu64" "
                   "<- source: %p",
                   nelem, (unsigned long)win, target_unitid_rel.id,
                   (unsigned long)disp_rel, src);
  }

  /*
   * Using MPI_Put as MPI_Win_flush is required to ensure remote completion.
   */
  DART_LOG_DEBUG("dart_put_blocking: MPI_Put");
  if (MPI_Put(src,
               nelem,
               mpi_dtype,
               target_unitid_rel.id,
               disp_rel,
               nelem,
               mpi_dtype,
               win)
      != MPI_SUCCESS) {
    DART_LOG_ERROR("dart_put_blocking ! MPI_Put failed");
    return DART_ERR_INVAL;
  }
  DART_LOG_DEBUG("dart_put_blocking: MPI_Win_flush");
  if (MPI_Win_flush(target_unitid_rel.id, win) != MPI_SUCCESS) {
    DART_LOG_ERROR("dart_put_blocking ! MPI_Win_flush failed");
    return DART_ERR_INVAL;
  }

  DART_LOG_DEBUG("dart_put_blocking > finished");
  return DART_OK;
}

/**
 * \todo Check if MPI_Accumulate (REPLACE) yields better performance
 */
dart_ret_t dart_get_blocking(
  void          * dest,
  dart_gptr_t     gptr,
  size_t          nelem,
  dart_datatype_t dtype)
{
  MPI_Win      win;
  MPI_Aint     disp_s,
               disp_rel;
  MPI_Datatype mpi_dtype         = dart_mpi_datatype(dtype);
  dart_global_unit_t  target_unitid_abs = DART_GLOBAL_UNIT_ID(gptr.unitid);
  dart_team_unit_t    target_unitid_rel = DART_TEAM_UNIT_ID(gptr.unitid);
  uint64_t     offset            = gptr.addr_or_offs.offset;
  int16_t      seg_id            = gptr.segid;

  /*
   * MPI uses offset type int, do not copy more than INT_MAX elements:
   */
  if (nelem > INT_MAX) {
    DART_LOG_ERROR("dart_get_blocking ! failed: nelem > INT_MAX");
    return DART_ERR_INVAL;
  }

  uint16_t index;
  if (dart_segment_get_teamidx(seg_id, &index) != DART_OK) {
    DART_LOG_ERROR("dart_get_blocking ! failed: Unknown segment %i!", seg_id);
    return DART_ERR_INVAL;
  }

  if (seg_id) {
    unit_g2l(index, target_unitid_abs, &target_unitid_rel);
  }

  dart_team_data_t *team_data = &dart_team_data[index];

  DART_LOG_DEBUG("dart_get_blocking() uid_abs:%d uid_rel:%d "
                 "o:%"PRIu64" s:%d i:%u, nelem:%zu",
                 target_unitid_abs.id, target_unitid_rel.id,
                 offset, seg_id, index, nelem);

#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
  DART_LOG_DEBUG("dart_get_blocking: shared windows enabled");
  if (seg_id >= 0 && team_data->sharedmem_tab[gptr.unitid].id >= 0) {
    return get_shared_mem(team_data, dest, gptr, nelem, dtype);
  }
#else
  DART_LOG_DEBUG("dart_get_blocking: shared windows disabled");
#endif /* !defined(DART_MPI_DISABLE_SHARED_WINDOWS) */
  /*
   * MPI shared windows disabled or target and calling unit are on different
   * nodes, use MPI_Rget:
   */
  if (seg_id) {
    if (dart_segment_get_disp(
          seg_id,
          target_unitid_rel,
          &disp_s) != DART_OK) {
      DART_LOG_ERROR("dart_get_blocking ! "
                     "dart_adapt_transtable_get_disp failed");
      return DART_ERR_INVAL;
    }
    win = team_data->window;
    disp_rel = disp_s + offset;
    DART_LOG_DEBUG("dart_get_blocking:  nelem:%zu "
                   "source (coll.): win:%p unit:%d offset:%p"
                   "-> dest: %p",
                   nelem, (void*)((unsigned long)win), target_unitid_rel.id,
                   (void*)disp_rel, dest);
  } else {
    win      = dart_win_local_alloc;
    disp_rel = offset;
    DART_LOG_DEBUG("dart_get_blocking:  nelem:%zu "
                   "source (local): win:%p unit:%d offset:%p "
                   "-> dest: %p",
                   nelem, (void*)((unsigned long)win), target_unitid_rel.id,
                   (void*)disp_rel, dest);
  }

  /*
   * Using MPI_Get as MPI_Win_flush is required to ensure remote completion.
   */
  DART_LOG_DEBUG("dart_get_blocking: MPI_Get");
  if (MPI_Get(dest,
              nelem,
              mpi_dtype,
              target_unitid_rel.id,
              disp_rel,
              nelem,
              mpi_dtype,
              win)
      != MPI_SUCCESS) {
    DART_LOG_ERROR("dart_get_blocking ! MPI_Get failed");
    return DART_ERR_INVAL;
  }
  DART_LOG_DEBUG("dart_get_blocking: MPI_Win_flush");
  if (MPI_Win_flush(target_unitid_rel.id, win) != MPI_SUCCESS) {
    DART_LOG_ERROR("dart_get_blocking ! MPI_Win_flush failed");
    return DART_ERR_INVAL;
  }

  DART_LOG_DEBUG("dart_get_blocking > finished");
  return DART_OK;
}

/* -- Dart RMA Synchronization Operations -- */

dart_ret_t dart_flush(
  dart_gptr_t gptr)
{
  MPI_Win     win;
  dart_global_unit_t target_unitid_abs = DART_GLOBAL_UNIT_ID(gptr.unitid);
  int16_t     seg_id = gptr.segid;
  DART_LOG_DEBUG("dart_flush() gptr: "
                 "unitid:%d offset:%"PRIu64" segid:%d index:%d",
                 gptr.unitid, gptr.addr_or_offs.offset,
                 gptr.segid,  gptr.flags);
  if (seg_id) {
    dart_team_unit_t target_unitid_rel;

    uint16_t index;
    if (dart_segment_get_teamidx(seg_id, &index) != DART_OK) {
      DART_LOG_ERROR("dart_flush ! failed: Unknown segment %i!", seg_id);
      return DART_ERR_INVAL;
    }

    win = dart_team_data[index].window;
    unit_g2l(index, target_unitid_abs, &target_unitid_rel);
    DART_LOG_TRACE("dart_flush: MPI_Win_flush");
    MPI_Win_flush(target_unitid_rel.id, win);
  } else {
    win = dart_win_local_alloc;
    DART_LOG_TRACE("dart_flush: MPI_Win_flush");
    MPI_Win_flush(target_unitid_abs.id, win);
  }
  DART_LOG_TRACE("dart_flush: MPI_Win_sync");
  if (MPI_Win_sync(win) != MPI_SUCCESS) {
    return DART_ERR_OTHER;
  }
  DART_LOG_DEBUG("dart_flush > finished");
  return DART_OK;
}

dart_ret_t dart_flush_all(
  dart_gptr_t gptr)
{
  int16_t seg_id;
  seg_id = gptr.segid;
  MPI_Win win;
  DART_LOG_DEBUG("dart_flush_all() gptr: "
                 "unitid:%d offset:%"PRIu64" segid:%d index:%d",
                 gptr.unitid, gptr.addr_or_offs.offset,
                 gptr.segid,  gptr.flags);
  if (seg_id) {

    uint16_t index;
    if (dart_segment_get_teamidx(seg_id, &index) != DART_OK) {
      DART_LOG_ERROR("dart_flush_all ! failed: Unknown segment %i!", seg_id);
      return DART_ERR_INVAL;
    }

    win = dart_team_data[index].window;
  } else {
    win = dart_win_local_alloc;
  }
  DART_LOG_TRACE("dart_flush_all: MPI_Win_sync");
  if (MPI_Win_sync(win) != MPI_SUCCESS) {
    return DART_ERR_OTHER;
  }
  DART_LOG_TRACE("dart_flush_all: MPI_Win_flush_all");
  if (MPI_Win_flush_all(win) != MPI_SUCCESS) {
    return DART_ERR_OTHER;
  }
  DART_LOG_TRACE("dart_flush_all: MPI_Win_sync");
  if (MPI_Win_sync(win) != MPI_SUCCESS) {
    return DART_ERR_OTHER;
  }
  DART_LOG_DEBUG("dart_flush_all > finished");
  return DART_OK;
}

dart_ret_t dart_flush_local(
  dart_gptr_t gptr)
{
  dart_global_unit_t target_unitid_abs = DART_GLOBAL_UNIT_ID(gptr.unitid);
  int16_t seg_id = gptr.segid;
  MPI_Win win;
  DART_LOG_DEBUG("dart_flush_local() gptr: "
                 "unitid:%d offset:%"PRIu64" segid:%d index:%d",
                 gptr.unitid, gptr.addr_or_offs.offset,
                 gptr.segid,  gptr.flags);
  if (seg_id) {
    uint16_t index;
    if (dart_segment_get_teamidx(seg_id, &index) != DART_OK) {
      DART_LOG_ERROR("dart_flush_local ! failed: Unknown segment %i!", seg_id);
      return DART_ERR_INVAL;
    }

    dart_team_unit_t target_unitid_rel;
    win = dart_team_data[index].window;
    DART_LOG_DEBUG("dart_flush_local() win:%"PRIu64" seg:%d unit:%d",
                   (unsigned long)win, seg_id, target_unitid_abs.id);
    unit_g2l(index, target_unitid_abs, &target_unitid_rel);
    DART_LOG_TRACE("dart_flush_local: MPI_Win_flush_local");
    MPI_Win_flush_local(target_unitid_rel.id, win);
  } else {
    win = dart_win_local_alloc;
    DART_LOG_DEBUG("dart_flush_local() lwin:%"PRIu64" seg:%d unit:%d",
                   (unsigned long)win, seg_id, target_unitid_abs.id);
    DART_LOG_TRACE("dart_flush_local: MPI_Win_flush_local");
    MPI_Win_flush_local(target_unitid_abs.id, win);
  }
  DART_LOG_DEBUG("dart_flush_local > finished");
  return DART_OK;
}

dart_ret_t dart_flush_local_all(
  dart_gptr_t gptr)
{
  int16_t seg_id = gptr.segid;
  MPI_Win win;
  DART_LOG_DEBUG("dart_flush_local_all() gptr: "
                 "unitid:%d offset:%"PRIu64" segid:%d index:%d",
                 gptr.unitid, gptr.addr_or_offs.offset,
                 gptr.segid,  gptr.flags);
  if (seg_id) {

    uint16_t index;
    if (dart_segment_get_teamidx(seg_id, &index) != DART_OK) {
      DART_LOG_ERROR("dart_flush_local_all ! failed: Unknown segment %i!", seg_id);
      return DART_ERR_INVAL;
    }

    win = dart_team_data[index].window;
  } else {
    win = dart_win_local_alloc;
  }
  MPI_Win_flush_local_all(win);
  DART_LOG_DEBUG("dart_flush_local_all > finished");
  return DART_OK;
}

dart_ret_t dart_wait_local(
  dart_handle_t handle)
{
  int mpi_ret;
  DART_LOG_DEBUG("dart_wait_local() handle:%p", (void*)(handle));
  if (handle != NULL) {
    DART_LOG_TRACE("dart_wait_local:     handle->dest: %d",
                   handle->dest);
    DART_LOG_TRACE("dart_wait_local:     handle->win:  %p",
                   (void*)(unsigned long)(handle->win));
    DART_LOG_TRACE("dart_wait_local:     handle->req:  %ld",
                   (long)handle->request);
    if (handle->request != MPI_REQUEST_NULL) {
      MPI_Status mpi_sta;
      mpi_ret = MPI_Wait(&(handle->request), &mpi_sta);
      DART_LOG_TRACE("dart_wait_local:        -- mpi_sta.MPI_SOURCE = %d",
                     mpi_sta.MPI_SOURCE);
      DART_LOG_TRACE("dart_wait_local:        -- mpi_sta.MPI_ERROR  = %d (%s)",
                     mpi_sta.MPI_ERROR,
                     DART__MPI__ERROR_STR(mpi_sta.MPI_ERROR));
      if (mpi_ret != MPI_SUCCESS) {
        DART_LOG_DEBUG("dart_wait_local ! MPI_Wait failed");
        return DART_ERR_INVAL;
      }
    } else {
      DART_LOG_TRACE("dart_wait_local:     handle->req == MPI_REQUEST_NULL");
    }
    /*
     * Do not free handle resource, it could be needed for a following
     * dart_wait() or dart_wait_all() to ensure remote completion.
     */
  }
  DART_LOG_DEBUG("dart_wait_local > finished");
  return DART_OK;
}

dart_ret_t dart_wait(
  dart_handle_t handle)
{
  int mpi_ret;
  DART_LOG_DEBUG("dart_wait() handle:%p", (void*)(handle));
  if (handle != NULL) {
    DART_LOG_TRACE("dart_wait_local:     handle->dest: %d",
                   handle->dest);
    DART_LOG_TRACE("dart_wait_local:     handle->win:  %"PRIu64"",
                   (unsigned long)handle->win);
    DART_LOG_TRACE("dart_wait_local:     handle->req:  %ld",
                   (unsigned long)handle->request);
    if (handle->request != MPI_REQUEST_NULL) {
      MPI_Status mpi_sta;
      DART_LOG_DEBUG("dart_wait:     -- MPI_Wait");
      mpi_ret = MPI_Wait(&(handle->request), &mpi_sta);
      DART_LOG_TRACE("dart_wait:        -- mpi_sta.MPI_SOURCE: %d",
                     mpi_sta.MPI_SOURCE);
      DART_LOG_TRACE("dart_wait:        -- mpi_sta.MPI_ERROR:  %d:%s",
                     mpi_sta.MPI_ERROR,
                     DART__MPI__ERROR_STR(mpi_sta.MPI_ERROR));
      if (mpi_ret != MPI_SUCCESS) {
        DART_LOG_DEBUG("dart_wait ! MPI_Wait failed");
        return DART_ERR_INVAL;
      }
      DART_LOG_DEBUG("dart_wait:     -- MPI_Win_flush");
      mpi_ret = MPI_Win_flush(handle->dest, handle->win);
      if (mpi_ret != MPI_SUCCESS) {
        DART_LOG_DEBUG("dart_wait ! MPI_Win_flush failed");
        return DART_ERR_INVAL;
      }
    } else {
      DART_LOG_TRACE("dart_wait:     handle->request: MPI_REQUEST_NULL");
    }
    /* Free handle resource */
    DART_LOG_DEBUG("dart_wait:   free handle %p", (void*)(handle));
    free(handle);
    handle = NULL;
  }
  DART_LOG_DEBUG("dart_wait > finished");
  return DART_OK;
}

dart_ret_t dart_waitall_local(
  dart_handle_t * handle,
  size_t          num_handles)
{
  dart_ret_t ret = DART_OK;

  DART_LOG_DEBUG("dart_waitall_local()");
  if (num_handles == 0) {
    DART_LOG_DEBUG("dart_waitall_local > number of handles = 0");
    return DART_OK;
  }
  if (num_handles > INT_MAX) {
    DART_LOG_ERROR("dart_waitall_local ! number of handles > INT_MAX");
    return DART_ERR_INVAL;
  }
  if (handle != NULL) {
    size_t      i,
                r_n = 0;
    MPI_Status  *mpi_sta;
    MPI_Request *mpi_req;
    mpi_req = (MPI_Request *) malloc(num_handles * sizeof(MPI_Request));
    mpi_sta = (MPI_Status  *) malloc(num_handles * sizeof(MPI_Status));
    for (i = 0; i < num_handles; i++)  {
      if (handle[i] != NULL && handle[i]->request != MPI_REQUEST_NULL) {
        DART_LOG_TRACE("dart_waitall_local: -- handle[%"PRIu64"]: %p)",
                       i, (void*)handle[i]);
        DART_LOG_TRACE("dart_waitall_local:    handle[%"PRIu64"]->dest: %d",
                       i, handle[i]->dest);
        DART_LOG_TRACE("dart_waitall_local:    handle[%"PRIu64"]->win:  %p",
                       i, (void*)((unsigned long)(handle[i]->win)));
        DART_LOG_TRACE("dart_waitall_local:    handle[%"PRIu64"]->req:  %p",
                       i, (void*)((unsigned long)(handle[i]->request)));
        mpi_req[r_n] = handle[i]->request;
        r_n++;
      }
    }
    /*
     * Wait for local completion of MPI requests:
     */
    DART_LOG_DEBUG("dart_waitall_local: "
                   "MPI_Waitall, %"PRIu64" requests from %"PRIu64" handles",
                   r_n, num_handles);
    if (r_n > 0) {
      if (MPI_Waitall(r_n, mpi_req, mpi_sta) == MPI_SUCCESS) {
        DART_LOG_DEBUG("dart_waitall_local: MPI_Waitall completed");
      } else {
        DART_LOG_ERROR("dart_waitall_local: MPI_Waitall failed");
        DART_LOG_TRACE("dart_waitall_local: free MPI_Request temporaries");
        free(mpi_req);
        DART_LOG_TRACE("dart_waitall_local: free MPI_Status temporaries");
        free(mpi_sta);
        return DART_ERR_INVAL;
      }
    } else {
      DART_LOG_DEBUG("dart_waitall_local > number of requests = 0");
      return DART_OK;
    }
    /*
     * copy MPI requests back to DART handles:
     */
    DART_LOG_TRACE("dart_waitall_local: "
                   "copying MPI requests back to DART handles");
    r_n = 0;
    for (i = 0; i < num_handles; i++) {
      if (handle[i]) {
        DART_LOG_TRACE("dart_waitall_local: -- mpi_sta[%"PRIu64"].MPI_SOURCE:"
                       " %d",
                       r_n, mpi_sta[r_n].MPI_SOURCE);
        DART_LOG_TRACE("dart_waitall_local: -- mpi_sta[%"PRIu64"].MPI_ERROR:"
                       "  %d:%s",
                       r_n,
                       mpi_sta[r_n].MPI_ERROR,
                       DART__MPI__ERROR_STR(mpi_sta[r_n].MPI_ERROR));
        if (mpi_req[r_n] != MPI_REQUEST_NULL) {
          if (mpi_sta[r_n].MPI_ERROR == MPI_SUCCESS) {
            DART_LOG_TRACE("dart_waitall_local: -- MPI_Request_free");
            if (MPI_Request_free(&mpi_req[r_n]) != MPI_SUCCESS) {
              DART_LOG_TRACE("dart_waitall_local ! MPI_Request_free failed");
              ret = DART_ERR_INVAL;
              break;
            }
          } else {
            DART_LOG_TRACE("dart_waitall_local: cannot free request %zu "
                           "mpi_sta[%zu] = %d (%s)",
                           r_n,
                           r_n,
                           mpi_sta[r_n].MPI_ERROR,
                           DART__MPI__ERROR_STR(mpi_sta[r_n].MPI_ERROR));
          }
        }
        DART_LOG_DEBUG("dart_waitall_local: free handle[%zu] %p",
                       i, (void*)(handle[i]));
        free(handle[i]);
        handle[i] = NULL;
        r_n++;
      }
    }
    DART_LOG_TRACE("dart_waitall_local: free MPI_Request temporaries");
    free(mpi_req);
    DART_LOG_TRACE("dart_waitall_local: free MPI_Status temporaries");
    free(mpi_sta);
  }
  DART_LOG_DEBUG("dart_waitall_local > %d", ret);
  return ret;
}

dart_ret_t dart_waitall(
  dart_handle_t * handle,
  size_t          n)
{
  size_t i, r_n;
  DART_LOG_DEBUG("dart_waitall()");
  if (n == 0) {
    DART_LOG_ERROR("dart_waitall > number of handles = 0");
    return DART_OK;
  }
  if (n > INT_MAX) {
    DART_LOG_ERROR("dart_waitall ! number of handles > INT_MAX");
    return DART_ERR_INVAL;
  }
  DART_LOG_DEBUG("dart_waitall: number of handles: %zu", n);
  if (handle) {
    MPI_Status  *mpi_sta;
    MPI_Request *mpi_req;
    mpi_req = (MPI_Request *) malloc(n * sizeof(MPI_Request));
    mpi_sta = (MPI_Status *)  malloc(n * sizeof(MPI_Status));
    /*
     * copy requests from DART handles to MPI request array:
     */
    DART_LOG_TRACE("dart_waitall: copying DART handles to MPI request array");
    r_n = 0;
    for (i = 0; i < n; i++) {
      if (handle[i] != NULL) {
        DART_LOG_DEBUG("dart_waitall: -- handle[%zu](%p): "
                       "dest:%d win:%"PRIu64" req:%"PRIu64"",
                       i, (void*)handle[i],
                       handle[i]->dest,
                       (unsigned long)handle[i]->win,
                       (unsigned long)handle[i]->request);
        mpi_req[r_n] = handle[i]->request;
        r_n++;
      }
    }
    /*
     * wait for communication of MPI requests:
     */
    DART_LOG_DEBUG("dart_waitall: MPI_Waitall, %zu requests from %zu handles",
                   r_n, n);
    /* From the MPI 3.1 standard:
     *
     * The i-th entry in array_of_statuses is set to the return
     * status of the i-th operation. Active persistent requests
     * are marked inactive.
     * Requests of any other type are deallocated and the
     * corresponding handles in the array are set to
     * MPI_REQUEST_NULL.
     * The list may contain null or inactive handles.
     * The call sets to empty the status of each such entry.
     */
    if (r_n > 0) {
      if (MPI_Waitall(r_n, mpi_req, mpi_sta) == MPI_SUCCESS) {
        DART_LOG_DEBUG("dart_waitall: MPI_Waitall completed");
      } else {
        DART_LOG_ERROR("dart_waitall: MPI_Waitall failed");
        DART_LOG_TRACE("dart_waitall: free MPI_Request temporaries");
        free(mpi_req);
        DART_LOG_TRACE("dart_waitall: free MPI_Status temporaries");
        free(mpi_sta);
        return DART_ERR_INVAL;
      }
    } else {
      DART_LOG_DEBUG("dart_waitall > number of requests = 0");
      return DART_OK;
    }
    /*
     * copy MPI requests back to DART handles:
     */
    DART_LOG_TRACE("dart_waitall: copying MPI requests back to DART handles");
    r_n = 0;
    for (i = 0; i < n; i++) {
      if (handle[i]) {
        if (mpi_req[r_n] == MPI_REQUEST_NULL) {
          DART_LOG_TRACE("dart_waitall: -- mpi_req[%zu] = MPI_REQUEST_NULL",
                         r_n);
        } else {
          DART_LOG_TRACE("dart_waitall: -- mpi_req[%zu]", r_n);
        }
        DART_LOG_TRACE("dart_waitall: -- mpi_sta[%zu].MPI_SOURCE: %d",
                       r_n, mpi_sta[r_n].MPI_SOURCE);
        DART_LOG_TRACE("dart_waitall: -- mpi_sta[%zu].MPI_ERROR:  %d:%s",
                       r_n,
                       mpi_sta[r_n].MPI_ERROR,
                       DART__MPI__ERROR_STR(mpi_sta[r_n].MPI_ERROR));
        handle[i]->request = mpi_req[r_n];
        r_n++;
      }
    }
    /*
     * wait for completion of MPI requests at origins and targets:
     */
    DART_LOG_DEBUG("dart_waitall: waiting for remote completion");
    for (i = 0; i < n; i++) {
      if (handle[i]) {
        if (handle[i]->request == MPI_REQUEST_NULL) {
          DART_LOG_TRACE("dart_waitall: -- handle[%zu] done (MPI_REQUEST_NULL)",
                         i);
        } else {
          DART_LOG_DEBUG("dart_waitall: -- MPI_Win_flush(handle[%zu]: %p))",
                         i, (void*)handle[i]);
          DART_LOG_TRACE("dart_waitall:      handle[%zu]->dest: %d",
                         i, handle[i]->dest);
          DART_LOG_TRACE("dart_waitall:      handle[%zu]->win:  %"PRIu64"",
                         i, (unsigned long)handle[i]->win);
          DART_LOG_TRACE("dart_waitall:      handle[%zu]->req:  %"PRIu64"",
                         i, (unsigned long)handle[i]->request);
          /*
           * MPI_Win_flush to wait for remote completion:
           */
          if (MPI_Win_flush(handle[i]->dest, handle[i]->win) != MPI_SUCCESS) {
            DART_LOG_ERROR("dart_waitall: MPI_Win_flush failed");
            DART_LOG_TRACE("dart_waitall: free MPI_Request temporaries");
            free(mpi_req);
            DART_LOG_TRACE("dart_waitall: free MPI_Status temporaries");
            free(mpi_sta);
            return DART_ERR_INVAL;
          }
          DART_LOG_TRACE("dart_waitall: -- MPI_Request_free");
          if (MPI_Request_free(&handle[i]->request) != MPI_SUCCESS) {
            DART_LOG_ERROR("dart_waitall: MPI_Request_free failed");
            DART_LOG_TRACE("dart_waitall: free MPI_Request temporaries");
            free(mpi_req);
            DART_LOG_TRACE("dart_waitall: free MPI_Status temporaries");
            free(mpi_sta);
            return DART_ERR_INVAL;
          }
        }
      }
    }
    /*
     * free memory:
     */
    DART_LOG_DEBUG("dart_waitall: free handles");
    for (i = 0; i < n; i++) {
      if (handle[i]) {
        /* Free handle resource */
        DART_LOG_TRACE("dart_waitall: -- free handle[%zu]: %p",
                       i, (void*)(handle[i]));
        free(handle[i]);
        handle[i] = NULL;
      }
    }
    DART_LOG_TRACE("dart_waitall: free MPI_Request temporaries");
    free(mpi_req);
    DART_LOG_TRACE("dart_waitall: free MPI_Status temporaries");
    free(mpi_sta);
  }
  DART_LOG_DEBUG("dart_waitall > finished");
  return DART_OK;
}

dart_ret_t dart_test_local(
  dart_handle_t handle,
  int32_t* is_finished)
{
  DART_LOG_DEBUG("dart_test_local()");
  if (!handle) {
    *is_finished = 1;
    return DART_OK;
  }
  MPI_Status mpi_sta;
  MPI_Test (&(handle->request), is_finished, &mpi_sta);
  DART_LOG_DEBUG("dart_test_local > finished");
  return DART_OK;
}

dart_ret_t dart_testall_local(
  dart_handle_t * handle,
  size_t          n,
  int32_t       * is_finished)
{
  size_t i, r_n;
  DART_LOG_DEBUG("dart_testall_local()");
  MPI_Status *mpi_sta;
  MPI_Request *mpi_req;
  mpi_req = (MPI_Request *)malloc(n * sizeof (MPI_Request));
  mpi_sta = (MPI_Status *)malloc(n * sizeof (MPI_Status));
  r_n = 0;
  for (i = 0; i < n; i++) {
    if (handle[i]){
      mpi_req[r_n] = handle[i] -> request;
      r_n++;
    }
  }
  MPI_Testall(r_n, mpi_req, is_finished, mpi_sta);
  r_n = 0;
  for (i = 0; i < n; i++) {
    if (handle[i]) {
      handle[i] -> request = mpi_req[r_n];
      r_n++;
    }
  }
  free(mpi_req);
  free(mpi_sta);
  DART_LOG_DEBUG("dart_testall_local > finished");
  return DART_OK;
}

/* -- Dart collective operations -- */

static int _dart_barrier_count = 0;

dart_ret_t dart_barrier(
  dart_team_t teamid)
{
  MPI_Comm comm;
  uint16_t index;
  int      result;

  DART_LOG_DEBUG("dart_barrier() barrier count: %d", _dart_barrier_count);
  _dart_barrier_count++;

  result = dart_adapt_teamlist_convert(teamid, &index);
  if (result == -1) {
    return DART_ERR_INVAL;
  }
  /* Fetch proper communicator from teams. */
  comm = dart_team_data[index].comm;
  if (MPI_Barrier(comm) == MPI_SUCCESS) {
    DART_LOG_DEBUG("dart_barrier > finished");
    return DART_OK;
  }
  DART_LOG_DEBUG("dart_barrier ! MPI_Barrier failed");
  return DART_ERR_INVAL;
}

dart_ret_t dart_bcast(
  void              * buf,
  size_t              nelem,
  dart_datatype_t     dtype,
  dart_team_unit_t    root,
  dart_team_t         teamid)
{
  MPI_Comm comm;
  uint16_t index;
  MPI_Datatype mpi_dtype = dart_mpi_datatype(dtype);

  DART_LOG_TRACE("dart_bcast() root:%d team:%d nelem:%"PRIu64"",
                 root.id, teamid, nelem);

  /*
   * MPI uses offset type int, do not copy more than INT_MAX elements:
   */
  if (nelem > INT_MAX) {
    DART_LOG_ERROR("dart_bcast ! failed: nelem > INT_MAX");
    return DART_ERR_INVAL;
  }

  int result = dart_adapt_teamlist_convert(teamid, &index);
  if (result == -1) {
    DART_LOG_ERROR("dart_bcast ! root:%d -> team:%d "
                   "dart_adapt_teamlist_convert failed", root.id, teamid);
    return DART_ERR_INVAL;
  }
  comm = dart_team_data[index].comm;
  if (MPI_Bcast(buf, nelem, mpi_dtype, root.id, comm) != MPI_SUCCESS) {
    DART_LOG_ERROR("dart_bcast ! root:%d -> team:%d "
                   "MPI_Bcast failed", root.id, teamid);
    return DART_ERR_INVAL;
  }
  DART_LOG_TRACE("dart_bcast > root:%d team:%d nelem:%"PRIu64" finished",
                 root, teamid, nelem);
  return DART_OK;
}

dart_ret_t dart_scatter(
  const void        * sendbuf,
  void              * recvbuf,
  size_t              nelem,
  dart_datatype_t     dtype,
  dart_team_unit_t    root,
  dart_team_t         teamid)
{
  MPI_Datatype mpi_dtype = dart_mpi_datatype(dtype);
  MPI_Comm     comm;
  uint16_t     index;

  /*
   * MPI uses offset type int, do not copy more than INT_MAX elements:
   */
  if (nelem > INT_MAX) {
    DART_LOG_ERROR("dart_scatter ! failed: nelem > INT_MAX");
    return DART_ERR_INVAL;
  }

  int result = dart_adapt_teamlist_convert(teamid, &index);
  if (result == -1) {
    return DART_ERR_INVAL;
  }
  comm = dart_team_data[index].comm;
  if (MPI_Scatter(
           sendbuf,
           nelem,
           mpi_dtype,
           recvbuf,
           nelem,
           mpi_dtype,
           root.id,
           comm) != MPI_SUCCESS) {
    return DART_ERR_INVAL;
  }
  return DART_OK;
}

dart_ret_t dart_gather(
  const void         * sendbuf,
  void               * recvbuf,
  size_t               nelem,
  dart_datatype_t      dtype,
  dart_team_unit_t     root,
  dart_team_t          teamid)
{
  MPI_Datatype mpi_dtype = dart_mpi_datatype(dtype);
  MPI_Comm     comm;
  uint16_t     index;

  /*
   * MPI uses offset type int, do not copy more than INT_MAX elements:
   */
  if (nelem > INT_MAX) {
    DART_LOG_ERROR("dart_gather ! failed: nelem > INT_MAX");
    return DART_ERR_INVAL;
  }

  int result = dart_adapt_teamlist_convert(teamid, &index);
  if (result == -1) {
    return DART_ERR_INVAL;
  }
  comm = dart_team_data[index].comm;
  if (MPI_Gather(
           sendbuf,
           nelem,
           mpi_dtype,
           recvbuf,
           nelem,
           mpi_dtype,
           root.id,
           comm) != MPI_SUCCESS) {
    return DART_ERR_INVAL;
  }
  return DART_OK;
}

dart_ret_t dart_allgather(
  const void      * sendbuf,
  void            * recvbuf,
  size_t            nelem,
  dart_datatype_t   dtype,
  dart_team_t       teamid)
{
  MPI_Datatype mpi_dtype = dart_mpi_datatype(dtype);
  MPI_Comm     comm;
  uint16_t     index;
  int          result;
  DART_LOG_TRACE("dart_allgather() team:%d nelem:%"PRIu64"",
                 teamid, nelem);

  /*
   * MPI uses offset type int, do not copy more than INT_MAX elements:
   */
  if (nelem > INT_MAX) {
    DART_LOG_ERROR("dart_allgather ! failed: nelem > INT_MAX");
    return DART_ERR_INVAL;
  }

  result = dart_adapt_teamlist_convert(teamid, &index);
  if (result == -1) {
    DART_LOG_ERROR("dart_allgather ! team:%d "
                   "dart_adapt_teamlist_convert failed", teamid);
    return DART_ERR_INVAL;
  }
  if (sendbuf == recvbuf || NULL == sendbuf) {
    sendbuf = MPI_IN_PLACE;
  }
  comm = dart_team_data[index].comm;
  if (MPI_Allgather(
           sendbuf,
           nelem,
           mpi_dtype,
           recvbuf,
           nelem,
           mpi_dtype,
           comm) != MPI_SUCCESS) {
    DART_LOG_ERROR("dart_allgather ! team:%d nelem:%"PRIu64" failed",
                   teamid, nelem);
    return DART_ERR_INVAL;
  }
  DART_LOG_TRACE("dart_allgather > team:%d nelem:%"PRIu64"",
                 teamid, nelem);
  return DART_OK;
}

dart_ret_t dart_allgatherv(
  const void      * sendbuf,
  size_t            nsendelem,
  dart_datatype_t   dtype,
  void            * recvbuf,
  const size_t    * nrecvcounts,
  const size_t    * recvdispls,
  dart_team_t       teamid)
{
  MPI_Datatype mpi_dtype = dart_mpi_datatype(dtype);
  MPI_Comm     comm;
  uint16_t     index;
  int          result;
  int          comm_size;
  DART_LOG_TRACE("dart_allgatherv() team:%d nsendelem:%"PRIu64"",
                 teamid, nsendelem);

  /*
   * MPI uses offset type int, do not copy more than INT_MAX elements:
   */
  if (nsendelem > INT_MAX) {
    DART_LOG_ERROR("dart_allgather ! failed: nelem > INT_MAX");
    return DART_ERR_INVAL;
  }

  result = dart_adapt_teamlist_convert(teamid, &index);
  if (result == -1) {
    DART_LOG_ERROR("dart_allgatherv ! team:%d "
                   "dart_adapt_teamlist_convert failed", teamid);
    return DART_ERR_INVAL;
  }
  if (sendbuf == recvbuf || NULL == sendbuf) {
    sendbuf = MPI_IN_PLACE;
  }
  comm = dart_team_data[index].comm;

  // convert nrecvcounts and recvdispls
  MPI_Comm_size(comm, &comm_size);
  int *inrecvcounts = malloc(sizeof(int) * comm_size);
  int *irecvdispls  = malloc(sizeof(int) * comm_size);
  for (int i = 0; i < comm_size; i++) {
    if (nrecvcounts[i] > INT_MAX || recvdispls[i] > INT_MAX) {
      DART_LOG_ERROR("dart_allgatherv ! failed: nrecvcounts[%i] > INT_MAX || recvdispls[%i] > INT_MAX", i, i);
      free(inrecvcounts);
      free(irecvdispls);
      return DART_ERR_INVAL;
    }
    inrecvcounts[i] = nrecvcounts[i];
    irecvdispls[i]  = recvdispls[i];
  }

  if (MPI_Allgatherv(
           sendbuf,
           nsendelem,
           mpi_dtype,
           recvbuf,
           inrecvcounts,
           irecvdispls,
           mpi_dtype,
           comm) != MPI_SUCCESS) {
    DART_LOG_ERROR("dart_allgatherv ! team:%d nsendelem:%"PRIu64" failed",
                   teamid, nsendelem);
    free(inrecvcounts);
    free(irecvdispls);
    return DART_ERR_INVAL;
  }
  free(inrecvcounts);
  free(irecvdispls);
  DART_LOG_TRACE("dart_allgatherv > team:%d nsendelem:%"PRIu64"",
                 teamid, nsendelem);
  return DART_OK;
}

dart_ret_t dart_allreduce(
  const void       * sendbuf,
  void             * recvbuf,
  size_t             nelem,
  dart_datatype_t    dtype,
  dart_operation_t   op,
  dart_team_t        team)
{
  MPI_Comm     comm;
  MPI_Op       mpi_op    = dart_mpi_op(op);
  MPI_Datatype mpi_dtype = dart_mpi_datatype(dtype);

  /*
   * MPI uses offset type int, do not copy more than INT_MAX elements:
   */
  if (nelem > INT_MAX) {
    DART_LOG_ERROR("dart_allreduce ! failed: nelem > INT_MAX");
    return DART_ERR_INVAL;
  }

  uint16_t index;
  int result = dart_adapt_teamlist_convert(team, &index);

  if (result == -1) {
    return DART_ERR_INVAL;
  }
  comm = dart_team_data[index].comm;
  if (MPI_Allreduce(
           sendbuf,   // send buffer
           recvbuf,   // receive buffer
           nelem,     // buffer size
           mpi_dtype, // datatype
           mpi_op,    // reduce operation
           comm) != MPI_SUCCESS) {
    return DART_ERR_INVAL;
  }
  return DART_OK;
}

dart_ret_t dart_reduce(
  const void        * sendbuf,
  void              * recvbuf,
  size_t              nelem,
  dart_datatype_t     dtype,
  dart_operation_t    op,
  dart_team_unit_t    root,
  dart_team_t         team)
{
  uint16_t     index;
  MPI_Comm     comm;
  MPI_Op       mpi_op    = dart_mpi_op(op);
  MPI_Datatype mpi_dtype = dart_mpi_datatype(dtype);

  /*
   * MPI uses offset type int, do not copy more than INT_MAX elements:
   */
  if (nelem > INT_MAX) {
    DART_LOG_ERROR("dart_allreduce ! failed: nelem > INT_MAX");
    return DART_ERR_INVAL;
  }

  int result = dart_adapt_teamlist_convert (team, &index);
  if (result == -1) {
    return DART_ERR_INVAL;
  }
  comm = dart_team_data[index].comm;
  if (MPI_Reduce(
           sendbuf,
           recvbuf,
           nelem,
           mpi_dtype,
           mpi_op,
           root.id,
           comm) != MPI_SUCCESS) {
    return DART_ERR_INVAL;
  }
  return DART_OK;
}

dart_ret_t dart_send(
  const void         * sendbuf,
  size_t              nelem,
  dart_datatype_t     dtype,
  int                 tag,
  dart_global_unit_t  unit)
{
  MPI_Comm comm;
  MPI_Datatype mpi_dtype = dart_mpi_datatype(dtype);
  dart_team_t team = DART_TEAM_ALL;
  uint16_t index;

  /*
   * MPI uses offset type int, do not copy more than INT_MAX elements:
   */
  if (nelem > INT_MAX) {
    DART_LOG_ERROR("dart_send ! failed: nelem > INT_MAX");
    return DART_ERR_INVAL;
  }

  int result = dart_adapt_teamlist_convert(team, &index);
  if(result == -1) {
    return DART_ERR_INVAL;
  }
  comm = dart_team_data[index].comm;
  // dart_unit = MPI rank in comm_world
  if(MPI_Send(
        sendbuf,
        nelem,
        mpi_dtype,
        unit.id,
        tag,
        comm) != MPI_SUCCESS) {
    return DART_ERR_INVAL;
  }
  return DART_OK;
}

dart_ret_t dart_recv(
  void                * recvbuf,
  size_t                nelem,
  dart_datatype_t       dtype,
  int                   tag,
  dart_global_unit_t    unit)
{
  MPI_Comm comm;
  MPI_Datatype mpi_dtype = dart_mpi_datatype(dtype);
  dart_team_t team = DART_TEAM_ALL;
  uint16_t index;

  /*
   * MPI uses offset type int, do not copy more than INT_MAX elements:
   */
  if (nelem > INT_MAX) {
    DART_LOG_ERROR("dart_recv ! failed: nelem > INT_MAX");
    return DART_ERR_INVAL;
  }

  int result = dart_adapt_teamlist_convert(team, &index);
  if(result == -1) {
    return DART_ERR_INVAL;
  }
  comm = dart_team_data[index].comm;
  // dart_unit = MPI rank in comm_world
  if(MPI_Recv(
        recvbuf,
        nelem,
        mpi_dtype,
        unit.id,
        tag,
        comm,
        MPI_STATUS_IGNORE) != MPI_SUCCESS) {
    return DART_ERR_INVAL;
  }
  return DART_OK;
}

dart_ret_t dart_sendrecv(
  const void         * sendbuf,
  size_t               send_nelem,
  dart_datatype_t      send_dtype,
  int                  send_tag,
  dart_global_unit_t   dest,
  void               * recvbuf,
  size_t               recv_nelem,
  dart_datatype_t      recv_dtype,
  int                  recv_tag,
  dart_global_unit_t   src)
{
  MPI_Comm comm;
  MPI_Datatype mpi_send_dtype = dart_mpi_datatype(send_dtype);
  MPI_Datatype mpi_recv_dtype = dart_mpi_datatype(recv_dtype);
  dart_team_t team = DART_TEAM_ALL;
  uint16_t index;

  /*
   * MPI uses offset type int, do not copy more than INT_MAX elements:
   */
  if (send_nelem > INT_MAX || recv_nelem > INT_MAX) {
    DART_LOG_ERROR("dart_sendrecv ! failed: nelem > INT_MAX");
    return DART_ERR_INVAL;
  }

  int result = dart_adapt_teamlist_convert(team, &index);
  if(result == -1) {
    return DART_ERR_INVAL;
  }
  comm = dart_team_data[index].comm;
  if(MPI_Sendrecv(
        sendbuf,
        send_nelem,
        mpi_send_dtype,
        dest.id,
        send_tag,
        recvbuf,
        recv_nelem,
        mpi_recv_dtype,
        src.id,
        recv_tag,
        comm,
        MPI_STATUS_IGNORE) != MPI_SUCCESS) {
    return DART_ERR_INVAL;
  }
  return DART_OK;
}


