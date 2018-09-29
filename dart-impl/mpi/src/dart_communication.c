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


#define CHECK_UNITID_RANGE(_unitid, _team_data)                             \
  do {                                                                      \
    if (dart__unlikely(_unitid.id < 0 || _unitid.id > _team_data->size)) {  \
      DART_LOG_ERROR("%s ! failed: unitid out of range 0 <= %d < %d",       \
          __FUNCTION__, _unitid.id, _team_data->size);           \
      return DART_ERR_INVAL;                                                \
    }                                                                       \
  } while (0)

#define CHECK_EQUAL_BASETYPE(_src_type, _dst_type) \
  do {                                                                        \
    if (dart__unlikely(!dart__mpi__datatype_samebase(_src_type, _dst_type))){ \
      char *src_name = dart__mpi__datatype_name(_src_type);                   \
      char *dst_name = dart__mpi__datatype_name(dst_type);                    \
      DART_LOG_ERROR("%s ! Cannot convert base-types (%s vs %s)",             \
          __FUNCTION__, src_name, dst_name);                        \
      free(src_name);                                                         \
      free(dst_name);                                                         \
      return DART_ERR_INVAL;                                                  \
    }                                                                         \
  } while (0)

#define CHECK_NUM_ELEM(_src_type, _dst_type, _num_elem)                       \
  do {                                                                        \
    size_t src_num_elem = dart__mpi__datatype_num_elem(_src_type);            \
    size_t dst_num_elem = dart__mpi__datatype_num_elem(_dst_type);            \
    if ((_num_elem % src_num_elem) != 0 || (_num_elem % dst_num_elem) != 0) { \
      char *src_name = dart__mpi__datatype_name(_src_type);                   \
      char *dst_name = dart__mpi__datatype_name(dst_type);                    \
      DART_LOG_ERROR(                                                         \
          "%s ! Type-mismatch would lead to truncation (%s vs %s with %zu elems)",\
          __FUNCTION__, src_name, dst_name, _num_elem);             \
      free(src_name);                                                         \
      free(dst_name);                                                         \
      return DART_ERR_INVAL;                                                  \
    }                                                                         \
  } while (0)

#define CHECK_TYPE_CONSTRAINTS(_src_type, _dst_type, _num_elem)               \
  CHECK_EQUAL_BASETYPE(_src_type, _dst_type);                                 \
  CHECK_NUM_ELEM(_src_type, _dst_type, _num_elem);

/**
 * Temporary space allocation:
 *   - on the stack for allocations <=64B
 *   - on the heap otherwise
 * Mainly meant to be used in dart_waitall* and dart_testall*
 */
#define ALLOC_TMP(__size) ((__size)<=64) ? alloca((__size)) : malloc((__size))
/**
 * Temporary space release: calls free() for allocations >64B
 */
#define FREE_TMP(__size, __ptr)        \
  do {                                 \
    if ((__size) > 64)                 \
    free(__ptr);                     \
  } while (0)

/** DART handle type for non-blocking one-sided operations. */
struct dart_handle_struct
{
  MPI_Request reqs[2];   // a large transfer might consist of two operations
  MPI_Win     win;
  dart_unit_t dest;
  uint8_t     num_reqs;
  bool        needs_flush;
};

/**
 * Help to check for return of MPI call.
 * Since DART currently does not define an MPI error handler the abort will not
 * be reached.
 */
#define CHECK_MPI_RET(__call, __name)                      \
  do {                                                     \
    if (dart__unlikely(__call != MPI_SUCCESS)) {           \
      DART_LOG_ERROR("%s ! %s failed!", __func__, __name); \
      dart_abort(DART_EXIT_ABORT);                         \
    }                                                      \
  } while (0)


#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
static dart_ret_t get_shared_mem(
    const dart_team_data_t    * team_data,
    const dart_segment_info_t * seginfo,
    void                      * dest,
    uint64_t                    offset,
    dart_team_unit_t            unitid,
    size_t                      nelem,
    dart_datatype_t             dtype)
{
  DART_LOG_DEBUG("dart_get: using shared memory window in segment %d enabled",
      seginfo->segid);
  dart_team_unit_t luid    = team_data->sharedmem_tab[unitid.id];
  char *           baseptr = seginfo->baseptr[luid.id];

  baseptr += offset;
  DART_LOG_DEBUG(
      "dart_get: memcpy %zu bytes", nelem * dart__mpi__datatype_sizeof(dtype));
  memcpy(dest, baseptr, nelem * dart__mpi__datatype_sizeof(dtype));
  return DART_OK;
}

static dart_ret_t put_shared_mem(
    const dart_team_data_t    * team_data,
    const dart_segment_info_t * seginfo,
    const void                * src,
    uint64_t                    offset,
    dart_team_unit_t            unitid,
    size_t                      nelem,
    dart_datatype_t             dtype)
{
  DART_LOG_DEBUG("dart_get: using shared memory window in segment %d enabled",
      seginfo->segid);
  dart_team_unit_t luid    = team_data->sharedmem_tab[unitid.id];
  char *           baseptr = seginfo->baseptr[luid.id];

  baseptr += offset;
  DART_LOG_DEBUG(
      "dart_get: memcpy %zu bytes", nelem * dart__mpi__datatype_sizeof(dtype));
  memcpy(baseptr, src, nelem * dart__mpi__datatype_sizeof(dtype));
  return DART_OK;
}
#endif // !defined(DART_MPI_DISABLE_SHARED_WINDOWS)

/**
 * Internal implementations of put/get with and without handles for
 * basic data types and complex data types.
 */

static __attribute__((always_inline)) inline
  int
dart__mpi__get(
    void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
    int target_rank, MPI_Aint target_disp, int target_count,
    MPI_Datatype target_datatype, MPI_Win win,
    MPI_Request *reqs, uint8_t * num_reqs)
{
  if (reqs != NULL) {
    return MPI_Rget(origin_addr, origin_count, origin_datatype,
        target_rank, target_disp, target_count, target_datatype,
        win, &reqs[(*num_reqs)++]);
  } else {
    return MPI_Get(origin_addr, origin_count, origin_datatype,
        target_rank, target_disp, target_count,
        target_datatype, win);
  }
}

static __attribute__((always_inline)) inline
  int
dart__mpi__put(
    const void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
    int target_rank, MPI_Aint target_disp, int target_count,
    MPI_Datatype target_datatype, MPI_Win win,
    MPI_Request *reqs, uint8_t * num_reqs)
{
  if (reqs != NULL) {
    return MPI_Rput(origin_addr, origin_count, origin_datatype,
        target_rank, target_disp, target_count, target_datatype,
        win, &reqs[(*num_reqs)++]);
  } else {
    return MPI_Put(origin_addr, origin_count, origin_datatype,
        target_rank, target_disp, target_count,
        target_datatype, win);
  }
}

static inline
  dart_ret_t
dart__mpi__get_basic(
    const dart_team_data_t    * team_data,
    dart_team_unit_t            team_unit_id,
    const dart_segment_info_t * seginfo,
    void                      * dest,
    uint64_t                    offset,
    size_t                      nelem,
    dart_datatype_t             dtype,
    MPI_Request               * reqs,
    uint8_t                   * num_reqs)
{
  if (num_reqs) *num_reqs = 0;

  if (team_data->unitid == team_unit_id.id) {
    // use direct memcpy if we are on the same unit
    memcpy(dest, seginfo->selfbaseptr + offset,
        nelem * dart__mpi__datatype_sizeof(dtype));
    DART_LOG_DEBUG("dart_get: memcpy nelem:%zu "
        "source (coll.): offset:%lu -> dest: %p",
        nelem, offset, dest);
    return DART_OK;
  }

#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
  DART_LOG_DEBUG("dart_get: shared windows enabled");
  if (seginfo->segid >= 0 && team_data->sharedmem_tab[team_unit_id.id].id >= 0) {
    return get_shared_mem(team_data, seginfo, dest, offset,
        team_unit_id, nelem, dtype);
  }
#else
  DART_LOG_DEBUG("dart_get: shared windows disabled");
#endif // !defined(DART_MPI_DISABLE_SHARED_WINDOWS)

  /*
   * MPI uses offset type int, chunk up the get if necessary
   */
  const size_t nchunks   = nelem / MAX_CONTIG_ELEMENTS;
  const size_t remainder = nelem % MAX_CONTIG_ELEMENTS;

  // source on another node or shared memory windows disabled
  MPI_Win win      = seginfo->win;
  offset          += dart_segment_disp(seginfo, team_unit_id);
  char * dest_ptr  = (char*) dest;

  if (nchunks > 0) {
    DART_LOG_TRACE("dart_get:  MPI_Get (dest %p, size %zu)",
        dest_ptr, nchunks * MAX_CONTIG_ELEMENTS);
    CHECK_MPI_RET(
        dart__mpi__get(dest_ptr, nchunks,
          dart__mpi__datatype_maxtype(dtype),
          team_unit_id.id,
          offset,
          nchunks,
          dart__mpi__datatype_maxtype(dtype),
          win, reqs, num_reqs),
        "MPI_Get");
    offset   += nchunks * MAX_CONTIG_ELEMENTS;
    dest_ptr += nchunks * MAX_CONTIG_ELEMENTS;
  }

  if (remainder > 0) {
    DART_LOG_TRACE("dart_get:  MPI_Get (dest %p, size %zu)",
        dest_ptr, remainder);
    CHECK_MPI_RET(
        dart__mpi__get(dest_ptr,
          remainder,
          dart__mpi__datatype_struct(dtype)->contiguous.mpi_type,
          team_unit_id.id,
          offset,
          remainder,
          dart__mpi__datatype_struct(dtype)->contiguous.mpi_type,
          win, reqs, num_reqs),
        "MPI_Get");
  }
  return DART_OK;
}

static inline
  dart_ret_t
