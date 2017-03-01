/*
 * dart_segment.h
 *
 *  Created on: Aug 12, 2016
 *      Author: joseph
 */

#ifndef DART_SEGMENT_H_
#define DART_SEGMENT_H_

#include <dash/dart/if/dart_types.h>
#include <mpi.h>

typedef int16_t dart_segid_t;

#define DART_SEGMENT_HASH_SIZE 256

typedef struct
{
  dart_segid_t seg_id; /* seg_id determines a global pointer uniquely */
  size_t       size;
  MPI_Aint   * disp;   /* address set of memory location of all units in certain team. */
  char      ** baseptr;
  char       * selfbaseptr;
  MPI_Win      win;
  uint16_t     flags;
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
  DART_SEGMENT_ALLOC,
  DART_SEGMENT_REGISTER
} dart_segment_type;


/**
 * Initialize the segment data hash table.
 */
dart_ret_t dart_segment_init(dart_segmentdata_t *segdata, dart_team_t teamid);

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
  dart_segment_type type);

dart_ret_t
dart_segment_register(
  dart_segmentdata_t  *segdata,
  dart_segment_info_t *seg);


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
dart_ret_t dart_segment_get_win(dart_segmentdata_t *segdata, int16_t seg_id, MPI_Win * win);

dart_ret_t dart_segment_get_baseptr(
  dart_segmentdata_t   * segdata,
  int16_t                seg_id,
  dart_team_unit_t       rel_unitid,
  char               **  baseptr_s);
#endif

dart_ret_t dart_segment_get_selfbaseptr(
  dart_segmentdata_t * segdata,
  int16_t              seg_id,
  char              ** baseptr);

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
dart_ret_t dart_segment_get_disp(dart_segmentdata_t * segdata,
                                 int16_t              seg_id,
                                 dart_team_unit_t     rel_unitid,
                                 MPI_Aint           * disp_s);

/**
 * Query the length of the global memory block indicated by the
 * specified seg_id.
 *
 * \retval ditto
 */
dart_ret_t dart_segment_get_size(
  dart_segmentdata_t * segdata,
  int16_t              seg_id,
  size_t             * size);

dart_ret_t dart_segment_get_flags(
  dart_segmentdata_t * segdata,
  int16_t              seg_id,
  uint16_t           * flags);

dart_ret_t dart_segment_set_flags(
  dart_segmentdata_t * segdata,
  int16_t              seg_id,
  uint16_t             flags);

/**
 * Deallocates the segment identified by the segment ID.
 */
dart_ret_t dart_segment_free(dart_segmentdata_t * segdata,
                             dart_segid_t         segid);


/**
 * Clear the segment data hash table.
 */
dart_ret_t dart_segment_fini(dart_segmentdata_t *segdata);


#endif /* DART_SEGMENT_H_ */
