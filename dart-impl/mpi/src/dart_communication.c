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
#include <alloca.h>

int dart__mpi__datatype_sizes[DART_TYPE_COUNT];

static inline size_t minsize(size_t a, size_t b)
{
  return (a < b) ? a : b;
}

dart_ret_t
dart__mpi__datatype_init()
{
  for (int i = DART_TYPE_UNDEFINED+1; i < DART_TYPE_COUNT; i++) {
    int ret = MPI_Type_size(
                dart__mpi__datatype(i),
                &dart__mpi__datatype_sizes[i]);
    if (ret != MPI_SUCCESS) {
      DART_LOG_ERROR("Failed to query size of DART data type %i", i);
      return DART_ERR_INVAL;
    }
  }
  return DART_OK;
}

#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
static dart_ret_t get_shared_mem(
  dart_team_data_t * team_data,
  void             * dest,
  dart_gptr_t        gptr,
  size_t             nelem,
  dart_datatype_t    dtype)
{
  int16_t      seg_id            = gptr.segid;
  uint64_t     offset            = gptr.addr_or_offs.offset;
  DART_LOG_DEBUG("dart_get: shared windows enabled");
  dart_team_unit_t luid = team_data->sharedmem_tab[gptr.unitid];
  char * baseptr;
  /*
   * Use memcpy if the target is in the same node as the calling unit:
   */
  DART_LOG_DEBUG("dart_get: shared memory segment, seg_id:%d",
                 seg_id);
  if (seg_id) {
    if (dart_segment_get_baseptr(
          &team_data->segdata, seg_id, luid, &baseptr) != DART_OK) {
      DART_LOG_ERROR("dart_get ! "
                     "dart_adapt_transtable_get_baseptr failed");
      return DART_ERR_INVAL;
    }
  } else {
    baseptr = dart_sharedmem_local_baseptr_set[luid.id];
  }
  baseptr += offset;
  DART_LOG_DEBUG(
    "dart_get: memcpy %zu bytes", nelem * dart__mpi__datatype_sizeof(dtype));
  memcpy((char*)dest, baseptr, nelem * dart__mpi__datatype_sizeof(dtype));
  return DART_OK;
}
#endif // !defined(DART_MPI_DISABLE_SHARED_WINDOWS)

dart_ret_t dart_get(
  void            * dest,
  dart_gptr_t       gptr,
  size_t            nelem,
  dart_datatype_t   dtype)
{
  MPI_Win          win;
  MPI_Datatype     mpi_dtype    = dart__mpi__datatype(dtype);
  uint64_t         offset       = gptr.addr_or_offs.offset;
  int16_t          seg_id       = gptr.segid;
  dart_team_unit_t team_unit_id = DART_TEAM_UNIT_ID(gptr.unitid);

  if (gptr.unitid < 0) {
    DART_LOG_ERROR("dart_get ! failed: gptr.unitid < 0");
    return DART_ERR_INVAL;
  }

  dart_team_data_t *team_data = dart_adapt_teamlist_get(gptr.teamid);
  if (team_data == NULL) {
    DART_LOG_ERROR("dart_get ! failed: Unknown team %i!", gptr.teamid);
    return DART_ERR_INVAL;
  }

  DART_LOG_DEBUG("dart_get() uid:%d o:%"PRIu64" s:%d t:%d nelem:%zu",
                 team_unit_id.id, offset, seg_id, gptr.teamid, nelem);

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
    MPI_Aint disp_s;
    if (dart_segment_get_disp(
          &team_data->segdata,
          seg_id,
          team_unit_id,
          &disp_s) != DART_OK) {
      return DART_ERR_INVAL;
    }

    if (team_data->unitid == team_unit_id.id) {
      // use direct memcpy if we are on the same unit
      memcpy(dest, ((void*)disp_s) + offset,
          nelem * dart__mpi__datatype_sizeof(dtype));
      DART_LOG_TRACE("dart_get: memcpy nelem:%zu "
                     "source (coll.): disp:%"PRId64" -> dest:%p",
                     nelem, offset, dest);
      return DART_OK;
    }

    offset += disp_s;
    win = team_data->window;
    DART_LOG_TRACE("dart_get:  nelem:%zu "
                   "source (coll.): win:%"PRIu64" unit:%d disp:%"PRId64" "
                   "-> dest:%p",
                   nelem, (unsigned long)win, team_unit_id.id, offset, dest);

  } else {

    if (team_data->unitid == team_unit_id.id) {
      // use direct memcpy if we are on the same unit
      memcpy(dest, dart_mempool_localalloc + offset,
          nelem * dart__mpi__datatype_sizeof(dtype));
      DART_LOG_TRACE("dart_get: memcpy nelem:%zu "
                     "source (local): disp:%"PRId64" -> dest:%p",
                     nelem, offset, dest);
      return DART_OK;
    }

    win      = dart_win_local_alloc;
    DART_LOG_TRACE("dart_get:  nelem:%zu "
                   "source (local): win:%"PRIu64" unit:%d disp:%"PRId64" "
                   "-> dest:%p",
                   nelem, (unsigned long)win, team_unit_id.id, offset, dest);
  }

  /*
   * MPI uses offset type int, do not copy more than INT_MAX elements:
   */
  // chunk up the get
  const size_t chunk_size = INT_MAX;
        char * dest_ptr   = (char*) dest;
        size_t copied     = 0;

  while (copied < nelem) {
    int to_copy = minsize(chunk_size, nelem - copied);
    DART_LOG_TRACE("dart_get:  MPI_Get (chunk %d, size %d)", chunk, to_copy);
    if (MPI_Get(dest_ptr,
                to_copy,
                mpi_dtype,
                team_unit_id.id,
                offset,
                to_copy,
                mpi_dtype,
                win) != MPI_SUCCESS) {
      DART_LOG_ERROR("dart_get ! MPI_Get failed");
      return DART_ERR_INVAL;
    }
    dest_ptr += chunk_size;
    offset   += chunk_size;
    copied   += chunk_size;
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
  MPI_Win          win;
  MPI_Datatype     mpi_dtype    = dart__mpi__datatype(dtype);
  uint64_t         offset       = gptr.addr_or_offs.offset;
  int16_t          seg_id       = gptr.segid;
  dart_team_unit_t team_unit_id = DART_TEAM_UNIT_ID(gptr.unitid);

  if (gptr.unitid < 0) {
    DART_LOG_ERROR("dart_put ! failed: gptr.unitid < 0");
    return DART_ERR_INVAL;
  }

  dart_team_data_t *team_data = dart_adapt_teamlist_get(gptr.teamid);
  if (team_data == NULL) {
    DART_LOG_ERROR("dart_put ! failed: Unknown team %i!", gptr.teamid);
    return DART_ERR_INVAL;
  }

  if (seg_id) {

    MPI_Aint disp_s;
    if (dart_segment_get_disp(
          &team_data->segdata,
          seg_id,
          team_unit_id,
          &disp_s) != DART_OK) {
      return DART_ERR_INVAL;
    }

    /* copy data directly if we are on the same unit */
    if (team_unit_id.id == team_data->unitid) {
      memcpy(((void*)disp_s) + offset, src,
          nelem * dart__mpi__datatype_sizeof(dtype));
      DART_LOG_DEBUG("dart_put: memcpy nelem:%zu (from global allocation)"
                     "offset: %"PRIu64"", nelem, offset);
      return DART_OK;
    }

    win     = team_data->window;
    offset += disp_s;

  } else {

    /* copy data directly if we are on the same unit */
    if (team_unit_id.id == team_data->unitid) {
      memcpy(dart_mempool_localalloc + offset, src,
          nelem * dart__mpi__datatype_sizeof(dtype));
      DART_LOG_DEBUG("dart_put: memcpy nelem:%zu (from local allocation)"
                     "offset: %"PRIu64"", nelem, offset);
      return DART_OK;
    }

    win = dart_win_local_alloc;
  }


  // chunk up the put
  const size_t chunk_size = INT_MAX;
        char * src_ptr    = (char*) src;
        size_t copied     = 0;

  while (copied < nelem) {
    int to_copy = minsize(chunk_size, nelem - copied);
    DART_LOG_TRACE("dart_get:  MPI_Put (chunk %d, size %d)", chunk, to_copy);
    if (MPI_Put(src_ptr,
                to_copy,
                mpi_dtype,
                team_unit_id.id,
                offset,
                to_copy,
                mpi_dtype,
                win) != MPI_SUCCESS) {
      DART_LOG_ERROR("dart_get ! MPI_Put failed");
      return DART_ERR_INVAL;
    }
    src_ptr += chunk_size;
    offset  += chunk_size;
    copied  += chunk_size;
  }

  return DART_OK;
}