dart__mpi__get_complex(
    dart_team_unit_t            team_unit_id,
    const dart_segment_info_t * seginfo,
    void                      * dest,
    uint64_t                    offset,
    size_t                      nelem,
    dart_datatype_t             src_type,
    dart_datatype_t             dst_type,
    MPI_Request               * reqs,
    uint8_t                   * num_reqs)
{
  if (num_reqs != NULL) *num_reqs = 0;

  CHECK_TYPE_CONSTRAINTS(src_type, dst_type, nelem);

  MPI_Win win     = seginfo->win;
  char * dest_ptr = (char*) dest;
  offset         += dart_segment_disp(seginfo, team_unit_id);

  MPI_Datatype src_mpi_type, dst_mpi_type;
  int src_num_elem, dst_num_elem;
  dart__mpi__datatype_convert_mpi(
      src_type, nelem, &src_mpi_type, &src_num_elem);
  if (src_type != dst_type) {
    dart__mpi__datatype_convert_mpi(
        dst_type, nelem, &dst_mpi_type, &dst_num_elem);
  } else {
    dst_mpi_type = src_mpi_type;
    dst_num_elem = src_num_elem;
  }

  DART_LOG_TRACE("dart_get:  MPI_Rget (dest %p, size %zu)", dest_ptr, nelem);
  CHECK_MPI_RET(
      dart__mpi__get(dest_ptr,
        dst_num_elem,
        dst_mpi_type,
        team_unit_id.id,
        offset,
        src_num_elem,
        src_mpi_type,
        win,
        reqs, num_reqs),
      "MPI_Rget");
  // clean-up strided data types
  if (dart__mpi__datatype_isstrided(src_type)) {
    dart__mpi__destroy_strided_mpi(&src_mpi_type);
  }
  if (src_type != dst_type && dart__mpi__datatype_isstrided(dst_type)) {
    dart__mpi__destroy_strided_mpi(&dst_mpi_type);
  }
  return DART_OK;
}

static inline
  dart_ret_t
dart__mpi__put_basic(
    const dart_team_data_t    * team_data,
    dart_team_unit_t            team_unit_id,
    const dart_segment_info_t * seginfo,
    const void                * src,
    uint64_t                    offset,
    size_t                      nelem,
    dart_datatype_t             dtype,
    MPI_Request               * reqs,
    uint8_t                   * num_reqs,
    bool                      * flush_required_ptr)
{
  if (num_reqs) *num_reqs = 0;

  /* copy data directly if we are on the same unit */
  if (team_unit_id.id == team_data->unitid) {
    if (flush_required_ptr) *flush_required_ptr = false;
    memcpy(seginfo->selfbaseptr + offset, src,
        nelem * dart__mpi__datatype_sizeof(dtype));
    DART_LOG_DEBUG("dart_put: memcpy nelem:%zu (from global allocation)"
        "offset: %"PRIu64"", nelem, offset);
    return DART_OK;
  }

#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
  DART_LOG_DEBUG("dart_put: shared windows enabled");
  if (seginfo->segid >= 0 && team_data->sharedmem_tab[team_unit_id.id].id >= 0) {
    if (flush_required_ptr) *flush_required_ptr = false;
    return put_shared_mem(team_data, seginfo, src, offset,
        team_unit_id, nelem, dtype);
  }
#else
  DART_LOG_DEBUG("dart_put: shared windows disabled");
#endif /* !defined(DART_MPI_DISABLE_SHARED_WINDOWS) */

  if (flush_required_ptr) *flush_required_ptr = true;

  // source on another node or shared memory windows disabled
  MPI_Win win            = seginfo->win;
  offset                += dart_segment_disp(seginfo, team_unit_id);
  const char * src_ptr   = (const char*) src;

  // chunk up the put
  const size_t nchunks   = nelem / MAX_CONTIG_ELEMENTS;
  const size_t remainder = nelem % MAX_CONTIG_ELEMENTS;

  if (nchunks > 0) {
    DART_LOG_TRACE("dart_put:  MPI_Rput (src %p, size %zu)",
        src_ptr, nchunks * MAX_CONTIG_ELEMENTS);
    CHECK_MPI_RET(
        dart__mpi__put(src_ptr,
          nchunks,
          dart__mpi__datatype_struct(dtype)->contiguous.max_type,
          team_unit_id.id,
          offset,
          nchunks,
          dart__mpi__datatype_struct(dtype)->contiguous.max_type,
          win,
          reqs, num_reqs),
        "MPI_Put");
    offset  += nchunks * MAX_CONTIG_ELEMENTS;
    src_ptr += nchunks * MAX_CONTIG_ELEMENTS;
  }

  if (remainder > 0) {
    DART_LOG_TRACE("dart_put:  MPI_Put (src %p, size %zu)", src_ptr, remainder);

    CHECK_MPI_RET(
        dart__mpi__put(src_ptr,
          remainder,
          dart__mpi__datatype_struct(dtype)->contiguous.mpi_type,
          team_unit_id.id,
          offset,
          remainder,
          dart__mpi__datatype_struct(dtype)->contiguous.mpi_type,
          win,
          reqs, num_reqs),
        "MPI_Put");
  }
  return DART_OK;
}

static inline
  dart_ret_t
dart__mpi__put_complex(
    dart_team_unit_t            team_unit_id,
    const dart_segment_info_t * seginfo,
    const void                * src,
    uint64_t                    offset,
    size_t                      nelem,
    dart_datatype_t             src_type,
    dart_datatype_t             dst_type,
    MPI_Request               * reqs,
    uint8_t                   * num_reqs,
    bool                      * flush_required_ptr)
{
  if (flush_required_ptr) *flush_required_ptr = true;
  if (num_reqs) *num_reqs = 0;

  // slow path for derived types
  CHECK_TYPE_CONSTRAINTS(src_type, dst_type, nelem);

  MPI_Win win            = seginfo->win;
  const char * src_ptr   = (const char*) src;
  offset                += dart_segment_disp(seginfo, team_unit_id);

  MPI_Datatype src_mpi_type, dst_mpi_type;
  int src_num_elem, dst_num_elem;
  dart__mpi__datatype_convert_mpi(
      src_type, nelem, &src_mpi_type, &src_num_elem);
  if (src_type != dst_type) {
    dart__mpi__datatype_convert_mpi(
        dst_type, nelem, &dst_mpi_type, &dst_num_elem);
  } else {
    dst_mpi_type = src_mpi_type;
    dst_num_elem = src_num_elem;
  }

  DART_LOG_TRACE(
      "dart_put:  MPI_Put (src %p, size %zu, src_type %p, dst_type %p)",
      src_ptr, nelem, src_mpi_type, dst_mpi_type);

  CHECK_MPI_RET(
      dart__mpi__put(src_ptr,
        src_num_elem,
        src_mpi_type,
        team_unit_id.id,
        offset,
        dst_num_elem,
        dst_mpi_type,
        win,
        reqs, num_reqs),
      "MPI_Put");

  // clean-up strided data types
  if (dart__mpi__datatype_isstrided(src_type)) {
    dart__mpi__destroy_strided_mpi(&src_mpi_type);
  }
  if (src_type != dst_type && dart__mpi__datatype_isstrided(dst_type)) {
    dart__mpi__destroy_strided_mpi(&dst_mpi_type);
  }
  return DART_OK;
}

/**
 * Public interface for put/get.
 */

dart_ret_t dart_get(
    void            * dest,
    dart_gptr_t       gptr,
    size_t            nelem,
    dart_datatype_t   src_type,
    dart_datatype_t   dst_type)
{
  uint64_t         offset       = gptr.addr_or_offs.offset;
  int16_t          seg_id       = gptr.segid;
  dart_team_unit_t team_unit_id = DART_TEAM_UNIT_ID(gptr.unitid);
  dart_team_t      teamid       = gptr.teamid;

  dart_team_data_t *team_data = dart_adapt_teamlist_get(teamid);
  if (dart__unlikely(team_data == NULL)) {
    DART_LOG_ERROR("dart_get ! failed: Unknown team %i!", teamid);
    return DART_ERR_INVAL;
  }
  CHECK_UNITID_RANGE(team_unit_id, team_data);

  DART_LOG_DEBUG("dart_get() uid:%d o:%"PRIu64" s:%d t:%d nelem:%zu",
      team_unit_id.id, offset, seg_id, teamid, nelem);

  dart_segment_info_t *seginfo = dart_segment_get_info(
      &(team_data->segdata), seg_id);
  if (dart__unlikely(seginfo == NULL)) {
    DART_LOG_ERROR("dart_get ! "
        "Unknown segment %i on team %i", seg_id, teamid);
    return DART_ERR_INVAL;
  }

  dart_ret_t ret = DART_OK;

  // leave complex data type handling to MPI
  if (dart__mpi__datatype_iscontiguous(src_type) &&
      dart__mpi__datatype_iscontiguous(dst_type)) {
    // fast-path for basic types
    CHECK_EQUAL_BASETYPE(src_type, dst_type);
    ret = dart__mpi__get_basic(team_data, team_unit_id, seginfo, dest,
        offset, nelem, src_type, NULL, NULL);
  } else {
    // slow path for derived types
    ret = dart__mpi__get_complex(team_unit_id, seginfo, dest,
        offset, nelem, src_type, dst_type, NULL, NULL);
  }

  DART_LOG_DEBUG("dart_get > finished");
  return ret;
}

dart_ret_t dart_put(
    dart_gptr_t       gptr,
    const void      * src,
    size_t            nelem,
    dart_datatype_t   src_type,
    dart_datatype_t   dst_type)
{
  uint64_t         offset       = gptr.addr_or_offs.offset;
  int16_t          seg_id       = gptr.segid;
  dart_team_unit_t team_unit_id = DART_TEAM_UNIT_ID(gptr.unitid);
  dart_team_t      teamid       = gptr.teamid;

  CHECK_EQUAL_BASETYPE(src_type, dst_type);

  dart_team_data_t *team_data = dart_adapt_teamlist_get(teamid);
  if (dart__unlikely(team_data == NULL)) {
    DART_LOG_ERROR("dart_put ! failed: Unknown team %i!", teamid);
    return DART_ERR_INVAL;
  }

  CHECK_UNITID_RANGE(team_unit_id, team_data);

  dart_segment_info_t *seginfo = dart_segment_get_info(
      &(team_data->segdata), seg_id);
  if (dart__unlikely(seginfo == NULL)) {
    DART_LOG_ERROR("dart_put ! "
        "Unknown segment %i on team %i", seg_id, teamid);
    return DART_ERR_INVAL;
  }

  dart_ret_t ret = DART_OK;

  if (dart__mpi__datatype_iscontiguous(src_type) &&
      dart__mpi__datatype_iscontiguous(dst_type)) {
    // fast path for basic data types
    CHECK_EQUAL_BASETYPE(src_type, dst_type);
    ret = dart__mpi__put_basic(team_data, team_unit_id, seginfo, src,
        offset, nelem, src_type,
        NULL, NULL, NULL);
  } else {
    // slow path for complex data types
    ret = dart__mpi__put_complex(team_unit_id, seginfo, src,
        offset, nelem, src_type, dst_type,
        NULL, NULL, NULL);
  }

  return ret;
}

