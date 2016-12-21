#include <string.h>
#include <dash/dart/mpi/dart_segment.h>
#include <dash/dart/base/logging.h>
#include <stdlib.h>
#include <inttypes.h>

#define DART_SEGMENT_HASH_SIZE 256
#define DART_SEGMENT_INVALID   (INT32_MAX)

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
   *
   * @note We implicitly assume that all segments are closed before a
   * team is closed,
   * thus avoiding cases where the team_id points to an invalid team.
   *
   * TODO: Is this a valid assumption?
   *
   * <fuchsto>: It might not, see dash::Team and dash::Array, especially
   *            register/unregister deallocators.
   */
  uint16_t team_idx;

  /**
   * @brief Data structure containing all relevant data for a segment.
   * To be queried using dart_adapt_transtable_* functions.
   */
  dart_segment_info_t seg_info;

  /* TODO: Add data fields as needed */

} dart_segment_t;

// forward declaration to make the compiler happy
typedef struct dart_seghash_elem dart_seghash_elem_t;

struct dart_seghash_elem {
  dart_seghash_elem_t *next;
  dart_segment_t       data;
  int32_t              seg_id; // use int32_t internally to signal "invalid segment id"
};

static dart_seghash_elem_t hashtab[DART_SEGMENT_HASH_SIZE];
static dart_seghash_elem_t *freelist_head = NULL;

static inline int hash_segid(dart_segid_t segid)
{
  /* Simply use the lower bits of the segment ID.
   * Since segment IDs are allocated continuously, this is likely to cause
   * collisions starting at (segment number == DART_SEGMENT_HASH_SIZE)
   * TODO: come up with a random distribution to account for random free'd segments?
   * */
  return (abs(segid) % DART_SEGMENT_HASH_SIZE);
}

/**
 * @brief Initialize the segment data hash table.
 */
dart_ret_t dart_segment_init()
{
  int i;
  memset(hashtab, 0, sizeof(dart_seghash_elem_t) * DART_SEGMENT_HASH_SIZE);

  for (i = 0; i < DART_SEGMENT_HASH_SIZE; i++) {
    hashtab[i].seg_id = DART_SEGMENT_INVALID;
  }

  return DART_OK;
}

static dart_segment_t * get_segment(dart_segid_t segid)
{
  int slot = hash_segid(segid);
  dart_seghash_elem_t *elem = &hashtab[slot];

  while (elem != NULL) {
    if (elem->data.segid == segid) {
      break;
    }
    elem = elem->next;
  }

  if (elem == NULL) {
    DART_LOG_ERROR("dart_segment__get_segment : Invalid segment ID %i",
                   segid);
    return NULL;
  }

  return &(elem->data);
}

/**
 * @brief Allocates a new segment data struct. May be served from a freelist.
 *
 * @return A pointer to an empty segment data object.
 */
dart_ret_t dart_segment_alloc(dart_segid_t segid, uint16_t team_idx)
{
  DART_LOG_DEBUG("dart_segment_alloc() segid:%d team_id:%d",
                 segid, team_idx);

  int slot = hash_segid(segid);
  dart_seghash_elem_t *elem = &hashtab[slot];

  if (elem->seg_id != DART_SEGMENT_INVALID) {
    dart_seghash_elem_t *pred = NULL;
    // we cannot use the first element, go to the last element in the
    // slot's list
    while (elem != NULL) {
      if (elem->seg_id == segid) {
        elem->data.segid = segid;
        elem->data.team_idx = team_idx;
        return DART_OK;
      }
      pred = elem;
      elem = elem->next;
    }

    // add a new element to the list, either allocate new or take from
    // freelist
    if (freelist_head != NULL) {
      elem = freelist_head;
      freelist_head = freelist_head->next;
      elem->next = NULL;
    } else {
      elem = calloc(1, sizeof(dart_seghash_elem_t));
    }
    pred->next = elem;

  }
  elem->seg_id        = segid;
  elem->data.segid    = segid;
  elem->data.team_idx = team_idx;

  DART_LOG_DEBUG("dart_segment_alloc > segid:%d team_id:%d",
                 segid, team_idx);
  return DART_OK;
}

/**
 * @brief Returns the registered segment data for the segment ID.
 *
 * @return A pointer to an existing segment data object.
 *         NULL if the segment ID was not found.
 */
dart_ret_t dart_segment_get_teamidx(dart_segid_t segid, uint16_t *team_idx)
{
  dart_segment_t *segment = get_segment(segid);
  if (segment == NULL) {
    // entry not found!
    DART_LOG_ERROR("dart_segment_get_teamidx ! Invalid segment ID %i", segid);
    return DART_ERR_INVAL;
  }

  *team_idx = segment->team_idx;
  return DART_OK;
}


dart_ret_t dart_segment_add_info(const dart_segment_info_t *item)
{
  dart_segment_t *segment = get_segment(item->seg_id);
  if (segment == NULL) {
    DART_LOG_ERROR("Invalid segment ID %i", item->seg_id);
    return DART_ERR_INVAL;
  }
  DART_LOG_TRACE(
    "dart_adapt_transtable_add() item: "
    "seg_id:%d size:%zu disp:%"PRIu64" win:%"PRIu64"",
    item->seg_id, item->size, (unsigned long)item->disp, (unsigned long)item->win);
  segment->seg_info.seg_id  = item->seg_id;
  segment->seg_info.size    = item->size;
  segment->seg_info.disp    = item->disp;
#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
  segment->seg_info.win     = item->win;
  segment->seg_info.baseptr = item->baseptr;
#endif
  segment->seg_info.selfbaseptr = item->selfbaseptr;

  return DART_OK;
}

