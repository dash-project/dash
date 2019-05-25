/*
 * dart_segment.h
 *
 *  Created on: Aug 12, 2016
 *      Author: joseph
 */

#ifndef DART_SEGMENT_H_
#define DART_SEGMENT_H_
#include <mpi.h>
#include <stdbool.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/base/macro.h>

typedef int16_t dart_segid_t;

#define DART_SEGMENT_HASH_SIZE 256

typedef struct
{
  size_t       size;
  MPI_Aint   * disp;        /* offsets at all units in the team */
  char      ** baseptr;     /* baseptr of all units in the sharedmem group */
  char       * selfbaseptr; /* baseptr of the current unit */
  MPI_Win      shmwin;      /* sharedmem window */
  MPI_Win      win;         /* window used to access this segment */
  uint16_t     flags;       /* 16 bit flags */
  dart_segid_t segid;       /* ID of the segment, globally unique in a team */
  bool         is_dynamic;  /* whether this is a shared memory segment */
  bool         sync_needed; /* whether a call to MPI_WIN_SYNC is needed */
} dart_segment_info_t;

// forward declaration to make the compiler happy
typedef struct dart_seghash_elem dart_seghash_elem_t;

typedef struct {
  dart_seghash_elem_t * hashtab[DART_SEGMENT_HASH_SIZE];
  dart_team_t           team_id;
  dart_seghash_elem_t * mem_freelist;
  dart_seghash_elem_t * reg_freelist;

  /**
   * For DART collective allocation/free: offset in the returned gptr
   * represents the displacement relative to the beginning of sub-memory
   * spanned by a DART collective allocation.
   * For DART local allocation/free: offset in the returned gptr represents
   * the displacement relative to the base address of memory region reserved
   * for the dart local allocation/free (see dart_buddy_allocator).
   * Local allocations are identified by Segment ID DART_SEGMENT_LOCAL.
   */
  int16_t memid;
  int16_t registermemid;
} dart_segmentdata_t;

typedef enum {
  DART_SEGMENT_LOCAL_ALLOC,
  DART_SEGMENT_ALLOC,
  DART_SEGMENT_REGISTER
} dart_segment_type;


/**
 * Initialize the segment data hash table.
 */
dart_ret_t dart_segment_init(
  dart_segmentdata_t *segdata,
  dart_team_t teamid) DART_INTERNAL;

/**
 * Allocates a new segment data struct. May be served from a freelist.
 * The call also allocates the correct segment ID based on the \c type
 * and registers the newly allocated segment in the segment data.
 *
 * \param segdata The segment data to of the team allocating this segment.
 * \param type    Whether the segment is allocated or registered.
 */
dart_segment_info_t *
dart_segment_alloc(
  dart_segmentdata_t *segdata,
  dart_segment_type type) DART_INTERNAL;

dart_ret_t
dart_segment_register(
  dart_segmentdata_t  *segdata,
  dart_segment_info_t *seg) DART_INTERNAL;

/**
 * Returns the segment info for the segment with ID \c segid.
 */
dart_segment_info_t * dart_segment_get_info(
  dart_segmentdata_t *segdata,
  dart_segid_t        segid) DART_INTERNAL;

/**
 * Returns the segment's displacement at unit \c team_unit_id.
 */
static inline
MPI_Aint
dart_segment_disp(
  const dart_segment_info_t *seginfo,
  dart_team_unit_t           team_unit_id)
{
  return (seginfo->disp != NULL) ? seginfo->disp[team_unit_id.id] : 0;
}


#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)

/**
 * Query the shared memory window object associated with the specified seg_id.
 *
 * \param[in] seg_id
 * \param[out] win A MPI window object.
 *
 * \retval non-negative integer Search successfully.
 * \retval negative integer Failure.
 */
dart_ret_t dart_segment_get_shmwin(
  dart_segmentdata_t * segdata,
  int16_t              seg_id,
  MPI_Win            * win) DART_INTERNAL;

dart_ret_t dart_segment_get_baseptr(
  dart_segmentdata_t   * segdata,
  int16_t                seg_id,
  dart_team_unit_t       rel_unitid,
  char               **  baseptr_s) DART_INTERNAL;
#endif

dart_ret_t dart_segment_get_selfbaseptr(
  dart_segmentdata_t * segdata,
  int16_t              seg_id,
  char              ** baseptr) DART_INTERNAL;

/**
 * Query the address of the memory location of the specified rel_unit in
 * specified team.
 *
 * The output disp_s information targets for the dart inter-node communication,
 * which means the one-sided communication proceed within the dynamic window
 * object instead of the shared memory window object.
 *
 * \retval ditto
 */
dart_ret_t dart_segment_get_disp(
  dart_segmentdata_t * segdata,
  int16_t              seg_id,
  dart_team_unit_t     rel_unitid,
  MPI_Aint           * disp_s) DART_INTERNAL;

/**
 * Query the length of the global memory block indicated by the
 * specified seg_id.
 *
 * \retval ditto
 */
dart_ret_t dart_segment_get_size(
  dart_segmentdata_t * segdata,
  int16_t              seg_id,
  size_t             * size) DART_INTERNAL;

dart_ret_t dart_segment_get_flags(
  dart_segmentdata_t * segdata,
  int16_t              seg_id,
  uint16_t           * flags) DART_INTERNAL;

dart_ret_t dart_segment_set_flags(
  dart_segmentdata_t * segdata,
  int16_t              seg_id,
  uint16_t             flags) DART_INTERNAL;

/**
 * Deallocates the segment identified by the segment ID.
 */
dart_ret_t dart_segment_free(
  dart_segmentdata_t * segdata,
  dart_segid_t         segid) DART_INTERNAL;


/**
 * Clear the segment data hash table.
 */
dart_ret_t dart_segment_fini(dart_segmentdata_t *segdata) DART_INTERNAL;


#endif /* DART_SEGMENT_H_ */