dart_ret_t dart_accumulate(
    dart_gptr_t      gptr,
    const void     * values,
    size_t           nelem,
    dart_datatype_t  dtype,
    dart_operation_t op)
{
  dart_team_unit_t  team_unit_id = DART_TEAM_UNIT_ID(gptr.unitid);
  uint64_t    offset = gptr.addr_or_offs.offset;
  int16_t     seg_id = gptr.segid;
  dart_team_t teamid = gptr.teamid;

  if (dart__unlikely(op > DART_OP_LAST)) {
    DART_LOG_ERROR("Custom reduction operators not allowed in dart_accumulate!");
    return DART_ERR_INVAL;
  }

  CHECK_IS_BASICTYPE(dtype);
  MPI_Op      mpi_op = dart__mpi__op(op, dtype);

  dart_team_data_t *team_data = dart_adapt_teamlist_get(teamid);
  if (dart__unlikely(team_data == NULL)) {
    DART_LOG_ERROR("dart_accumulate ! failed: Unknown team %i!", teamid);
    return DART_ERR_INVAL;
  }

  CHECK_UNITID_RANGE(team_unit_id, team_data);

  DART_LOG_DEBUG("dart_accumulate() nelem:%zu dtype:%ld op:%d unit:%d",
      nelem, dtype, op, team_unit_id.id);

  dart_segment_info_t *seginfo = dart_segment_get_info(
      &(team_data->segdata), seg_id);
  if (dart__unlikely(seginfo == NULL)) {
    DART_LOG_ERROR("dart_accumulate ! "
        "Unknown segment %i on team %i", seg_id, teamid);
    return DART_ERR_INVAL;
  }

  MPI_Win win = seginfo->win;
  offset     += dart_segment_disp(seginfo, team_unit_id);

  // chunk up the put
  const size_t nchunks   = nelem / MAX_CONTIG_ELEMENTS;
  const size_t remainder = nelem % MAX_CONTIG_ELEMENTS;
  const char * src_ptr   = (const char*) values;

  if (nchunks > 0) {
    DART_LOG_TRACE("dart_accumulate:  MPI_Accumulate (src %p, size %zu)",
        src_ptr, nchunks * MAX_CONTIG_ELEMENTS);
    CHECK_MPI_RET(
        MPI_Accumulate(
          src_ptr,
          nchunks,
          dart__mpi__datatype_maxtype(dtype),
          team_unit_id.id,
          offset,
          nchunks,
          dart__mpi__datatype_maxtype(dtype),
          mpi_op,
          win),
        "MPI_Accumulate");
    offset  += nchunks * MAX_CONTIG_ELEMENTS;
    src_ptr += nchunks * MAX_CONTIG_ELEMENTS;
  }

  if (remainder > 0) {
    DART_LOG_TRACE("dart_accumulate:  MPI_Accumulate (src %p, size %zu)",
        src_ptr, remainder);

    MPI_Datatype mpi_dtype = dart__mpi__datatype_struct(dtype)->contiguous.mpi_type;
    CHECK_MPI_RET(
        MPI_Accumulate(
          src_ptr,
          remainder,
          mpi_dtype,
          team_unit_id.id,
          offset,
          remainder,
          mpi_dtype,
          mpi_op,
          win),
        "MPI_Accumulate");
  }

  DART_LOG_DEBUG("dart_accumulate > finished");
  return DART_OK;
}


dart_ret_t dart_accumulate_blocking_local(
    dart_gptr_t      gptr,
    const void     * values,
    size_t           nelem,
    dart_datatype_t  dtype,
    dart_operation_t op)
{
  dart_team_unit_t  team_unit_id = DART_TEAM_UNIT_ID(gptr.unitid);
  uint64_t    offset = gptr.addr_or_offs.offset;
  int16_t     seg_id = gptr.segid;
  dart_team_t teamid = gptr.teamid;

  if (dart__unlikely(op > DART_OP_LAST)) {
    DART_LOG_ERROR("Custom reduction operators not allowed in "
                   "dart_accumulate_blocking_local!");
    return DART_ERR_INVAL;
  }

  CHECK_IS_BASICTYPE(dtype);
  MPI_Op      mpi_op = dart__mpi__op(op, dtype);

  dart_team_data_t *team_data = dart_adapt_teamlist_get(teamid);
  if (dart__unlikely(team_data == NULL)) {
    DART_LOG_ERROR("dart_accumulate ! failed: Unknown team %i!", teamid);
    return DART_ERR_INVAL;
  }

  CHECK_UNITID_RANGE(team_unit_id, team_data);

  DART_LOG_DEBUG("dart_accumulate() nelem:%zu dtype:%ld op:%d unit:%d",
      nelem, dtype, op, team_unit_id.id);

  dart_segment_info_t *seginfo = dart_segment_get_info(
      &(team_data->segdata), seg_id);
  if (dart__unlikely(seginfo == NULL)) {
    DART_LOG_ERROR("dart_accumulate ! "
        "Unknown segment %i on team %i", seg_id, teamid);
    return DART_ERR_INVAL;
  }

  MPI_Win win = seginfo->win;
  offset     += dart_segment_disp(seginfo, team_unit_id);

  // chunk up the put
  const size_t nchunks   = nelem / MAX_CONTIG_ELEMENTS;
  const size_t remainder = nelem % MAX_CONTIG_ELEMENTS;
  const char * src_ptr   = (const char*) values;

  MPI_Request reqs[2];
  int num_reqs = 0;

  if (nchunks > 0) {
    DART_LOG_TRACE("dart_accumulate:  MPI_Raccumulate (src %p, size %zu)",
        src_ptr, nchunks * MAX_CONTIG_ELEMENTS);
    CHECK_MPI_RET(
        MPI_Raccumulate(
          src_ptr,
          nchunks,
          dart__mpi__datatype_maxtype(dtype),
          team_unit_id.id,
          offset,
          nchunks,
          dart__mpi__datatype_maxtype(dtype),
          mpi_op,
          win,
          &reqs[num_reqs++]),
        "MPI_Accumulate");
    offset  += nchunks * MAX_CONTIG_ELEMENTS;
    src_ptr += nchunks * MAX_CONTIG_ELEMENTS;
  }

  if (remainder > 0) {
    DART_LOG_TRACE("dart_accumulate:  MPI_Raccumulate (src %p, size %zu)",
        src_ptr, remainder);

    MPI_Datatype mpi_dtype = dart__mpi__datatype_struct(dtype)->contiguous.mpi_type;
    CHECK_MPI_RET(
        MPI_Raccumulate(
          src_ptr,
          remainder,
          mpi_dtype,
          team_unit_id.id,
          offset,
          remainder,
          mpi_dtype,
          mpi_op,
          win,
          &reqs[num_reqs++]),
        "MPI_Accumulate");
  }

  MPI_Waitall(num_reqs, reqs, MPI_STATUSES_IGNORE);

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
  MPI_Datatype mpi_dtype;
  MPI_Op       mpi_op;
  dart_team_unit_t  team_unit_id = DART_TEAM_UNIT_ID(gptr.unitid);
  uint64_t    offset = gptr.addr_or_offs.offset;
  int16_t     seg_id = gptr.segid;
  dart_team_t teamid = gptr.teamid;

  if (dart__unlikely(op > DART_OP_LAST)) {
    DART_LOG_ERROR("Custom reduction operators not allowed in dart_fetch_and_op!");
    return DART_ERR_INVAL;
  }

  CHECK_IS_BASICTYPE(dtype);
  mpi_dtype          = dart__mpi__datatype_struct(dtype)->contiguous.mpi_type;
  mpi_op             = dart__mpi__op(op, dtype);

  dart_team_data_t *team_data = dart_adapt_teamlist_get(teamid);
  if (dart__unlikely(team_data == NULL)) {
    DART_LOG_ERROR("dart_fetch_and_op ! failed: Unknown team %i!", teamid);
    return DART_ERR_INVAL;
  }

  dart_segment_info_t *seginfo = dart_segment_get_info(
      &(team_data->segdata), seg_id);
  if (dart__unlikely(seginfo == NULL)) {
    DART_LOG_ERROR("dart_fetch_and_op ! "
        "Unknown segment %i on team %i", seg_id, teamid);
    return DART_ERR_INVAL;
  }

  CHECK_UNITID_RANGE(team_unit_id, team_data);

  DART_LOG_DEBUG("dart_fetch_and_op() dtype:%ld op:%d unit:%d "
      "offset:%"PRIu64" segid:%d",
      dtype, op, team_unit_id.id,
      gptr.addr_or_offs.offset, seg_id);

  MPI_Win win = seginfo->win;
  offset     += dart_segment_disp(seginfo, team_unit_id);

  CHECK_MPI_RET(
      MPI_Fetch_and_op(
        value,             // Origin address
        result,            // Result address
        mpi_dtype,         // Data type of each buffer entry
        team_unit_id.id,   // Rank of target
        offset,            // Displacement from start of window to beginning
        // of target buffer
        mpi_op,            // Reduce operation
        win),
      "MPI_Fetch_and_op");

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
  dart_team_unit_t  team_unit_id = DART_TEAM_UNIT_ID(gptr.unitid);
  uint64_t    offset = gptr.addr_or_offs.offset;
  int16_t     seg_id = gptr.segid;
  dart_team_t teamid = gptr.teamid;

  if (dtype > DART_TYPE_LONGLONG) {
    DART_LOG_ERROR("dart_compare_and_swap ! failed: "
        "only valid on integral types");
    return DART_ERR_INVAL;
  }
  MPI_Datatype mpi_dtype = dart__mpi__datatype_struct(dtype)->contiguous.mpi_type;

  dart_team_data_t *team_data = dart_adapt_teamlist_get(gptr.teamid);
  if (team_data == NULL) {
    DART_LOG_ERROR("dart_compare_and_swap ! failed: Unknown team %i!",
        gptr.teamid);
    return DART_ERR_INVAL;
  }

  CHECK_UNITID_RANGE(team_unit_id, team_data);

  DART_LOG_TRACE("dart_compare_and_swap() dtype:%ld unit:%d offset:%"PRIu64,
      dtype, team_unit_id.id, gptr.addr_or_offs.offset);

  dart_segment_info_t *seginfo = dart_segment_get_info(
      &(team_data->segdata), seg_id);
  if (dart__unlikely(seginfo == NULL)) {
    DART_LOG_ERROR("dart_compare_and_swap ! "
        "Unknown segment %i on team %i", seg_id, teamid);
    return DART_ERR_INVAL;
  }

  MPI_Win win  = seginfo->win;
  offset      += dart_segment_disp(seginfo, team_unit_id);

  CHECK_MPI_RET(
      MPI_Compare_and_swap(
        value,
        compare,
        result,
        mpi_dtype,
        team_unit_id.id,
        offset,
        win),
      "MPI_Compare_and_swap");
  DART_LOG_DEBUG("dart_compare_and_swap > finished");
  return DART_OK;
}