dart_ret_t dart_accumulate(
  dart_gptr_t      gptr,
  const void     * values,
  size_t           nelem,
  dart_datatype_t  dtype,
  dart_operation_t op)
{
  MPI_Win      win;
  MPI_Datatype mpi_dtype;
  MPI_Op       mpi_op;
  dart_team_unit_t  team_unit_id = DART_TEAM_UNIT_ID(gptr.unitid);
  uint64_t offset   = gptr.addr_or_offs.offset;
  int16_t  seg_id   = gptr.segid;
  mpi_dtype         = dart__mpi__datatype(dtype);
  mpi_op            = dart__mpi__op(op);

  if (gptr.unitid < 0) {
    DART_LOG_ERROR("dart_accumulate ! failed: gptr.unitid < 0");
    return DART_ERR_INVAL;
  }

  DART_LOG_DEBUG("dart_accumulate() nelem:%zu dtype:%d op:%d unit:%d",
                 nelem, dtype, op, team_unit_id.id);

  /*
   * MPI uses offset type int, do not copy more than INT_MAX elements:
   */
  if (nelem > INT_MAX) {
    DART_LOG_ERROR("dart_accumulate ! failed: nelem > INT_MAX");
    return DART_ERR_INVAL;
  }

  if (seg_id) {
    MPI_Aint disp_s;
    dart_team_data_t *team_data = dart_adapt_teamlist_get(gptr.teamid);
    if (team_data == NULL) {
      DART_LOG_ERROR("dart_accumulate ! failed: Unknown team %i!", gptr.teamid);
      return DART_ERR_INVAL;
    }

    win = team_data->window;
    if (dart_segment_get_disp(
          &team_data->segdata,
          seg_id,
          team_unit_id,
          &disp_s) != DART_OK) {
      DART_LOG_ERROR("dart_accumulate ! "
                     "dart_adapt_transtable_get_disp failed");
      return DART_ERR_INVAL;
    }
    offset += disp_s;
    DART_LOG_TRACE("dart_accumulate:  nelem:%zu (from collective allocation) "
                   "target unit: %d offset: %"PRIu64"",
                   nelem, team_unit_id.id, offset);
  } else {
    win = dart_win_local_alloc;
    DART_LOG_TRACE("dart_accumulate:  nelem:%zu (from local allocation) "
                   "target unit: %d offset: %"PRIu64"",
                   nelem, team_unit_id.id, offset);
  }

  MPI_Accumulate(
    values,            // Origin address
    nelem,             // Number of entries in buffer
    mpi_dtype,         // Data type of each buffer entry
    team_unit_id.id,   // Rank of target
    offset,            // Displacement from start of window to beginning
                       // of target buffer
    nelem,             // Number of entries in target buffer
    mpi_dtype,         // Data type of each entry in target buffer
    mpi_op,            // Reduce operation
    win);

  DART_LOG_DEBUG("dart_accumulate > finished");
  return DART_OK;
}

dart_ret_t dart_fetch_and_op(
  dart_gptr_t      gptr,
  const void *     value,
  void *           result,
  dart_datatype_t  dtype,
  dart_operation_t op)
{
  MPI_Win      win;
  MPI_Datatype mpi_dtype;
  MPI_Op       mpi_op;
  dart_team_unit_t  team_unit_id = DART_TEAM_UNIT_ID(gptr.unitid);
  uint64_t offset   = gptr.addr_or_offs.offset;
  int16_t  seg_id   = gptr.segid;
  mpi_dtype         = dart__mpi__datatype(dtype);
  mpi_op            = dart__mpi__op(op);

  if (gptr.unitid < 0) {
    DART_LOG_ERROR("dart_fetch_and_op ! failed: gptr.unitid < 0");
    return DART_ERR_INVAL;
  }

  DART_LOG_DEBUG("dart_fetch_and_op() dtype:%d op:%d unit:%d "
                 "offset:%"PRIu64" segid:%d",
                 dtype, op, team_unit_id.id,
                 gptr.addr_or_offs.offset, gptr.segid);
  if (seg_id) {

    dart_team_data_t *team_data = dart_adapt_teamlist_get(gptr.teamid);
    if (team_data == NULL) {
      DART_LOG_ERROR("dart_fetch_and_op ! failed: Unknown team %i!",
                     gptr.teamid);
      return DART_ERR_INVAL;
    }

    MPI_Aint disp_s;
    if (dart_segment_get_disp(
          &team_data->segdata,
          seg_id,
          team_unit_id,
          &disp_s) != DART_OK) {
      DART_LOG_ERROR("dart_fetch_and_op ! "
                     "dart_adapt_transtable_get_disp failed");
      return DART_ERR_INVAL;
    }
    offset += disp_s;
    win = team_data->window;
    DART_LOG_TRACE("dart_fetch_and_op:  (from coll. allocation) "
                   "target unit: %d offset: %"PRIu64,
                   team_unit_id.id, offset);
  } else {
    win = dart_win_local_alloc;
    DART_LOG_TRACE("dart_fetch_and_op:  (from local allocation) "
                   "target unit: %d offset: %"PRIu64,
                   team_unit_id.id, offset);
  }
  MPI_Fetch_and_op(
    value,             // Origin address
    result,            // Result address
    mpi_dtype,         // Data type of each buffer entry
    team_unit_id.id,   // Rank of target
    offset,            // Displacement from start of window to beginning
                       // of target buffer
    mpi_op,            // Reduce operation
    win);
  DART_LOG_DEBUG("dart_fetch_and_op > finished");
  return DART_OK;
}

