/*
 * dart_segment.h
 *
 *  Created on: Aug 12, 2016
 *      Author: joseph
 */

#ifndef DART_SEGMENT_H_
#define DART_SEGMENT_H_

#include <dash/dart/if/dart_types.h>

typedef int16_t dart_segid_t;

/**
 * @brief A data structure holding all required data for a segment.
 */

typedef struct dart_segment_data {

  /**
   * @brief The segment ID
   */
  dart_segid_t segid;

  /**
   * @brief The index of the team in the active team array.
   * Note that we implicitly assume that all segments are closed before a team is closed,
   * thus avoiding cases where the team_id points to an invalid team.
   * TODO: Is this a valid assumption?
   */
  int16_t team_idx;

  /* TODO: Add data fields as needed */

} dart_segment_t;

/**
 * @brief Initialize the segment data hash table.
 */
void dart_segment_init();

/**
 * @brief Allocates a new segment data struct. May be served from a freelist.
 */
dart_segment_t * dart_segment_alloc(dart_segid_t segid);

/**
 * @brief Returns the registered segment data for the segment ID.
 */
dart_segment_t * dart_segment_get(dart_segid_t segid);

/**
 * @brief Deallocates the segment identified by the segment ID.
 */
int dart_segment_dealloc(dart_segid_t segid);


/**
 * @brief Clear the segment data hash table.
 */
void dart_segment_clear();


#endif /* DART_SEGMENT_H_ */