/* -- Non-blocking dart one-sided operations -- */

dart_ret_t dart_get_handle(
    void          * dest,
    dart_gptr_t     gptr,
    size_t          nelem,
    dart_datatype_t src_type,
    dart_datatype_t dst_type,
    dart_handle_t * handleptr)
{
  dart_team_unit_t team_unit_id = DART_TEAM_UNIT_ID(gptr.unitid);
  uint64_t         offset = gptr.addr_or_offs.offset;
  int16_t          seg_id = gptr.segid;
  dart_team_t      teamid = gptr.teamid;

  *handleptr = DART_HANDLE_NULL;

  dart_team_data_t *team_data = dart_adapt_teamlist_get(teamid);
  if (dart__unlikely(team_data == NULL)) {
    DART_LOG_ERROR("dart_get_handle ! failed: Unknown team %i!", teamid);
    return DART_ERR_INVAL;
  }

  CHECK_UNITID_RANGE(team_unit_id, team_data);

  dart_segment_info_t *seginfo = dart_segment_get_info(
      &(team_data->segdata), seg_id);
  if (dart__unlikely(seginfo == NULL)) {
    DART_LOG_ERROR("dart_get_handle ! "
        "Unknown segment %i on team %i", seg_id, teamid);
    return DART_ERR_INVAL;
  }

  MPI_Win win  = seginfo->win;

  dart_handle_t handle = calloc(1, sizeof(struct dart_handle_struct));
  handle->dest         = team_unit_id.id;
  handle->win          = win;
  handle->needs_flush  = false;


  DART_LOG_DEBUG("dart_get_handle() uid:%d o:%"PRIu64" s:%d t:%d, nelem:%zu",
      team_unit_id.id, offset, seg_id, gptr.teamid, nelem);
  DART_LOG_TRACE("dart_get_handle:  allocated handle:%p", (void *)(handle));

  dart_ret_t ret = DART_OK;

  // leave complex data type handling to MPI
  if (dart__mpi__datatype_iscontiguous(src_type) &&
      dart__mpi__datatype_iscontiguous(dst_type)) {
    // fast-path for basic types
    CHECK_EQUAL_BASETYPE(src_type, dst_type);
    ret = dart__mpi__get_basic(team_data, team_unit_id, seginfo, dest,
        offset, nelem, src_type,
        handle->reqs, &handle->num_reqs);
  } else {
    // slow path for derived types
    ret = dart__mpi__get_complex(team_unit_id, seginfo, dest,
        offset, nelem, src_type, dst_type,
        handle->reqs, &handle->num_reqs);
  }

  if (handle->num_reqs == 0) {
    free(handle);
    handle = DART_HANDLE_NULL;
  }

  *handleptr = handle;

  DART_LOG_TRACE("dart_get_handle > handle(%p) dest:%d",
                 (void*)(handle), team_unit_id.id);
  return ret;
}

dart_ret_t dart_put_handle(
  dart_gptr_t       gptr,
  const void      * src,
  size_t            nelem,
  dart_datatype_t   src_type,
  dart_datatype_t   dst_type,
  dart_handle_t   * handleptr)
{
  dart_team_unit_t  team_unit_id = DART_TEAM_UNIT_ID(gptr.unitid);
  uint64_t     offset   = gptr.addr_or_offs.offset;
  int16_t      seg_id   = gptr.segid;
  dart_team_t  teamid   = gptr.teamid;

  *handleptr = DART_HANDLE_NULL;

  CHECK_EQUAL_BASETYPE(src_type, dst_type);

  dart_team_data_t *team_data = dart_adapt_teamlist_get(teamid);
  if (dart__unlikely(team_data == NULL)) {
    DART_LOG_ERROR("dart_put_handle ! failed: Unknown team %i!", teamid);
    return DART_ERR_INVAL;
  }

  CHECK_UNITID_RANGE(team_unit_id, team_data);

  dart_segment_info_t *seginfo = dart_segment_get_info(
                                    &(team_data->segdata), seg_id);
  if (dart__unlikely(seginfo == NULL)) {
    DART_LOG_ERROR("dart_put_handle ! "
                   "Unknown segment %i on team %i", seg_id, teamid);
    return DART_ERR_INVAL;
  }

  MPI_Win win  = seginfo->win;

  // chunk up the put
  dart_handle_t handle   = calloc(1, sizeof(struct dart_handle_struct));
  handle->dest           = team_unit_id.id;
  handle->win            = win;
  handle->needs_flush    = true;

  dart_ret_t ret = DART_OK;

  if (dart__mpi__datatype_iscontiguous(src_type) &&
      dart__mpi__datatype_iscontiguous(dst_type)) {
    // fast path for basic data types
    CHECK_EQUAL_BASETYPE(src_type, dst_type);
    ret = dart__mpi__put_basic(team_data, team_unit_id, seginfo, src,
                               offset, nelem, src_type,
                               handle->reqs,
                               &handle->num_reqs,
                               &handle->needs_flush);
  } else {
    // slow path for complex data types
    ret = dart__mpi__put_complex(team_unit_id, seginfo, src,
                                 offset, nelem, src_type, dst_type,
                                 handle->reqs,
                                 &handle->num_reqs,
                                 &handle->needs_flush);
  }

  if (handle->num_reqs == 0) {
    free(handle);
    handle = DART_HANDLE_NULL;
  }

  *handleptr = handle;

  DART_LOG_TRACE("dart_put_handle > handle(%p) dest:%d",
                 (void*)(handle), team_unit_id.id);

  return ret;
}

/* -- Blocking dart one-sided operations -- */

/**
 * \todo Check if MPI_Get_accumulate (MPI_NO_OP) yields better performance
 */
dart_ret_t dart_put_blocking(
  dart_gptr_t       gptr,
  const void      * src,
  size_t            nelem,
  dart_datatype_t   src_type,
  dart_datatype_t   dst_type)
{
  dart_team_unit_t  team_unit_id = DART_TEAM_UNIT_ID(gptr.unitid);
  uint64_t          offset       = gptr.addr_or_offs.offset;
  int16_t           seg_id       = gptr.segid;
  dart_team_t       teamid       = gptr.teamid;

  CHECK_EQUAL_BASETYPE(src_type, dst_type);

  dart_team_data_t *team_data = dart_adapt_teamlist_get(gptr.teamid);
  if (dart__unlikely(team_data == NULL)) {
    DART_LOG_ERROR("dart_put_blocking ! failed: Unknown team %i!", gptr.teamid);
    return DART_ERR_INVAL;
  }

  CHECK_UNITID_RANGE(team_unit_id, team_data);

  dart_segment_info_t *seginfo = dart_segment_get_info(
                                    &(team_data->segdata), seg_id);
  if (dart__unlikely(seginfo == NULL)) {
    DART_LOG_ERROR("dart_put_blocking ! "
                   "Unknown segment %i on team %i", seg_id, teamid);
    return DART_ERR_INVAL;
  }

  DART_LOG_DEBUG("dart_put_blocking() uid:%d o:%"PRIu64" s:%d t:%d, nelem:%zu",
                 team_unit_id.id, offset, seg_id, gptr.teamid, nelem);

  MPI_Win win  = seginfo->win;

  dart_ret_t ret = DART_OK;
  bool needs_flush = false;

  if (dart__mpi__datatype_iscontiguous(src_type) &&
      dart__mpi__datatype_iscontiguous(dst_type)) {
    // fast path for basic data types
    CHECK_EQUAL_BASETYPE(src_type, dst_type);
    ret = dart__mpi__put_basic(team_data, team_unit_id, seginfo, src,
                               offset, nelem, src_type,
                               NULL, NULL, &needs_flush);
  } else {
    // slow path for complex data types
    ret = dart__mpi__put_complex(team_unit_id, seginfo, src,
                                 offset, nelem, src_type, dst_type,
                                 NULL, NULL, &needs_flush);
  }

  if (ret == DART_OK && needs_flush) {
    DART_LOG_DEBUG("dart_put_blocking: MPI_Win_flush");
    CHECK_MPI_RET(MPI_Win_flush(team_unit_id.id, win), "MPI_Win_flush");
  }

  DART_LOG_DEBUG("dart_put_blocking > finished");
  return ret;
}

/**
 * \todo Check if MPI_Accumulate (REPLACE) yields better performance
 */
dart_ret_t dart_get_blocking(
  void            * dest,
  dart_gptr_t       gptr,
  size_t            nelem,
  dart_datatype_t   src_type,
  dart_datatype_t   dst_type)
{
  dart_team_unit_t  team_unit_id = DART_TEAM_UNIT_ID(gptr.unitid);
  uint64_t          offset       = gptr.addr_or_offs.offset;
  int16_t           seg_id       = gptr.segid;
  dart_team_t       teamid       = gptr.teamid;

  CHECK_EQUAL_BASETYPE(src_type, dst_type);

  dart_team_data_t *team_data = dart_adapt_teamlist_get(teamid);
  if (dart__unlikely(team_data == NULL)) {
    DART_LOG_ERROR("dart_get_blocking ! failed: Unknown team %i!", teamid);
    return DART_ERR_INVAL;
  }

  CHECK_UNITID_RANGE(team_unit_id, team_data);

  DART_LOG_DEBUG("dart_get_blocking() uid:%d "
                 "o:%"PRIu64" s:%d t:%u, nelem:%zu",
                 team_unit_id.id,
                 offset, seg_id, teamid, nelem);

  dart_segment_info_t *seginfo = dart_segment_get_info(
                                    &(team_data->segdata), seg_id);
  if (dart__unlikely(seginfo == NULL)) {
    DART_LOG_ERROR("dart_get_blocking ! "
                   "Unknown segment %i on team %i", seg_id, teamid);
    return DART_ERR_INVAL;
  }

  dart_ret_t ret = DART_OK;

  MPI_Request reqs[2]  = {MPI_REQUEST_NULL, MPI_REQUEST_NULL};
  uint8_t     num_reqs = 0;

  // leave complex data type handling to MPI
  if (dart__mpi__datatype_iscontiguous(src_type) &&
      dart__mpi__datatype_iscontiguous(dst_type)) {
    // fast-path for basic types
    CHECK_EQUAL_BASETYPE(src_type, dst_type);
    ret = dart__mpi__get_basic(team_data, team_unit_id, seginfo, dest,
                               offset, nelem, src_type,
                               reqs, &num_reqs);
  } else {
    // slow path for derived types
    ret = dart__mpi__get_complex(team_unit_id, seginfo, dest,
                                 offset, nelem, src_type, dst_type,
                                 reqs, &num_reqs);
  }

  if (ret == DART_OK && num_reqs > 0) {
    CHECK_MPI_RET(
      MPI_Waitall(num_reqs, reqs, MPI_STATUSES_IGNORE), "MPI_Waitall");
  }

  DART_LOG_DEBUG("dart_get_blocking > finished");
  return DART_OK;
}