dart_ret_t dart_compare_and_swap(
  dart_gptr_t      gptr,
  const void     * value,
  const void     * compare,
  void           * result,
  dart_datatype_t  dtype)
{
  MPI_Win win;
  dart_team_unit_t  team_unit_id = DART_TEAM_UNIT_ID(gptr.unitid);
  uint64_t offset   = gptr.addr_or_offs.offset;
  int16_t  seg_id   = gptr.segid;
  MPI_Datatype mpi_dtype         = dart__mpi__datatype(dtype);

  if (gptr.unitid < 0) {
    DART_LOG_ERROR("dart_compare_and_swap ! failed: gptr.unitid < 0");
    return DART_ERR_INVAL;
  }

  if (dtype > DART_TYPE_LONGLONG) {
    DART_LOG_ERROR("dart_compare_and_swap ! failed: "
                   "only valid on integral types");
    return DART_ERR_INVAL;
  }

  DART_LOG_TRACE("dart_compare_and_swap() dtype:%d unit:%d offset:%"PRIu64,
                 dtype, team_unit_id.id, gptr.addr_or_offs.offset);

  if (seg_id) {
    MPI_Aint disp_s;

    dart_team_data_t *team_data = dart_adapt_teamlist_get(gptr.teamid);
    if (team_data == NULL) {
      DART_LOG_ERROR("dart_get_handle ! failed: Unknown team %i!", gptr.teamid);
      return DART_ERR_INVAL;
    }

    win = team_data->window;
    if (dart_segment_get_disp(
          &team_data->segdata,
          seg_id,
          team_unit_id,
          &disp_s) != DART_OK) {
      DART_LOG_ERROR("dart_accumulate ! "
                     "dart_adapt_transtable_get_disp failed");
      return DART_ERR_INVAL;
    }
    offset += disp_s;
    DART_LOG_TRACE("dart_compare_and_swap: target unit: %d offset: %"PRIu64"",
              team_unit_id.id, offset);
  } else {
    win = dart_win_local_alloc;
    DART_LOG_TRACE("dart_compare_and_swap: target unit: %d offset: %"PRIu64"",
              team_unit_id.id, offset);
  }
  MPI_Compare_and_swap(
        value,
        compare,
        result,
        mpi_dtype,
        team_unit_id.id,
        offset,
        win);
  DART_LOG_DEBUG("dart_compare_and_swap > finished");
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
  MPI_Datatype mpi_type = dart__mpi__datatype(dtype);
  MPI_Win      win;
  dart_team_unit_t    team_unit_id = DART_TEAM_UNIT_ID(gptr.unitid);
  uint64_t     offset = gptr.addr_or_offs.offset;
  int16_t      seg_id = gptr.segid;

  *handle = NULL;

  if (gptr.unitid < 0) {
    DART_LOG_ERROR("dart_get_handle ! failed: gptr.unitid < 0");
    return DART_ERR_INVAL;
  }

  /*
   * MPI uses offset type int, do not copy more than INT_MAX elements:
   */
  if (nelem > INT_MAX) {
    DART_LOG_ERROR("dart_get_handle ! failed: nelem > INT_MAX");
    return DART_ERR_INVAL;
  }

  dart_team_data_t *team_data = dart_adapt_teamlist_get(gptr.teamid);
  if (team_data == NULL) {
    DART_LOG_ERROR("dart_get_handle ! failed: Unknown team %i!", gptr.teamid);
    return DART_ERR_INVAL;
  }

  DART_LOG_DEBUG("dart_get_handle() uid:%d o:%"PRIu64" s:%d t:%d, nelem:%zu",
                 team_unit_id.id, offset, seg_id, gptr.teamid, nelem);
  DART_LOG_TRACE("dart_get_handle:  allocated handle:%p", (void *)(*handle));

#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
  DART_LOG_DEBUG("dart_get_handle: shared windows enabled");

  if (seg_id >= 0 && team_data->sharedmem_tab[gptr.unitid].id >= 0) {
    dart_ret_t ret = get_shared_mem(team_data, dest, gptr, nelem, dtype);

    /*
     * Mark request as completed:
     */
    *handle            = malloc(sizeof(struct dart_handle_struct));
    (*handle)->request = MPI_REQUEST_NULL;
    if (seg_id != 0) {
      (*handle)->dest = team_unit_id.id;
      (*handle)->win = team_data->window;
    } else {
      (*handle)->dest = team_unit_id.id;
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
  if (seg_id) {

    /*
     * The memory accessed is allocated with collective allocation.
     */
    win = team_data->window;

    MPI_Aint disp_s;
    if (dart_segment_get_disp(
          &team_data->segdata,
          seg_id,
          team_unit_id,
          &disp_s) != DART_OK) {
      DART_LOG_ERROR(
        "dart_get_handle ! dart_adapt_transtable_get_disp failed");
      return DART_ERR_INVAL;
    }
    offset += disp_s;

    DART_LOG_DEBUG("dart_get_handle:  -- %zu elements (collective allocation) "
                   "from %d at offset %"PRIu64"",
                   nelem, team_unit_id.id, offset);
  } else {
    /*
     * The memory accessed is allocated with local allocation.
     */
    DART_LOG_DEBUG("dart_get_handle:  -- %zu elements (local allocation) "
                   "from %d at offset %"PRIu64"",
                   nelem, team_unit_id.id, offset);
    win     = dart_win_local_alloc;
  }
  DART_LOG_DEBUG("dart_get_handle:  -- MPI_Rget");
  MPI_Request mpi_req;
  int mpi_ret = MPI_Rget(
                  dest,              // origin address
                  nelem,             // origin count
                  mpi_type,          // origin data type
                  team_unit_id.id, // target rank
                  offset,            // target disp in window
                  nelem,             // target count
                  mpi_type,          // target data type
                  win,               // window
                  &mpi_req);
  if (mpi_ret != MPI_SUCCESS) {
    DART_LOG_ERROR("dart_get_handle ! MPI_Rget failed");
    return DART_ERR_INVAL;
  }
  *handle            = malloc(sizeof(struct dart_handle_struct));
  (*handle)->dest    = team_unit_id.id;
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
  MPI_Datatype mpi_type = dart__mpi__datatype(dtype);
  dart_team_unit_t  team_unit_id = DART_TEAM_UNIT_ID(gptr.unitid);
  uint64_t     offset   = gptr.addr_or_offs.offset;
  int16_t      seg_id   = gptr.segid;
  MPI_Win      win;

  *handle = NULL;

  if (gptr.unitid < 0) {
    DART_LOG_ERROR("dart_put_handle ! failed: gptr.unitid < 0");
    return DART_ERR_INVAL;
  }

  /*
   * MPI uses offset type int, do not copy more than INT_MAX elements:
   */
  if (nelem > INT_MAX) {
    DART_LOG_ERROR("dart_put_handle ! failed: nelem > INT_MAX");
    return DART_ERR_INVAL;
  }


  if (seg_id != 0) {
    dart_team_data_t *team_data = dart_adapt_teamlist_get(gptr.teamid);
    if (team_data == NULL) {
      DART_LOG_ERROR("dart_put_handle ! failed: Unknown team %i!", gptr.teamid);
      return DART_ERR_INVAL;
    }

    win = team_data->window;

    MPI_Aint disp_s;
    if (dart_segment_get_disp(
          &team_data->segdata,
          seg_id,
          team_unit_id,
          &disp_s) != DART_OK) {
      return DART_ERR_INVAL;
    }
    offset += disp_s;

    DART_LOG_DEBUG("dart_put_handle: nelem:%zu dtype:%d"
                   "(from collective allocation) "
                   "target_unit:%d offset:%"PRIu64"",
                   nelem, dtype, team_unit_id.id, offset);
  } else {
    win = dart_win_local_alloc;
    DART_LOG_DEBUG("dart_put_handle: nlem:%zu dtype:%d"
                   "(from local allocation) "
                   "target_unit:%d offset:%"PRIu64"",
                   nelem, dtype, team_unit_id.id, offset);
  }

  DART_LOG_DEBUG("dart_put_handle: MPI_RPut");
  int ret = MPI_Rput(
              src,
              nelem,
              mpi_type,
              team_unit_id.id,
              offset,
              nelem,
              mpi_type,
              win,
              &mpi_req);

  if (ret != MPI_SUCCESS) {
    DART_LOG_ERROR("dart_put_handle ! MPI_Rput failed");
    return DART_ERR_INVAL;
  }
  *handle = malloc(sizeof(struct dart_handle_struct));
  (*handle) -> dest    = team_unit_id.id;
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
  MPI_Win           win;
  MPI_Datatype      mpi_dtype    = dart__mpi__datatype(dtype);
  dart_team_unit_t  team_unit_id = DART_TEAM_UNIT_ID(gptr.unitid);
  uint64_t          offset       = gptr.addr_or_offs.offset;
  int16_t           seg_id       = gptr.segid;

  if (gptr.unitid < 0) {
    DART_LOG_ERROR("dart_put_blocking ! failed: gptr.unitid < 0");
    return DART_ERR_INVAL;
  }

  dart_team_data_t *team_data = dart_adapt_teamlist_get(gptr.teamid);
  if (team_data == NULL) {
    DART_LOG_ERROR("dart_put_blocking ! failed: Unknown team %i!", gptr.teamid);
    return DART_ERR_INVAL;
  }

  DART_LOG_DEBUG("dart_put_blocking() uid:%d o:%"PRIu64" s:%d t:%d, nelem:%zu",
                 team_unit_id.id, offset, seg_id, gptr.teamid, nelem);

#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
  DART_LOG_DEBUG("dart_put_blocking: shared windows enabled");
  if (seg_id >= 0) {
    /*
     * Use memcpy if the target is in the same node as the calling unit:
     * The value of i will be the target's relative ID in teamid.
     */
    dart_team_unit_t luid = team_data->sharedmem_tab[gptr.unitid];
    if (luid.id >= 0) {
      char * baseptr;
      DART_LOG_DEBUG("dart_put_blocking: shared memory segment, seg_id:%d",
                     seg_id);
      if (seg_id) {
        if (dart_segment_get_baseptr(
                &team_data->segdata,
                seg_id,
                luid,
                &baseptr) != DART_OK) {
          DART_LOG_ERROR("dart_put_blocking ! "
                         "dart_adapt_transtable_get_baseptr failed");
          return DART_ERR_INVAL;
        }
      } else {
        baseptr = dart_sharedmem_local_baseptr_set[luid.id];
      }
      baseptr += offset;
      DART_LOG_DEBUG("dart_put_blocking: memcpy %zu bytes",
                        nelem * dart__mpi__datatype_sizeof(dtype));
      memcpy(baseptr, src, nelem * dart__mpi__datatype_sizeof(dtype));
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
    MPI_Aint disp_s;
    if (dart_segment_get_disp(
          &team_data->segdata,
          seg_id,
          team_unit_id,
          &disp_s) != DART_OK) {
      DART_LOG_ERROR("dart_put_blocking ! "
                     "dart_adapt_transtable_get_disp failed");
      return DART_ERR_INVAL;
    }

    /* copy data directly if we are on the same unit */
    if (team_unit_id.id == team_data->unitid) {
      memcpy(((void*)disp_s) + offset, src,
          nelem*dart__mpi__datatype_sizeof(dtype));
      DART_LOG_DEBUG("dart_put: memcpy nelem:%zu "
                     "target unit: %d offset: %"PRIu64"",
                     nelem, team_unit_id.id, offset);
      return DART_OK;
    }

    win = team_data->window;
    offset += disp_s;
    DART_LOG_DEBUG("dart_put_blocking:  nelem:%zu "
                   "target (coll.): win:%p unit:%d offset:%lu "
                   "<- source: %p",
                   nelem, win, team_unit_id.id,
                   offset, src);

  } else {

    /* copy data directly if we are on the same unit */
    if (team_unit_id.id == team_data->unitid) {
      memcpy(dart_mempool_localalloc + offset, src,
          nelem * dart__mpi__datatype_sizeof(dtype));
      DART_LOG_DEBUG("dart_put: memcpy nelem:%zu offset: %"PRIu64"",
                     nelem, offset);
      return DART_OK;
    }

    win      = dart_win_local_alloc;
    DART_LOG_DEBUG("dart_put_blocking:  nelem:%zu "
                   "target (local): win:%p unit:%d offset:%lu "
                   "<- source: %p",
                   nelem, win, team_unit_id.id,
                   offset, src);
  }

  /*
   * Using MPI_Put as MPI_Win_flush is required to ensure remote completion.
   */
  // chunk up the put
  const size_t   chunk_size = INT_MAX;
        char   * src_ptr    = (char*) src;
        size_t   copied     = 0;

  while (copied < nelem) {
    int to_copy = minsize(chunk_size, nelem - copied);
    DART_LOG_TRACE("dart_get:  MPI_Put (chunk %d, size %d)", chunk, to_copy);
    if (MPI_Put(src_ptr,
                to_copy,
                mpi_dtype,
                team_unit_id.id,
                offset,
                to_copy,
                mpi_dtype,
                win) != MPI_SUCCESS) {
      DART_LOG_ERROR("dart_get ! MPI_Put failed");
      return DART_ERR_INVAL;
    }
    src_ptr += chunk_size;
    offset  += chunk_size;
    copied  += chunk_size;
  }


  DART_LOG_DEBUG("dart_put_blocking: MPI_Win_flush");
  if (MPI_Win_flush(team_unit_id.id, win) != MPI_SUCCESS) {
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
  MPI_Win           win;
  MPI_Datatype      mpi_dtype    = dart__mpi__datatype(dtype);
  dart_team_unit_t  team_unit_id = DART_TEAM_UNIT_ID(gptr.unitid);
  uint64_t          offset       = gptr.addr_or_offs.offset;
  int16_t           seg_id       = gptr.segid;

  if (gptr.unitid < 0) {
    DART_LOG_ERROR("dart_get_blocking ! failed: gptr.unitid < 0");
    return DART_ERR_INVAL;
  }

  dart_team_data_t *team_data = dart_adapt_teamlist_get(gptr.teamid);
  if (team_data == NULL) {
    DART_LOG_ERROR("dart_get_blocking ! failed: Unknown team %i!", gptr.teamid);
    return DART_ERR_INVAL;
  }

  DART_LOG_DEBUG("dart_get_blocking() uid:%d "
                 "o:%"PRIu64" s:%d t:%u, nelem:%zu",
                 team_unit_id.id,
                 offset, seg_id, gptr.teamid, nelem);

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
    MPI_Aint disp_s;
    if (dart_segment_get_disp(
          &team_data->segdata,
          seg_id,
          team_unit_id,
          &disp_s) != DART_OK) {
      DART_LOG_ERROR("dart_get_blocking ! "
                     "dart_adapt_transtable_get_disp failed");
      return DART_ERR_INVAL;
    }

    if (team_data->unitid == team_unit_id.id) {
      // use direct memcpy if we are on the same unit
      memcpy(dest, ((void*)disp_s) + offset,
          nelem * dart__mpi__datatype_sizeof(dtype));
      DART_LOG_DEBUG("dart_get_blocking: memcpy nelem:%zu "
                     "source (coll.): offset:%lu -> dest: %p",
                     nelem, offset, dest);
      return DART_OK;
    }

    win     = team_data->window;
    offset += disp_s;
    DART_LOG_DEBUG("dart_get_blocking:  nelem:%zu "
                   "source (coll.): win:%p unit:%d offset:%lu "
                   "-> dest: %p",
                   nelem, win, team_unit_id.id,
                   offset, dest);

  } else {

    if (team_data->unitid == team_unit_id.id) {
      /* use direct memcpy if we are on the same unit */
      memcpy(dest, dart_mempool_localalloc + offset,
          nelem * dart__mpi__datatype_sizeof(dtype));
      DART_LOG_DEBUG("dart_get_blocking: memcpy nelem:%zu "
                     "source (coll.): offset:%lu -> dest: %p",
                     nelem, offset, dest);
      return DART_OK;
    }

    win = dart_win_local_alloc;
    DART_LOG_DEBUG("dart_get_blocking:  nelem:%zu "
                   "source (local): win:%p unit:%d offset:%lu "
                   "-> dest: %p",
                   nelem, win, team_unit_id.id,
                   offset, dest);
  }

  /*
   * Using MPI_Get as MPI_Win_flush is required to ensure remote completion.
   */

  if (nelem < INT_MAX) {
    DART_LOG_DEBUG("dart_get_blocking: MPI_Rget");
    MPI_Request req;
    if (MPI_Rget(dest,
                nelem,
                mpi_dtype,
                team_unit_id.id,
                offset,
                nelem,
                mpi_dtype,
                win,
                &req)
        != MPI_SUCCESS) {
      DART_LOG_ERROR("dart_get_blocking ! MPI_Rget failed");
      return DART_ERR_INVAL;
    }
    DART_LOG_DEBUG("dart_get_blocking: MPI_Wait");
    if (MPI_Wait(&req, MPI_STATUS_IGNORE) != MPI_SUCCESS) {
      DART_LOG_ERROR("dart_get_blocking ! MPI_Wait failed");
      return DART_ERR_INVAL;
    }
  } else {
    // chunk up the get
    const size_t chunk_size = INT_MAX;
          char *dest_ptr = (char*) dest;
    const int num_chunks = (int)ceil(((double)nelem) / ((double)INT_MAX));
    MPI_Request *reqs    = alloca(num_chunks * sizeof(MPI_Request));
    for (int chunk = 0; chunk < num_chunks; ++chunk) {
      DART_LOG_TRACE("dart_get:  MPI_Rget (chunk %d)", chunk);
      int to_copy = minsize(chunk_size, nelem - (chunk*chunk_size));
      if (MPI_Rget(dest_ptr,
                  to_copy,
                  mpi_dtype,
                  team_unit_id.id,
                  offset,
                  to_copy,
                  mpi_dtype,
                  win,
                  &reqs[chunk]) != MPI_SUCCESS) {
        DART_LOG_ERROR("dart_get ! MPI_Rget failed");
        return DART_ERR_INVAL;
      }
      dest_ptr += chunk_size;
      offset   += chunk_size;
    }
    // wait for all requests to finish
    MPI_Waitall(num_chunks, reqs, MPI_STATUSES_IGNORE);
  }


  DART_LOG_DEBUG("dart_get_blocking > finished");
  return DART_OK;
}

/* -- Dart RMA Synchronization Operations -- */

dart_ret_t dart_flush(
  dart_gptr_t gptr)
{
  MPI_Win          win;
  MPI_Comm         comm         = DART_COMM_WORLD;
  dart_team_unit_t team_unit_id = DART_TEAM_UNIT_ID(gptr.unitid);
  int16_t          seg_id       = gptr.segid;
  DART_LOG_DEBUG("dart_flush() gptr: "
                 "unitid:%d offset:%"PRIu64" segid:%d teamid:%d",
                 gptr.unitid, gptr.addr_or_offs.offset,
                 gptr.segid,  gptr.teamid);

  if (gptr.unitid < 0) {
    DART_LOG_ERROR("dart_flush ! failed: gptr.unitid < 0");
    return DART_ERR_INVAL;
  }

  if (seg_id) {
    dart_team_data_t *team_data = dart_adapt_teamlist_get(gptr.teamid);
    if (team_data == NULL) {
      DART_LOG_ERROR("dart_flush ! failed: Unknown team %i!", gptr.teamid);
      return DART_ERR_INVAL;
    }
    win = team_data->window;
    comm = team_data->comm;
  } else {
    win = dart_win_local_alloc;
  }

  DART_LOG_TRACE("dart_flush: MPI_Win_flush");
  if (MPI_Win_flush(team_unit_id.id, win) != MPI_SUCCESS) {
    DART_LOG_ERROR("dart_flush ! MPI_Win_flush failed!");
    return DART_ERR_OTHER;
  }
  DART_LOG_TRACE("dart_flush: MPI_Win_sync");
  if (MPI_Win_sync(win) != MPI_SUCCESS) {
    DART_LOG_ERROR("dart_flush ! MPI_Win_sync failed!");
    return DART_ERR_OTHER;
  }

  // trigger progress
  int flag;
  MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &flag, MPI_STATUS_IGNORE);

  DART_LOG_DEBUG("dart_flush > finished");
  return DART_OK;
}

dart_ret_t dart_flush_all(
  dart_gptr_t gptr)
{
  MPI_Win  win;
  MPI_Comm comm   = DART_COMM_WORLD;
  int16_t  seg_id = gptr.segid;
  DART_LOG_DEBUG("dart_flush_all() gptr: "
                 "unitid:%d offset:%"PRIu64" segid:%d teamid:%d",
                 gptr.unitid, gptr.addr_or_offs.offset,
                 gptr.segid,  gptr.teamid);
  if (gptr.unitid < 0) {
    DART_LOG_ERROR("dart_flush_all ! failed: gptr.unitid < 0");
    return DART_ERR_INVAL;
  }

  if (seg_id) {
    dart_team_data_t *team_data = dart_adapt_teamlist_get(gptr.teamid);
    if (team_data == NULL) {
      DART_LOG_ERROR("dart_flush_all ! failed: Unknown team %i!", gptr.teamid);
      return DART_ERR_INVAL;
    }

    win  = team_data->window;
    comm = team_data->comm;
  } else {
    win = dart_win_local_alloc;
  }
  DART_LOG_TRACE("dart_flush_all: MPI_Win_flush_all");
  if (MPI_Win_flush_all(win) != MPI_SUCCESS) {
    DART_LOG_ERROR("dart_flush_all ! MPI_Win_flush_all failed!");
    return DART_ERR_OTHER;
  }
  DART_LOG_TRACE("dart_flush_all: MPI_Win_sync");
  if (MPI_Win_sync(win) != MPI_SUCCESS) {
    DART_LOG_ERROR("dart_flush_all ! MPI_Win_sync failed!");
    return DART_ERR_OTHER;
  }

  // trigger progress
  int flag;
  MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &flag, MPI_STATUS_IGNORE);

  DART_LOG_DEBUG("dart_flush_all > finished");
  return DART_OK;
}

dart_ret_t dart_flush_local(
  dart_gptr_t gptr)
{
  MPI_Win  win;
  MPI_Comm comm   = DART_COMM_WORLD;
  int16_t  seg_id = gptr.segid;
  dart_team_unit_t team_unit_id = DART_TEAM_UNIT_ID(gptr.unitid);

  DART_LOG_DEBUG("dart_flush_local() gptr: "
                 "unitid:%d offset:%"PRIu64" segid:%d teamid:%d",
                 gptr.unitid, gptr.addr_or_offs.offset,
                 gptr.segid,  gptr.teamid);

  if (gptr.unitid < 0) {
    DART_LOG_ERROR("dart_flush_local ! failed: gptr.unitid < 0");
    return DART_ERR_INVAL;
  }

  if (seg_id) {
    dart_team_data_t *team_data = dart_adapt_teamlist_get(gptr.teamid);
    if (team_data == NULL) {
      DART_LOG_ERROR("dart_flush_local ! failed: Unknown team %i!", gptr.segid);
      return DART_ERR_INVAL;
    }

    win = team_data->window;
    comm = team_data->comm;
    DART_LOG_DEBUG("dart_flush_local() win:%"PRIu64" seg:%d unit:%d",
                   (unsigned long)win, seg_id, team_unit_id.id);
  } else {
    win = dart_win_local_alloc;
    DART_LOG_DEBUG("dart_flush_local() lwin:%"PRIu64" seg:%d unit:%d",
                   (unsigned long)win, seg_id, team_unit_id.id);
  }
  DART_LOG_TRACE("dart_flush_local: MPI_Win_flush_local");
  if (MPI_Win_flush_local(team_unit_id.id, win) != MPI_SUCCESS) {
    DART_LOG_ERROR("dart_flush_all ! MPI_Win_flush_local failed!");
    return DART_ERR_OTHER;
  }

  // trigger progress
  int flag;
  MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &flag, MPI_STATUS_IGNORE);

  DART_LOG_DEBUG("dart_flush_local > finished");
  return DART_OK;
}

