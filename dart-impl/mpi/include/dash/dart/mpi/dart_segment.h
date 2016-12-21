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

typedef struct
{
  dart_segid_t seg_id; /* seg_id determines a global pointer uniquely */
  size_t       size;
  MPI_Aint   * disp;   /* address set of memory location of all units in certain team. */
  char      ** baseptr;
  char       * selfbaseptr;
  MPI_Win      win;
} dart_segment_info_t;


/**
 * @brief Initialize the segment data hash table.
 */
dart_ret_t dart_segment_init();

/**
 * @brief Allocates a new segment data struct. May be served from a freelist.
 */
dart_ret_t dart_segment_alloc(dart_segid_t segid, uint16_t team_idx);

/**
 * @brief Returns the registered team index for the segment ID.
 */
dart_ret_t dart_segment_get_teamidx(dart_segid_t segid, uint16_t *team_idx);

/**
 * @brief Add segment information to the segment hash table.
 */
dart_ret_t dart_segment_add_info(const dart_segment_info_t *item);

#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
/** @brief Query the shared memory window object associated with the specified seg_id.
 *
 *  @param[in] seg_id
 *  @param[out] win A MPI window object.
 *
 *  @retval non-negative integer Search successfully.
 *  @retval negative integer Failure.
 */
dart_ret_t dart_segment_get_win(int16_t seg_id, MPI_Win * win);

dart_ret_t dart_segment_get_baseptr(
  int16_t                seg_id,
  dart_team_unit_t       rel_unitid,
  char               **  baseptr_s);
#endif

dart_ret_t dart_segment_get_selfbaseptr(
  int16_t    seg_id,
  char   **  baseptr);

/** @brief Query the address of the memory location of the specified rel_unit in specified team.
 *
 *  The output disp_s information targets for the dart inter-node communication, which means
 *  the one-sided communication proceed within the dynamic window object instead of the
 *  shared memory window object.
 *
 *  @retval ditto
 */
dart_ret_t dart_segment_get_disp(int16_t             seg_id,
                                 dart_team_unit_t    rel_unitid,
                                 MPI_Aint          * disp_s);

/** @brief Query the length of the global memory block indicated by the specified seg_id.
 *
 *  @retval ditto
 */
dart_ret_t dart_segment_get_size(
  int16_t   seg_id,
  size_t  * size);

/**
 * @brief Deallocates the segment identified by the segment ID.
 */
dart_ret_t dart_segment_free(dart_segid_t segid);


/**
 * @brief Clear the segment data hash table.
 */
dart_ret_t dart_segment_fini();


#endif /* DART_SEGMENT_H_ */