/* -- Dart RMA Synchronization Operations -- */

dart_ret_t dart_flush(
  dart_gptr_t gptr)
{
  dart_team_unit_t team_unit_id = DART_TEAM_UNIT_ID(gptr.unitid);
  int16_t          seg_id       = gptr.segid;
  dart_team_t      teamid       = gptr.teamid;
  DART_LOG_DEBUG("dart_flush() gptr: "
                 "unitid:%d offset:%"PRIu64" segid:%d teamid:%d",
                 gptr.unitid, gptr.addr_or_offs.offset,
                 gptr.segid,  gptr.teamid);

  dart_team_data_t *team_data = dart_adapt_teamlist_get(teamid);
  if (dart__unlikely(team_data == NULL)) {
    DART_LOG_ERROR("dart_flush ! failed: Unknown team %i!", teamid);
    return DART_ERR_INVAL;
  }

  CHECK_UNITID_RANGE(team_unit_id, team_data);

  dart_segment_info_t *seginfo = dart_segment_get_info(
                                    &(team_data->segdata), seg_id);
  if (dart__unlikely(seginfo == NULL)) {
    DART_LOG_ERROR("dart_flush ! "
                   "Unknown segment %i on team %i", seg_id, teamid);
    return DART_ERR_INVAL;
  }

  MPI_Comm comm = team_data->comm;
  MPI_Win  win  = seginfo->win;

  DART_LOG_TRACE("dart_flush: MPI_Win_flush");
  CHECK_MPI_RET(
    MPI_Win_flush(team_unit_id.id, win), "MPI_Win_flush");
  DART_LOG_TRACE("dart_flush: MPI_Win_sync");
  CHECK_MPI_RET(
    MPI_Win_sync(win), "MPI_Win_sync");

  // trigger progress
  int flag;
  CHECK_MPI_RET(
    MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &flag, MPI_STATUS_IGNORE),
    "MPI_Iprobe");

  DART_LOG_DEBUG("dart_flush > finished");
  return DART_OK;
}

dart_ret_t dart_flush_all(
  dart_gptr_t gptr)
{
  int16_t     seg_id = gptr.segid;
  dart_team_t teamid = gptr.teamid;

  DART_LOG_DEBUG("dart_flush_all() gptr: "
                 "unitid:%d offset:%"PRIu64" segid:%d teamid:%d",
                 gptr.unitid, gptr.addr_or_offs.offset,
                 gptr.segid,  gptr.teamid);

  dart_team_data_t *team_data = dart_adapt_teamlist_get(teamid);
  if (dart__unlikely(team_data == NULL)) {
    DART_LOG_ERROR("dart_flush ! failed: Unknown team %i!", teamid);
    return DART_ERR_INVAL;
  }

  dart_segment_info_t *seginfo = dart_segment_get_info(
                                    &(team_data->segdata), seg_id);
  if (dart__unlikely(seginfo == NULL)) {
    DART_LOG_ERROR("dart_get_blocking ! "
                   "Unknown segment %i on team %i", seg_id, teamid);
    return DART_ERR_INVAL;
  }

  MPI_Comm comm = team_data->comm;
  MPI_Win  win  = seginfo->win;

  DART_LOG_TRACE("dart_flush_all: MPI_Win_flush_all");
  CHECK_MPI_RET(
    MPI_Win_flush_all(win), "MPI_Win_flush");
  DART_LOG_TRACE("dart_flush_all: MPI_Win_sync");
  CHECK_MPI_RET(
    MPI_Win_sync(win), "MPI_Win_sync");

  // trigger progress
  int flag;
  CHECK_MPI_RET(
    MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &flag, MPI_STATUS_IGNORE),
    "MPI_Iprobe");

  DART_LOG_DEBUG("dart_flush_all > finished");
  return DART_OK;
}

dart_ret_t dart_flush_local(
  dart_gptr_t gptr)
{
  int16_t     seg_id = gptr.segid;
  dart_team_t teamid = gptr.teamid;
  dart_team_unit_t team_unit_id = DART_TEAM_UNIT_ID(gptr.unitid);

  DART_LOG_DEBUG("dart_flush_local() gptr: "
                 "unitid:%d offset:%"PRIu64" segid:%d teamid:%d",
                 gptr.unitid, gptr.addr_or_offs.offset,
                 gptr.segid,  gptr.teamid);

  dart_team_data_t *team_data = dart_adapt_teamlist_get(teamid);
  if (dart__unlikely(team_data == NULL)) {
    DART_LOG_ERROR("dart_flush_local ! failed: Unknown team %i!", teamid);
    return DART_ERR_INVAL;
  }

  CHECK_UNITID_RANGE(team_unit_id, team_data);

  dart_segment_info_t *seginfo = dart_segment_get_info(
                                    &(team_data->segdata), seg_id);
  if (dart__unlikely(seginfo == NULL)) {
    DART_LOG_ERROR("dart_flush_local ! "
                   "Unknown segment %i on team %i", seg_id, teamid);
    return DART_ERR_INVAL;
  }
  MPI_Comm comm = team_data->comm;
  MPI_Win  win  = seginfo->win;

  DART_LOG_TRACE("dart_flush_local: MPI_Win_flush_local");
  CHECK_MPI_RET(
    MPI_Win_flush_local(team_unit_id.id, win),
    "MPI_Win_flush_local");

  // trigger progress
  int flag;
  CHECK_MPI_RET(
    MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &flag, MPI_STATUS_IGNORE),
    "MPI_Iprobe");

  DART_LOG_DEBUG("dart_flush_local > finished");
  return DART_OK;
}

dart_ret_t dart_flush_local_all(
  dart_gptr_t gptr)
{
  int16_t     seg_id = gptr.segid;
  dart_team_t teamid = gptr.teamid;
  DART_LOG_DEBUG("dart_flush_local_all() gptr: "
                 "unitid:%d offset:%"PRIu64" segid:%d teamid:%d",
                 gptr.unitid, gptr.addr_or_offs.offset,
                 gptr.segid,  gptr.teamid);

  dart_team_data_t *team_data = dart_adapt_teamlist_get(teamid);
  if (dart__unlikely(team_data == NULL)) {
    DART_LOG_ERROR("dart_flush ! failed: Unknown team %i!", teamid);
    return DART_ERR_INVAL;
  }

  dart_segment_info_t *seginfo = dart_segment_get_info(
                                    &(team_data->segdata), seg_id);
  if (dart__unlikely(seginfo == NULL)) {
    DART_LOG_ERROR("dart_get_blocking ! "
                   "Unknown segment %i on team %i", seg_id, teamid);
    return DART_ERR_INVAL;
  }

  MPI_Comm comm = team_data->comm;
  MPI_Win  win  = seginfo->win;

  CHECK_MPI_RET(
    MPI_Win_flush_local_all(win),
    "MPI_Win_flush_local_all");

  // trigger progress
  int flag;
  CHECK_MPI_RET(
    MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &flag, MPI_STATUS_IGNORE),
    "MPI_Iprobe");

  DART_LOG_DEBUG("dart_flush_local_all > finished");
  return DART_OK;
}

dart_ret_t dart_wait_local(
  dart_handle_t * handleptr)
{
  DART_LOG_DEBUG("dart_wait_local() handle:%p", (void*)(handleptr));
  if (handleptr != NULL && *handleptr != DART_HANDLE_NULL) {
    dart_handle_t handle = *handleptr;
    DART_LOG_TRACE("dart_wait_local:     handle: %p",
                   handle);
    DART_LOG_TRACE("dart_wait_local:     handle->dest: %d",
                   handle->dest);
    DART_LOG_TRACE("dart_wait_local:     handle->win:  %p",
                   (void*)(unsigned long)(handle->win));
    if (handle->num_reqs > 0) {
      DART_LOG_DEBUG("dart_wait_local:     -- MPI_Wait");
      int ret = MPI_Waitall(handle->num_reqs, handle->reqs, MPI_STATUS_IGNORE);
      if (ret != MPI_SUCCESS) {
        DART_LOG_DEBUG("dart_wait_local ! MPI_Wait failed");
        return DART_ERR_INVAL;
      }
    } else {
      DART_LOG_TRACE("dart_wait_local:     handle->num_reqs == 0");
    }
    free(handle);
    *handleptr = DART_HANDLE_NULL;
  }
  DART_LOG_DEBUG("dart_wait_local > finished");
  return DART_OK;
}

dart_ret_t dart_wait(
  dart_handle_t * handleptr)
{
  DART_LOG_DEBUG("dart_wait() handle:%p", (void*)(handleptr));
  if (handleptr != NULL && *handleptr != DART_HANDLE_NULL) {
    dart_handle_t handle = *handleptr;
    DART_LOG_TRACE("dart_wait:     handle: %p",
                   handle);
    DART_LOG_TRACE("dart_wait:     handle->dest: %d",
                   handle->dest);
    DART_LOG_TRACE("dart_wait:     handle->win:  %"PRIu64"",
                   (unsigned long)handle->win);
    if (handle->num_reqs > 0) {
      DART_LOG_DEBUG("dart_wait:     -- MPI_Wait");
      CHECK_MPI_RET(
        MPI_Waitall(handle->num_reqs, handle->reqs, MPI_STATUSES_IGNORE),
        "MPI_Waitall");

      if (handle->needs_flush) {
        DART_LOG_DEBUG("dart_wait:   -- MPI_Win_flush");
        CHECK_MPI_RET(MPI_Win_flush(handle->dest, handle->win),
                      "MPI_Win_flush");
      }
    } else {
      DART_LOG_TRACE("dart_wait:     handle->num_reqs == 0");
    }
    /* Free handle resource */
    free(handle);
    *handleptr = DART_HANDLE_NULL;
  }
  DART_LOG_DEBUG("dart_wait > finished");
  return DART_OK;
}