dart_ret_t dart_flush_local_all(
  dart_gptr_t gptr)
{
  MPI_Win  win;
  MPI_Comm comm   = DART_COMM_WORLD;
  int16_t  seg_id = gptr.segid;
  DART_LOG_DEBUG("dart_flush_local_all() gptr: "
                 "unitid:%d offset:%"PRIu64" segid:%d teamid:%d",
                 gptr.unitid, gptr.addr_or_offs.offset,
                 gptr.segid,  gptr.teamid);

  if (gptr.unitid < 0) {
    DART_LOG_ERROR("dart_flush_local_all ! failed: gptr.unitid < 0");
    return DART_ERR_INVAL;
  }


  if (seg_id) {
    dart_team_data_t *team_data = dart_adapt_teamlist_get(gptr.teamid);
    if (team_data == NULL) {
      DART_LOG_ERROR("dart_flush_local_all ! failed: Unknown team %i!",
                          gptr.teamid);
      return DART_ERR_INVAL;
    }
    win  = team_data->window;
    comm = team_data->comm;
  } else {
    win = dart_win_local_alloc;
  }
  if (MPI_Win_flush_local_all(win) != MPI_SUCCESS) {
    DART_LOG_ERROR("dart_flush_all ! MPI_Win_flush_local_all failed!");
    return DART_ERR_OTHER;
  }

  // trigger progress
  int flag;
  MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &flag, MPI_STATUS_IGNORE);

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
      free(mpi_req);
      free(mpi_sta);
      return DART_OK;
    }
    /*
     * copy MPI requests back to DART handles:
     */
    DART_LOG_TRACE("dart_waitall_local: "
                   "releasing DART handles");
    r_n = 0;
    for (i = 0; i < num_handles; i++) {
      if (handle[i]) {
        if (handle[i]->request) {
          DART_LOG_TRACE("dart_waitall_local: -- mpi_sta[%"PRIu64"].MPI_SOURCE:"
                         " %d",
                         r_n, mpi_sta[r_n].MPI_SOURCE);
          DART_LOG_TRACE("dart_waitall_local: -- mpi_sta[%"PRIu64"].MPI_ERROR:"
                         "  %d:%s",
                         r_n,
                         mpi_sta[r_n].MPI_ERROR,
                         DART__MPI__ERROR_STR(mpi_sta[r_n].MPI_ERROR));
          if (mpi_sta[r_n].MPI_ERROR != MPI_SUCCESS) {
            DART_LOG_ERROR("dart_waitall_local: detected unsuccesful request "
                           "%zu mpi_sta[%zu] = %d (%s)",
                           i,
                           r_n,
                           mpi_sta[r_n].MPI_ERROR,
                           DART__MPI__ERROR_STR(mpi_sta[r_n].MPI_ERROR));
          }
          r_n++;
        }
        DART_LOG_TRACE("dart_waitall_local: free handle[%zu] %p",
                       i, (void*)(handle[i]));
        free(handle[i]);
        handle[i] = NULL;
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
      free(mpi_req);
      free(mpi_sta);
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

  DART_LOG_DEBUG("dart_barrier() barrier count: %d", _dart_barrier_count);

  if (teamid == DART_UNDEFINED_TEAM_ID) {
    DART_LOG_ERROR("dart_barrier ! failed: team may not be DART_UNDEFINED_TEAM_ID");
    return DART_ERR_INVAL;
  }

  _dart_barrier_count++;

  dart_team_data_t *team_data = dart_adapt_teamlist_get(teamid);
  if (team_data == NULL) {
    return DART_ERR_INVAL;
  }
  /* Fetch proper communicator from teams. */
  comm = team_data->comm;
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
  MPI_Datatype mpi_dtype = dart__mpi__datatype(dtype);

  DART_LOG_TRACE("dart_bcast() root:%d team:%d nelem:%"PRIu64"",
                 root.id, teamid, nelem);

  if (root.id < 0) {
    DART_LOG_ERROR("dart_bcast ! failed: root < 0");
    return DART_ERR_INVAL;
  }

  if (teamid == DART_UNDEFINED_TEAM_ID) {
    DART_LOG_ERROR("dart_bcast ! failed: team may not be DART_UNDEFINED_TEAM_ID");
    return DART_ERR_INVAL;
  }

  /*
   * MPI uses offset type int, do not copy more than INT_MAX elements:
   */
  if (nelem > INT_MAX) {
    DART_LOG_ERROR("dart_bcast ! failed: nelem > INT_MAX");
    return DART_ERR_INVAL;
  }

  dart_team_data_t *team_data = dart_adapt_teamlist_get(teamid);
  if (team_data == NULL) {
    DART_LOG_ERROR("dart_bcast ! root:%d -> team:%d "
                   "dart_adapt_teamlist_convert failed", root.id, teamid);
    return DART_ERR_INVAL;
  }
  comm = team_data->comm;
  if (MPI_Bcast(buf, nelem, mpi_dtype, root.id, comm) != MPI_SUCCESS) {
    DART_LOG_ERROR("dart_bcast ! root:%d -> team:%d "
                   "MPI_Bcast failed", root.id, teamid);
    return DART_ERR_INVAL;
  }
  DART_LOG_TRACE("dart_bcast > root:%d team:%d nelem:%zu finished",
                 root.id, teamid, nelem);
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
  MPI_Datatype mpi_dtype = dart__mpi__datatype(dtype);
  MPI_Comm     comm;

  if (root.id < 0) {
    DART_LOG_ERROR("dart_scatter ! failed: root < 0");
    return DART_ERR_INVAL;
  }

  if (teamid == DART_UNDEFINED_TEAM_ID) {
    DART_LOG_ERROR("dart_scatter ! failed: team may not be DART_UNDEFINED_TEAM_ID");
    return DART_ERR_INVAL;
  }

  /*
   * MPI uses offset type int, do not copy more than INT_MAX elements:
   */
  if (nelem > INT_MAX) {
    DART_LOG_ERROR("dart_scatter ! failed: nelem > INT_MAX");
    return DART_ERR_INVAL;
  }

  dart_team_data_t *team_data = dart_adapt_teamlist_get(teamid);
  if (team_data == NULL) {
    return DART_ERR_INVAL;
  }
  comm = team_data->comm;
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
  MPI_Datatype mpi_dtype = dart__mpi__datatype(dtype);
  MPI_Comm     comm;

  if (root.id < 0) {
    DART_LOG_ERROR("dart_gather ! failed: root < 0");
    return DART_ERR_INVAL;
  }

  if (teamid == DART_UNDEFINED_TEAM_ID) {
    DART_LOG_ERROR("dart_gather ! failed: team may not be DART_UNDEFINED_TEAM_ID");
    return DART_ERR_INVAL;
  }

  /*
   * MPI uses offset type int, do not copy more than INT_MAX elements:
   */
  if (nelem > INT_MAX) {
    DART_LOG_ERROR("dart_gather ! failed: nelem > INT_MAX");
    return DART_ERR_INVAL;
  }

  dart_team_data_t *team_data = dart_adapt_teamlist_get(teamid);
  if (team_data == NULL) {
    return DART_ERR_INVAL;
  }
  comm = team_data->comm;
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
  MPI_Datatype mpi_dtype = dart__mpi__datatype(dtype);
  MPI_Comm     comm;
  DART_LOG_TRACE("dart_allgather() team:%d nelem:%"PRIu64"",
                 teamid, nelem);

  if (teamid == DART_UNDEFINED_TEAM_ID) {
    DART_LOG_ERROR("dart_accumulate ! failed: team may not be DART_UNDEFINED_TEAM_ID");
    return DART_ERR_INVAL;
  }

  /*
   * MPI uses offset type int, do not copy more than INT_MAX elements:
   */
  if (nelem > INT_MAX) {
    DART_LOG_ERROR("dart_allgather ! failed: nelem > INT_MAX");
    return DART_ERR_INVAL;
  }

  dart_team_data_t *team_data = dart_adapt_teamlist_get(teamid);
  if (team_data == NULL) {
    DART_LOG_ERROR("dart_allgather ! team:%d "
                   "dart_adapt_teamlist_convert failed", teamid);
    return DART_ERR_INVAL;
  }
  if (sendbuf == recvbuf || NULL == sendbuf) {
    sendbuf = MPI_IN_PLACE;
  }
  comm = team_data->comm;
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
  MPI_Datatype mpi_dtype = dart__mpi__datatype(dtype);
  MPI_Comm     comm;
  int          comm_size;
  DART_LOG_TRACE("dart_allgatherv() team:%d nsendelem:%"PRIu64"",
                 teamid, nsendelem);

  if (teamid == DART_UNDEFINED_TEAM_ID) {
    DART_LOG_ERROR("dart_allgatherv ! failed: team may not be DART_UNDEFINED_TEAM_ID");
    return DART_ERR_INVAL;
  }

  /*
   * MPI uses offset type int, do not copy more than INT_MAX elements:
   */
  if (nsendelem > INT_MAX) {
    DART_LOG_ERROR("dart_allgather ! failed: nelem > INT_MAX");
    return DART_ERR_INVAL;
  }

  dart_team_data_t *team_data = dart_adapt_teamlist_get(teamid);
  if (team_data == NULL) {
    DART_LOG_ERROR("dart_allgatherv ! team:%d "
                   "dart_adapt_teamlist_convert failed", teamid);
    return DART_ERR_INVAL;
  }
  if (sendbuf == recvbuf || NULL == sendbuf) {
    sendbuf = MPI_IN_PLACE;
  }
  comm = team_data->comm;

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
  MPI_Op       mpi_op    = dart__mpi__op(op);
  MPI_Datatype mpi_dtype = dart__mpi__datatype(dtype);

  if (team == DART_UNDEFINED_TEAM_ID) {
    DART_LOG_ERROR("dart_allreduce ! failed: team may not be DART_UNDEFINED_TEAM_ID");
    return DART_ERR_INVAL;
  }

  /*
   * MPI uses offset type int, do not copy more than INT_MAX elements:
   */
  if (nelem > INT_MAX) {
    DART_LOG_ERROR("dart_allreduce ! failed: nelem > INT_MAX");
    return DART_ERR_INVAL;
  }

  dart_team_data_t *team_data = dart_adapt_teamlist_get(team);
  if (team_data == NULL) {
    return DART_ERR_INVAL;
  }
  comm = team_data->comm;
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
  MPI_Comm     comm;
  MPI_Op       mpi_op    = dart__mpi__op(op);
  MPI_Datatype mpi_dtype = dart__mpi__datatype(dtype);

  if (root.id < 0) {
    DART_LOG_ERROR("dart_reduce ! failed: root < 0");
    return DART_ERR_INVAL;
  }

  if (team == DART_UNDEFINED_TEAM_ID) {
    DART_LOG_ERROR("dart_reduce ! failed: team may not be DART_UNDEFINED_TEAM_ID");
    return DART_ERR_INVAL;
  }

  /*
   * MPI uses offset type int, do not copy more than INT_MAX elements:
   */
  if (nelem > INT_MAX) {
    DART_LOG_ERROR("dart_allreduce ! failed: nelem > INT_MAX");
    return DART_ERR_INVAL;
  }

  dart_team_data_t *team_data = dart_adapt_teamlist_get(team);
  if (team_data == NULL) {
    return DART_ERR_INVAL;
  }
  comm = team_data->comm;
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
  MPI_Datatype mpi_dtype = dart__mpi__datatype(dtype);
  dart_team_t team = DART_TEAM_ALL;

  if (unit.id < 0) {
    DART_LOG_ERROR("dart_send ! failed: unit < 0");
    return DART_ERR_INVAL;
  }

  /*
   * MPI uses offset type int, do not copy more than INT_MAX elements:
   */
  if (nelem > INT_MAX) {
    DART_LOG_ERROR("dart_send ! failed: nelem > INT_MAX");
    return DART_ERR_INVAL;
  }

  dart_team_data_t *team_data = dart_adapt_teamlist_get(team);
  if (team_data == NULL) {
    return DART_ERR_INVAL;
  }
  comm = team_data->comm;
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
  MPI_Datatype mpi_dtype = dart__mpi__datatype(dtype);
  dart_team_t team = DART_TEAM_ALL;

  if (unit.id < 0) {
    DART_LOG_ERROR("dart_recv ! failed: unit < 0");
    return DART_ERR_INVAL;
  }

  /*
   * MPI uses offset type int, do not copy more than INT_MAX elements:
   */
  if (nelem > INT_MAX) {
    DART_LOG_ERROR("dart_recv ! failed: nelem > INT_MAX");
    return DART_ERR_INVAL;
  }

  dart_team_data_t *team_data = dart_adapt_teamlist_get(team);
  if (team_data == NULL) {
    return DART_ERR_INVAL;
  }
  comm = team_data->comm;
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
  MPI_Datatype mpi_send_dtype = dart__mpi__datatype(send_dtype);
  MPI_Datatype mpi_recv_dtype = dart__mpi__datatype(recv_dtype);
  dart_team_t team = DART_TEAM_ALL;

  if (src.id < 0 || dest.id < 0) {
    DART_LOG_ERROR("dart_send ! failed: src (%i) or dest (%i) unit invalid", src.id, dest.id);
    return DART_ERR_INVAL;
  }

  /*
   * MPI uses offset type int, do not copy more than INT_MAX elements:
   */
  if (send_nelem > INT_MAX || recv_nelem > INT_MAX) {
    DART_LOG_ERROR("dart_sendrecv ! failed: nelem > INT_MAX");
    return DART_ERR_INVAL;
  }

  dart_team_data_t *team_data = dart_adapt_teamlist_get(team);
  if (team_data == NULL) {
    return DART_ERR_INVAL;
  }
  comm = team_data->comm;
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