#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
dart_ret_t dart_segment_get_win(int16_t seg_id, MPI_Win * win)
{
  dart_segment_t *segment = get_segment(seg_id);
  if (segment == NULL || segment->seg_info.seg_id != seg_id) {
    DART_LOG_ERROR("Invalid segment ID %i", seg_id);
    return DART_ERR_INVAL;
  }

  *win = segment->seg_info.win;
  return DART_OK;
}
#endif

dart_ret_t dart_segment_get_disp(int16_t             seg_id,
                                 dart_team_unit_t    rel_unitid,
                                 MPI_Aint          * disp_s)
{
  MPI_Aint trans_disp = 0;
  *disp_s  = 0;

  DART_LOG_TRACE("dart_segment_get_disp() "
                 "seq_id:%d rel_unitid:%d", seg_id, rel_unitid.id);

  dart_segment_t *segment = get_segment(seg_id);
  if (segment == NULL || segment->seg_info.seg_id != seg_id) {
    DART_LOG_ERROR("dart_segment_get_disp ! Invalid segment ID %i", seg_id);
    return DART_ERR_INVAL;
  }

  trans_disp = segment->seg_info.disp[rel_unitid.id];
  *disp_s    = trans_disp;
  DART_LOG_TRACE("dart_segment_get_disp > dist:%"PRIu64"",
                 (unsigned long)trans_disp);
  return DART_OK;
}

#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
dart_ret_t dart_segment_get_baseptr(
  int16_t               seg_id,
  dart_team_unit_t      rel_unitid,
  char              **  baseptr_s)
{
  dart_segment_t *segment = get_segment(seg_id);
  if (segment == NULL || segment->seg_info.seg_id != seg_id) {
    DART_LOG_ERROR("dart_segment_get_baseptr ! Invalid segment ID %i",
                   seg_id);
    return DART_ERR_INVAL;
  }

  *baseptr_s = segment->seg_info.baseptr[rel_unitid.id];
  return DART_OK;
}
#endif

dart_ret_t dart_segment_get_selfbaseptr(
  int16_t    seg_id,
  char   **  baseptr)
{
  dart_segment_t *segment = get_segment(seg_id);
  if (segment == NULL) {
    DART_LOG_ERROR("dart_segment_get_selfbaseptr ! Invalid segment ID %i",
                   seg_id);
    return DART_ERR_INVAL;
  }

  *baseptr = segment->seg_info.selfbaseptr;
  return DART_OK;
}

dart_ret_t dart_segment_get_size(
  int16_t   seg_id,
  size_t  * size)
{
  dart_segment_t *segment = get_segment(seg_id);
  if (segment == NULL || segment->seg_info.seg_id != seg_id) {
    DART_LOG_ERROR("dart_segment_get_size ! Invalid segment ID %i", seg_id);
    return DART_ERR_INVAL;
  }

  *size = segment->seg_info.size;
  return DART_OK;
}

static inline void free_segment_info(dart_segment_info_t *seg_info){
  if (seg_info->disp != NULL) {
    free(seg_info->disp);
    seg_info->disp = NULL;
  }
#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
  if (seg_info->baseptr) {
    free(seg_info->baseptr);
    seg_info->baseptr = NULL;
  }
#endif
  memset(seg_info, 0, sizeof(dart_segment_info_t));
}

/**
 * @brief Deallocates the segment identified by the segment ID.
 *
 * @return DART_OK on success.
 *         DART_ERR_INVAL if the segment was not found.
 */
dart_ret_t dart_segment_free(dart_segid_t segid)
{
  int slot = hash_segid(segid);
  dart_seghash_elem_t *pred = NULL;
  dart_seghash_elem_t *elem = &hashtab[slot];

  // shortcut: the bucket head is not moved to the freelist
  if (elem->seg_id == segid) {
    elem->seg_id = DART_SEGMENT_INVALID;
    return DART_OK;
  }

  // find the correct entry in this bucket
  pred = elem;
  elem = elem->next;
  while (elem != NULL) {

    if (elem->data.segid == segid) {
      pred->next = elem->next;
      free_segment_info(&elem->data.seg_info);
      elem->seg_id = DART_SEGMENT_INVALID;
      if (freelist_head == NULL) {
        // make it the new freelist head
        freelist_head = elem;
        freelist_head->next = NULL;
      } else {
        // insert after the freelist head
        elem->next = freelist_head->next;
        freelist_head->next = elem;
      }
      return DART_OK;
    }

    pred = elem;
    elem = elem->next;
  }

  // element not found
  return DART_ERR_INVAL;
}

static void clear_segdata_list(dart_seghash_elem_t *listhead)
{
  dart_seghash_elem_t *elem = listhead->next;
  while (elem != NULL) {
    dart_seghash_elem_t *tmp = elem;
    elem = tmp->next;
    tmp->next = NULL;
    free_segment_info(&tmp->data.seg_info);
    free(tmp);
  }
  listhead->next = NULL;
}

/**
 * @brief Clear the segment data hash table.
 */
dart_ret_t dart_segment_fini()
{
  int i;
  // clear the hash table
  for (i = 0; i < DART_SEGMENT_HASH_SIZE; i++) {
    clear_segdata_list(&hashtab[i]);
  }

  // clear the free_list
  if (freelist_head != NULL) {
    clear_segdata_list(freelist_head);
    free(freelist_head);
    freelist_head = NULL;
  }
  return DART_OK;
}