dart_ret_t dart_waitall_local(
  dart_handle_t handles[],
  size_t        num_handles)
{
  dart_ret_t ret = DART_OK;

  DART_LOG_DEBUG("dart_waitall_local()");
  if (num_handles == 0) {
    DART_LOG_DEBUG("dart_waitall_local > number of handles = 0");
    return DART_OK;
  }
  if (dart__unlikely(num_handles > INT_MAX)) {
    DART_LOG_ERROR("dart_waitall_local ! number of handles > INT_MAX");
    return DART_ERR_INVAL;
  }
  if (handles != NULL) {
    size_t r_n = 0;
    MPI_Request *mpi_req = ALLOC_TMP(2 * num_handles * sizeof(MPI_Request));
    for (size_t i = 0; i < num_handles; ++i) {
      if (handles[i] != DART_HANDLE_NULL) {
        for (uint8_t j = 0; j < handles[i]->num_reqs; ++j) {
          if (handles[i]->reqs[j] != MPI_REQUEST_NULL){
            DART_LOG_TRACE("dart_waitall_local: -- handle[%"PRIu64"]: %p)",
                          i, (void*)handles[i]->reqs[j]);
            DART_LOG_TRACE("dart_waitall_local:    handle[%"PRIu64"]->dest: %d",
                          i, handles[i]->dest);
            mpi_req[r_n] = handles[i]->reqs[j];
            r_n++;
          }
        }
      }
    }

    /*
     * Wait for local completion of MPI requests:
     */
    DART_LOG_DEBUG("dart_waitall_local: "
                   "MPI_Waitall, %"PRIu64" requests from %"PRIu64" handles",
                   r_n, num_handles);
    if (r_n > 0) {
      if (MPI_Waitall(r_n, mpi_req, MPI_STATUSES_IGNORE) != MPI_SUCCESS) {
        DART_LOG_ERROR("dart_waitall_local: MPI_Waitall failed");
        FREE_TMP(2 * num_handles * sizeof(MPI_Request), mpi_req);
        return DART_ERR_INVAL;
      }
    } else {
      DART_LOG_DEBUG("dart_waitall_local > number of requests = 0");
      FREE_TMP(2 * num_handles * sizeof(MPI_Request), mpi_req);
      return DART_OK;
    }

    /*
     * free DART handles
     */
    DART_LOG_TRACE("dart_waitall_local: releasing DART handles");
    for (size_t i = 0; i < num_handles; i++) {
      if (handles[i] != DART_HANDLE_NULL) {
        DART_LOG_TRACE("dart_waitall_local: free handle[%zu] %p",
                       i, (void*)(handles[i]));
        // free the handle
        free(handles[i]);
        handles[i] = DART_HANDLE_NULL;
      }
    }
    FREE_TMP(2 * num_handles * sizeof(MPI_Request), mpi_req);
  }
  DART_LOG_DEBUG("dart_waitall_local > %d", ret);
  return ret;
}

static
dart_ret_t wait_remote_completion(
  dart_handle_t *handles,
  size_t         n
)
{
  for (size_t i = 0; i < n; i++) {
    if (handles[i] != DART_HANDLE_NULL && handles[i]->needs_flush) {
      DART_LOG_DEBUG("dart_waitall: -- MPI_Win_flush(handle[%zu]: %p, dest: %d))",
                      i, (void*)handles[i], handles[i]->dest);
      /*
        * MPI_Win_flush to wait for remote completion if required:
        */
      if (MPI_Win_flush(handles[i]->dest, handles[i]->win) != MPI_SUCCESS) {
        return DART_ERR_INVAL;
      }
    }
  }
  return DART_OK;
}

dart_ret_t dart_waitall(
  dart_handle_t handles[],
  size_t        n)
{
  DART_LOG_DEBUG("dart_waitall()");
  if (n == 0) {
    DART_LOG_DEBUG("dart_waitall > number of handles = 0");
    return DART_OK;
  }

  if (dart__unlikely(n > INT_MAX)) {
    DART_LOG_ERROR("dart_waitall ! number of handles > INT_MAX");
    return DART_ERR_INVAL;
  }

  DART_LOG_DEBUG("dart_waitall: number of handles: %zu", n);

  if (handles != NULL) {
    MPI_Request *mpi_req = ALLOC_TMP(2 * n * sizeof(MPI_Request));
    /*
     * copy requests from DART handles to MPI request array:
     */
    DART_LOG_TRACE("dart_waitall: copying DART handles to MPI request array");
    size_t r_n = 0;
    for (size_t i = 0; i < n; i++) {
      if (handles[i] != DART_HANDLE_NULL) {
        for (uint8_t j = 0; j < handles[i]->num_reqs; ++j) {
          if (handles[i]->reqs[j] != MPI_REQUEST_NULL){
            DART_LOG_DEBUG("dart_waitall: -- handle[%zu](%p): "
                          "dest:%d win:%"PRIu64,
                          i, (void*)handles[i]->reqs[0],
                          handles[i]->dest,
                          (unsigned long)handles[i]->win);
            mpi_req[r_n] = handles[i]->reqs[j];
            r_n++;
          }
        }
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
      if (MPI_Waitall(r_n, mpi_req, MPI_STATUSES_IGNORE) != MPI_SUCCESS) {
        DART_LOG_ERROR("dart_waitall: MPI_Waitall failed");
        FREE_TMP(2 * n * sizeof(MPI_Request), mpi_req);
        return DART_ERR_INVAL;
      }
    } else {
      DART_LOG_DEBUG("dart_waitall > number of requests = 0");
      FREE_TMP(2 * n * sizeof(MPI_Request), mpi_req);
      return DART_OK;
    }

    /*
     * wait for completion of MPI requests at origins and targets:
     */
    DART_LOG_DEBUG("dart_waitall: waiting for remote completion");
    if (DART_OK != wait_remote_completion(handles, n)) {
      DART_LOG_ERROR("dart_waitall: MPI_Win_flush failed");
      FREE_TMP(2 * n * sizeof(MPI_Request), mpi_req);
      return DART_ERR_OTHER;
    }

    /*
     * free memory:
     */
    DART_LOG_DEBUG("dart_waitall: free handles");
    for (size_t i = 0; i < n; i++) {
      if (handles[i] != DART_HANDLE_NULL) {
        /* Free handle resource */
        DART_LOG_TRACE("dart_waitall: -- free handle[%zu]: %p",
                       i, (void*)(handles[i]));
        // free the handle
        free(handles[i]);
        handles[i] = DART_HANDLE_NULL;
      }
    }
    DART_LOG_TRACE("dart_waitall: free MPI_Request temporaries");
    FREE_TMP(2 * n * sizeof(MPI_Request), mpi_req);
  }
  DART_LOG_DEBUG("dart_waitall > finished");
  return DART_OK;
}

/**
 * Wrapper around MPI_Testall to account for broken MPICH implementation.
 * MPICH <= 3.2.1 and its derivatives seem to be affected
 */
inline static
int
dart__mpi__testall(int num_reqs, MPI_Request *reqs, int *flag_ptr)
{
#if defined(MPICH_NUMVERSION) && MPICH_NUMVERSION <= 30201300
  int flag_result = 1;
  for (int i = 0; i < num_reqs; ++i) {
    int flag;
    /*
     * if the test succeeds the request is set to MPI_REQUEST_NULL,
     * which can be safely passed to MPI_Test again.
     * Eventually we will have all requests tested succesfully.
     */
    int ret = MPI_Test(&reqs[i], &flag, MPI_STATUS_IGNORE);
    if (ret != MPI_SUCCESS) {
      return ret;
    }
    // one incomplete request will flip the flag to 0
    flag_result &= flag;
  }
  *flag_ptr = flag_result;
  // we checked all requests succesfully
  return MPI_SUCCESS;
#else
  return MPI_Testall(num_reqs, reqs,
                flag_ptr, MPI_STATUSES_IGNORE);
#endif //defined(MPICH_NUMVERSION) && MPICH_NUMVERSION <= 30201300
}

dart_ret_t dart_test_local(
  dart_handle_t * handleptr,
  int32_t       * is_finished)
{
  int flag;

  DART_LOG_DEBUG("dart_test_local()");
  if (handleptr == NULL ||
      *handleptr == DART_HANDLE_NULL ||
      (*handleptr)->num_reqs == 0) {
    *is_finished = 1;
    return DART_OK;
  }
  *is_finished = 0;

  dart_handle_t handle = *handleptr;
  CHECK_MPI_RET(
    dart__mpi__testall(handle->num_reqs, handle->reqs, &flag),
    "MPI_Testall");

  if (flag) {
    // deallocate handle
    free(handle);
    *handleptr = DART_HANDLE_NULL;
    *is_finished = 1;
  }
  DART_LOG_DEBUG("dart_test_local > finished");
  return DART_OK;
}


dart_ret_t dart_test(
  dart_handle_t * handleptr,
  int32_t       * is_finished)
{
  int flag;

  DART_LOG_DEBUG("dart_test()");
  if (handleptr == NULL ||
      *handleptr == DART_HANDLE_NULL ||
      (*handleptr)->num_reqs == 0) {
    *is_finished = 1;
    return DART_OK;
  }
  *is_finished = 0;

  dart_handle_t handle = *handleptr;
  CHECK_MPI_RET(
    dart__mpi__testall(handle->num_reqs, handle->reqs, &flag),
    "MPI_Testall");

  if (flag) {
    if (handle->needs_flush) {
      CHECK_MPI_RET(
        MPI_Win_flush(handle->dest, handle->win),
        "MPI_Win_flush"
      );
    }
    // deallocate handle
    free(handle);
    *handleptr = DART_HANDLE_NULL;
    *is_finished = 1;
  }
  DART_LOG_DEBUG("dart_test > finished");
  return DART_OK;
}

dart_ret_t dart_testall_local(
  dart_handle_t   handles[],
  size_t          n,
  int32_t       * is_finished)
{
  int flag;

  DART_LOG_DEBUG("dart_testall_local()");
  if (handles == NULL || n == 0) {
    DART_LOG_DEBUG("dart_testall_local: empty handles");
    *is_finished = 1;
    return DART_OK;
  }
  *is_finished = 0;

  MPI_Request *mpi_req = ALLOC_TMP(2 * n * sizeof (MPI_Request));
  size_t r_n = 0;
  for (size_t i = 0; i < n; ++i) {
    if (handles[i] != DART_HANDLE_NULL) {
      for (uint8_t j = 0; j < handles[i]->num_reqs; ++j) {
        if (handles[i]->reqs[j] != MPI_REQUEST_NULL){
          mpi_req[r_n] = handles[i]->reqs[j];
          ++r_n;
        }
      }
    }
  }

  if (r_n) {
    if (dart__mpi__testall(r_n, mpi_req, &flag) != MPI_SUCCESS){
      FREE_TMP(2 * n * sizeof(MPI_Request), mpi_req);
      DART_LOG_ERROR("dart_testall_local: MPI_Testall failed!");
      return DART_ERR_OTHER;
    }

    if (flag) {
      for (size_t i = 0; i < n; i++) {
        if (handles[i] != DART_HANDLE_NULL) {
          // free the handle
          free(handles[i]);
          handles[i] = DART_HANDLE_NULL;
        }
      }
      *is_finished = 1;
    }
  } else {
    *is_finished = 1;
  }
  FREE_TMP(2 * n * sizeof(MPI_Request), mpi_req);
  DART_LOG_DEBUG("dart_testall_local > finished");
  return DART_OK;
}


dart_ret_t dart_testall(
  dart_handle_t   handles[],
  size_t          n,
  int32_t       * is_finished)
{
  DART_LOG_DEBUG("dart_testall_local()");
  if (handles == NULL || n == 0) {
    DART_LOG_DEBUG("dart_testall_local: empty handles");
    return DART_OK;
  }

  MPI_Request *mpi_req = ALLOC_TMP(2 * n * sizeof (MPI_Request));
  size_t r_n = 0;
  for (size_t i = 0; i < n; ++i) {
    if (handles[i] != DART_HANDLE_NULL) {
      for (uint8_t j = 0; j < handles[i]->num_reqs; ++j) {
        if (handles[i]->reqs[j] != MPI_REQUEST_NULL){
          mpi_req[r_n] = handles[i]->reqs[j];
          ++r_n;
        }
      }
    }
  }

  if (r_n) {
    DART_LOG_TRACE("  MPI_Testall on %zu requests", r_n);
    if (dart__mpi__testall(r_n, mpi_req, is_finished) != MPI_SUCCESS){
      DART_LOG_ERROR("dart_testall: MPI_Testall failed");
      FREE_TMP(2 * n * sizeof(MPI_Request), mpi_req);
      return DART_ERR_OTHER;
    }

    if (*is_finished) {
      /*
      * wait for completion of MPI requests at origins and targets:
      */
      DART_LOG_DEBUG("dart_testall: waiting for remote completion");
      if (DART_OK != wait_remote_completion(handles, n)) {
        DART_LOG_ERROR("dart_testall: MPI_Win_flush failed");
        FREE_TMP(2 * n * sizeof(MPI_Request), mpi_req);
        return DART_ERR_OTHER;
      }

      for (size_t i = 0; i < n; i++) {
        if (handles[i] != DART_HANDLE_NULL) {
          // free the handle
          free(handles[i]);
          handles[i] = DART_HANDLE_NULL;
        }
      }
    }
  } else {
    *is_finished = 1;
  }
  FREE_TMP(2 * n * sizeof(MPI_Request), mpi_req);
  DART_LOG_DEBUG("dart_testall_local > finished");
  return DART_OK;
}

dart_ret_t dart_handle_free(
  dart_handle_t * handleptr)
{
  if (handleptr != NULL && *handleptr != DART_HANDLE_NULL) {
    free(*handleptr);
    *handleptr = DART_HANDLE_NULL;
  }
  return DART_OK;
}

/* -- Dart collective operations -- */

static int _dart_barrier_count = 0;

dart_ret_t dart_barrier(
  dart_team_t teamid)
{
  DART_LOG_DEBUG("dart_barrier() barrier count: %d", _dart_barrier_count);

  if (dart__unlikely(teamid == DART_UNDEFINED_TEAM_ID)) {
    DART_LOG_ERROR("dart_barrier ! failed: team may not be DART_UNDEFINED_TEAM_ID");
    return DART_ERR_INVAL;
  }

  _dart_barrier_count++;

  dart_team_data_t *team_data = dart_adapt_teamlist_get(teamid);
  if (dart__unlikely(team_data == NULL)) {
    DART_LOG_ERROR("dart_barrier ! failed: Unknown team: %d", teamid);
    return DART_ERR_INVAL;
  }

  /* Fetch proper communicator from teams. */
  CHECK_MPI_RET(
    MPI_Barrier(team_data->comm), "MPI_Barrier");

  DART_LOG_DEBUG("dart_barrier > MPI_Barrier finished");
  return DART_OK;
}

dart_ret_t dart_bcast(
  void              * buf,
  size_t              nelem,
  dart_datatype_t     dtype,
  dart_team_unit_t    root,
  dart_team_t         teamid)
{
  DART_LOG_TRACE("dart_bcast() root:%d team:%d nelem:%"PRIu64"",
                 root.id, teamid, nelem);

  dart_team_data_t *team_data = dart_adapt_teamlist_get(teamid);
  if (dart__unlikely(team_data == NULL)) {
    DART_LOG_ERROR("dart_bcast ! failed: unknown team %d", teamid);
    return DART_ERR_INVAL;
  }

  CHECK_UNITID_RANGE(root, team_data);

  MPI_Comm comm = team_data->comm;

  // chunk up the bcast if necessary
  const size_t nchunks   = nelem / MAX_CONTIG_ELEMENTS;
  const size_t remainder = nelem % MAX_CONTIG_ELEMENTS;
        char * src_ptr   = (char*) buf;

  if (nchunks > 0) {
    CHECK_MPI_RET(
      MPI_Bcast(src_ptr, nchunks,
                dart__mpi__datatype_maxtype(dtype),
                root.id, comm),
      "MPI_Bcast");
    src_ptr += nchunks * MAX_CONTIG_ELEMENTS;
  }

  if (remainder > 0) {
    MPI_Datatype mpi_dtype = dart__mpi__datatype_struct(dtype)->contiguous.mpi_type;
    CHECK_MPI_RET(
      MPI_Bcast(src_ptr, remainder, mpi_dtype, root.id, comm),
      "MPI_Bcast");
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
  CHECK_IS_CONTIGUOUSTYPE(dtype);

  dart_team_data_t *team_data = dart_adapt_teamlist_get(teamid);
  if (dart__unlikely(team_data == NULL)) {
    DART_LOG_ERROR("dart_scatter ! failed: unknown team %d", teamid);
    return DART_ERR_INVAL;
  }

  CHECK_UNITID_RANGE(root, team_data);

  // chunk up the scatter if necessary
  const size_t nchunks   = nelem / MAX_CONTIG_ELEMENTS;
  const size_t remainder = nelem % MAX_CONTIG_ELEMENTS;
  const char * send_ptr  = (const char*) sendbuf;
        char * recv_ptr  = (char*) recvbuf;

  MPI_Comm comm = team_data->comm;

  if (nchunks > 0) {
    MPI_Datatype mpi_dtype = dart__mpi__datatype_maxtype(dtype);
    CHECK_MPI_RET(
      MPI_Scatter(
          send_ptr,
          nchunks,
          mpi_dtype,
          recv_ptr,
          nchunks,
          mpi_dtype,
          root.id,
          comm),
      "MPI_Scatter");
    send_ptr += nchunks * MAX_CONTIG_ELEMENTS;
    recv_ptr += nchunks * MAX_CONTIG_ELEMENTS;
  }

  if (remainder > 0) {
    MPI_Datatype mpi_dtype = dart__mpi__datatype_struct(dtype)->contiguous.mpi_type;
    CHECK_MPI_RET(
      MPI_Scatter(
          send_ptr,
          remainder,
          mpi_dtype,
          recv_ptr,
          remainder,
          mpi_dtype,
          root.id,
          comm),
      "MPI_Scatter");
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
  DART_LOG_TRACE("dart_gather() team:%d nelem:%"PRIu64"",
                 teamid, nelem);

  CHECK_IS_CONTIGUOUSTYPE(dtype);

  dart_team_data_t *team_data = dart_adapt_teamlist_get(teamid);
  if (dart__unlikely(team_data == NULL)) {
    DART_LOG_ERROR("dart_gather ! failed: unknown teamid %d", teamid);
    return DART_ERR_INVAL;
  }

  CHECK_UNITID_RANGE(root, team_data);

  // chunk up the scatter if necessary
  const size_t nchunks   = nelem / MAX_CONTIG_ELEMENTS;
  const size_t remainder = nelem % MAX_CONTIG_ELEMENTS;
  const char * send_ptr  = (const char*) sendbuf;
        char * recv_ptr  = (char*) recvbuf;

  MPI_Comm comm = team_data->comm;

  if (nchunks > 0) {
    MPI_Datatype mpi_dtype = dart__mpi__datatype_maxtype(dtype);
    CHECK_MPI_RET(
      MPI_Gather(
          send_ptr,
          nchunks,
          mpi_dtype,
          recv_ptr,
          nchunks,
          mpi_dtype,
          root.id,
          comm),
      "MPI_Gather");
    send_ptr += nchunks * MAX_CONTIG_ELEMENTS;
    recv_ptr += nchunks * MAX_CONTIG_ELEMENTS;
  }

  if (remainder > 0) {
    MPI_Datatype mpi_dtype = dart__mpi__datatype_struct(dtype)->contiguous.mpi_type;
    CHECK_MPI_RET(
      MPI_Gather(
          send_ptr,
          remainder,
          mpi_dtype,
          recv_ptr,
          remainder,
          mpi_dtype,
          root.id,
          comm),
      "MPI_Gather");
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
  DART_LOG_TRACE("dart_allgather() team:%d nelem:%"PRIu64"",
                 teamid, nelem);

  CHECK_IS_CONTIGUOUSTYPE(dtype);

  dart_team_data_t *team_data = dart_adapt_teamlist_get(teamid);
  if (dart__unlikely(team_data == NULL)) {
    DART_LOG_ERROR("dart_allgather ! unknown teamid %d", teamid);
    return DART_ERR_INVAL;
  }

  if (sendbuf == recvbuf || NULL == sendbuf) {
    sendbuf = MPI_IN_PLACE;
  }

  // chunk up the scatter if necessary
  const size_t nchunks   = nelem / MAX_CONTIG_ELEMENTS;
  const size_t remainder = nelem % MAX_CONTIG_ELEMENTS;
  const char * send_ptr  = (const char*) sendbuf;
        char * recv_ptr  = (char*) recvbuf;

  MPI_Comm comm = team_data->comm;

  if (nchunks > 0) {
    MPI_Datatype mpi_dtype = dart__mpi__datatype_maxtype(dtype);
    CHECK_MPI_RET(
      MPI_Allgather(
          send_ptr,
          nchunks,
          mpi_dtype,
          recv_ptr,
          nchunks,
          mpi_dtype,
          comm),
      "MPI_Allgather");
    send_ptr += nchunks * MAX_CONTIG_ELEMENTS;
    recv_ptr += nchunks * MAX_CONTIG_ELEMENTS;
  }

  if (remainder > 0) {
    MPI_Datatype mpi_dtype = dart__mpi__datatype_struct(dtype)->contiguous.mpi_type;
    CHECK_MPI_RET(
      MPI_Allgather(
          send_ptr,
          remainder,
          mpi_dtype,
          recv_ptr,
          remainder,
          mpi_dtype,
          comm),
      "MPI_Allgather");
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
  DART_LOG_TRACE("dart_allgatherv() team:%d nsendelem:%"PRIu64"",
                 teamid, nsendelem);

  CHECK_IS_CONTIGUOUSTYPE(dtype);

  /*
   * MPI uses offset type int, do not copy more than INT_MAX elements:
   */
  if (dart__unlikely(nsendelem > MAX_CONTIG_ELEMENTS)) {
    DART_LOG_ERROR("dart_allgather ! failed: nsendelem (%zu) > INT_MAX",
                   nsendelem);
    return DART_ERR_INVAL;
  }

  dart_team_data_t *team_data = dart_adapt_teamlist_get(teamid);
  if (dart__unlikely(team_data == NULL)) {
    DART_LOG_ERROR("dart_allgatherv ! unknown teamid %d", teamid);
    return DART_ERR_INVAL;
  }
  if (sendbuf == recvbuf || NULL == sendbuf) {
    sendbuf = MPI_IN_PLACE;
  }
  MPI_Comm comm      = team_data->comm;
  int      comm_size = team_data->size;

  // convert nrecvcounts and recvdispls
  MPI_Comm_size(comm, &comm_size);
  int *inrecvcounts = malloc(sizeof(int) * comm_size);
  int *irecvdispls  = malloc(sizeof(int) * comm_size);
  for (int i = 0; i < comm_size; i++) {
    if (nrecvcounts[i] > MAX_CONTIG_ELEMENTS ||
        recvdispls[i] > MAX_CONTIG_ELEMENTS)
    {
      DART_LOG_ERROR(
        "dart_allgatherv ! failed: nrecvcounts[%i] (%zu) > INT_MAX || "
        "recvdispls[%i] (%zu) > INT_MAX", i, nrecvcounts[i], i, recvdispls[i]);
      free(inrecvcounts);
      free(irecvdispls);
      return DART_ERR_INVAL;
    }
    inrecvcounts[i] = nrecvcounts[i];
    irecvdispls[i]  = recvdispls[i];
  }

  MPI_Datatype mpi_dtype = dart__mpi__datatype_struct(dtype)->contiguous.mpi_type;
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

  CHECK_IS_CONTIGUOUSTYPE(dtype);

  MPI_Op       mpi_op    = dart__mpi__op(op, dtype);
  MPI_Datatype mpi_dtype = dart__mpi__op_type(op, dtype);

  /*
   * MPI uses offset type int, do not copy more than INT_MAX elements:
   */
  if (dart__unlikely(nelem > MAX_CONTIG_ELEMENTS)) {
    DART_LOG_ERROR("dart_allreduce ! failed: nelem (%zu) > INT_MAX", nelem);
    return DART_ERR_INVAL;
  }

  dart_team_data_t *team_data = dart_adapt_teamlist_get(team);
  if (dart__unlikely(team_data == NULL)) {
    DART_LOG_ERROR("dart_allreduce ! unknown teamid %d", team);
    return DART_ERR_INVAL;
  }
  MPI_Comm comm = team_data->comm;
  CHECK_MPI_RET(
    MPI_Allreduce(
           sendbuf,   // send buffer
           recvbuf,   // receive buffer
           nelem,     // buffer size
           mpi_dtype, // datatype
           mpi_op,    // reduce operation
           comm),
    "MPI_Allreduce");
  return DART_OK;
}

dart_ret_t dart_alltoall(
    const void *    sendbuf,
    void *          recvbuf,
    size_t          nelem,
    dart_datatype_t dtype,
    dart_team_t     teamid)
{
  DART_LOG_TRACE("dart_alltoall() team:%d nelem:%" PRIu64 "", teamid, nelem);

  CHECK_IS_BASICTYPE(dtype);

  /*
   * MPI uses offset type int, do not copy more than INT_MAX elements:
   */
  if (dart__unlikely(nelem > MAX_CONTIG_ELEMENTS)) {
    DART_LOG_ERROR("dart_alltoall ! failed: nelem (%zu) > INT_MAX", nelem);
    return DART_ERR_INVAL;
  }

  dart_team_data_t *team_data = dart_adapt_teamlist_get(teamid);
  if (dart__unlikely(team_data == NULL)) {
    DART_LOG_ERROR("dart_alltoall ! unknown teamid %d", teamid);
    return DART_ERR_INVAL;
  }

  if (sendbuf == recvbuf || NULL == sendbuf) {
    sendbuf = MPI_IN_PLACE;
  }

  MPI_Comm comm = team_data->comm;

  MPI_Datatype mpi_dtype = dart__mpi__datatype_struct(dtype)->contiguous.mpi_type;

  CHECK_MPI_RET(
      MPI_Alltoall(
          sendbuf,
          nelem,
          mpi_dtype,
          recvbuf,
          nelem,
          mpi_dtype,
          comm),
      "MPI_Alltoall");

  DART_LOG_TRACE("dart_alltoall > team:%d nelem:%" PRIu64 "", teamid, nelem);
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
  CHECK_IS_CONTIGUOUSTYPE(dtype);
  MPI_Op       mpi_op    = dart__mpi__op(op, dtype);
  MPI_Datatype mpi_dtype = dart__mpi__op_type(op, dtype);
  /*
   * MPI uses offset type int, do not copy more than INT_MAX elements:
   */
  if (dart__unlikely(nelem > MAX_CONTIG_ELEMENTS)) {
    DART_LOG_ERROR("dart_allreduce ! failed: nelem (%zu) > INT_MAX", nelem);
    return DART_ERR_INVAL;
  }

  dart_team_data_t *team_data = dart_adapt_teamlist_get(team);
  if (dart__unlikely(team_data == NULL)) {
    DART_LOG_ERROR("dart_reduce ! unknown teamid %d", team);
    return DART_ERR_INVAL;
  }

  CHECK_UNITID_RANGE(root, team_data);

  comm = team_data->comm;
  CHECK_MPI_RET(
    MPI_Reduce(
           sendbuf,
           recvbuf,
           nelem,
           mpi_dtype,
           mpi_op,
           root.id,
           comm),
    "MPI_Reduce");
  return DART_OK;
}

dart_ret_t dart_send(
  const void         * sendbuf,
  size_t               nelem,
  dart_datatype_t      dtype,
  int                  tag,
  dart_global_unit_t   unit)
{
  MPI_Comm comm;
  CHECK_IS_CONTIGUOUSTYPE(dtype);
  MPI_Datatype mpi_dtype = dart__mpi__datatype_struct(dtype)->contiguous.mpi_type;
  dart_team_t team = DART_TEAM_ALL;

  /*
   * MPI uses offset type int, do not copy more than INT_MAX elements:
   */
  if (dart__unlikely(nelem > MAX_CONTIG_ELEMENTS)) {
    DART_LOG_ERROR("dart_send ! failed: nelem (%zu) > INT_MAX", nelem);
    return DART_ERR_INVAL;
  }

  dart_team_data_t *team_data = dart_adapt_teamlist_get(team);
  if (dart__unlikely(team_data == NULL)) {
    DART_LOG_ERROR("dart_send ! unknown teamid %d", team);
    return DART_ERR_INVAL;
  }

  CHECK_UNITID_RANGE(unit, team_data);

  comm = team_data->comm;
  // dart_unit = MPI rank in comm_world
  CHECK_MPI_RET(
    MPI_Send(
        sendbuf,
        nelem,
        mpi_dtype,
        unit.id,
        tag,
        comm),
    "MPI_Send");
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
  CHECK_IS_CONTIGUOUSTYPE(dtype);
  MPI_Datatype mpi_dtype = dart__mpi__datatype_struct(dtype)->contiguous.mpi_type;
  dart_team_t team = DART_TEAM_ALL;

  /*
   * MPI uses offset type int, do not copy more than INT_MAX elements:
   */
  if (dart__unlikely(nelem > MAX_CONTIG_ELEMENTS)) {
    DART_LOG_ERROR("dart_recv ! failed: nelem (%zu) > INT_MAX", nelem);
    return DART_ERR_INVAL;
  }

  dart_team_data_t *team_data = dart_adapt_teamlist_get(team);
  if (dart__unlikely(team_data == NULL)) {
    DART_LOG_ERROR("dart_recv ! unknown teamid %d", team);
    return DART_ERR_INVAL;
  }

  CHECK_UNITID_RANGE(unit, team_data);

  comm = team_data->comm;
  // dart_unit = MPI rank in comm_world
  CHECK_MPI_RET(
    MPI_Recv(
        recvbuf,
        nelem,
        mpi_dtype,
        unit.id,
        tag,
        comm,
        MPI_STATUS_IGNORE),
    "MPI_Recv");
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
  CHECK_IS_CONTIGUOUSTYPE(send_dtype);
  CHECK_IS_CONTIGUOUSTYPE(recv_dtype);
  MPI_Datatype mpi_send_dtype =
                    dart__mpi__datatype_struct(send_dtype)->contiguous.mpi_type;
  MPI_Datatype mpi_recv_dtype =
                    dart__mpi__datatype_struct(recv_dtype)->contiguous.mpi_type;
  dart_team_t team = DART_TEAM_ALL;

  /*
   * MPI uses offset type int, do not copy more than INT_MAX elements:
   */
  if (dart__unlikely(
        send_nelem > MAX_CONTIG_ELEMENTS || recv_nelem > MAX_CONTIG_ELEMENTS)) {
    DART_LOG_ERROR("dart_sendrecv ! failed: nelem (%zu, %zu) > INT_MAX",
                   recv_nelem, send_nelem);
    return DART_ERR_INVAL;
  }

  dart_team_data_t *team_data = dart_adapt_teamlist_get(team);
  if (dart__unlikely(team_data == NULL)) {
    DART_LOG_ERROR("dart_sendrecv ! unknown teamid %d", team);
    return DART_ERR_INVAL;
  }

  CHECK_UNITID_RANGE(dest, team_data);
  CHECK_UNITID_RANGE(src, team_data);

  comm = team_data->comm;
  CHECK_MPI_RET(
    MPI_Sendrecv(
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
        MPI_STATUS_IGNORE),
    "MPI_Sendrecv");
  return DART_OK;
}
