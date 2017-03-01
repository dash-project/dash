#include <string.h>
#include <dash/dart/mpi/dart_segment.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/base/assert.h>
#include <stdlib.h>
#include <inttypes.h>

#define DART_SEGMENT_INVALID   (INT32_MAX)

struct dart_seghash_elem {
  dart_seghash_elem_t *next;
  dart_segment_info_t  data;
};


static inline int hash_segid(dart_segid_t segid)
{
  /* Simply use the lower bits of the segment ID.
   * Since segment IDs are allocated continuously, this is likely to cause
   * collisions starting at (segment number == DART_SEGMENT_HASH_SIZE)
   * TODO: come up with a random distribution to account for random free'd
   * segments?
   * */
  return (abs(segid) % DART_SEGMENT_HASH_SIZE);
}

static inline void
register_segment(dart_segmentdata_t *segdata, dart_seghash_elem_t *elem)
{
  int slot = hash_segid(elem->data.seg_id);
  elem->next = segdata->hashtab[slot];
  segdata->hashtab[slot] = elem;
}

/**
 * Initialize the segment data hash table.
 */
dart_ret_t dart_segment_init(dart_segmentdata_t *segdata, dart_team_t teamid)
{
  memset(segdata->hashtab, 0,
    sizeof(dart_seghash_elem_t*) * DART_SEGMENT_HASH_SIZE);

  segdata->team_id = teamid;
  segdata->mem_freelist = NULL;
  segdata->reg_freelist = NULL;
  segdata->memid = 1;
  segdata->registermemid = -1;

  // register the segment for non-global allocations on DART_TEAM_ALL
  if (teamid == DART_TEAM_ALL) {
    dart_seghash_elem_t *elem = calloc(1, sizeof(dart_seghash_elem_t));
    register_segment(segdata, elem);
  }
  return DART_OK;
}

static dart_segment_info_t * get_segment(
    dart_segmentdata_t *segdata,
    dart_segid_t        segid)
{
  int slot = hash_segid(segid);
  dart_seghash_elem_t *elem = segdata->hashtab[slot];

  while (elem != NULL) {
    if (elem->data.seg_id == segid) {
      break;
    }
    elem = elem->next;
  }

  if (elem == NULL) {
    DART_LOG_ERROR("dart_segment__get_segment : "
                   "Invalid segment ID %i on team %i",
                   segid, segdata->team_id);
    return NULL;
  }

  return &(elem->data);
}

/**
 * Allocates a new segment data struct. May be served from a freelist.
 *
 * \return A pointer to an empty segment data object.
 */
dart_segment_info_t *
dart_segment_alloc(dart_segmentdata_t *segdata, dart_segment_type type)
{
  DART_LOG_DEBUG("dart_segment_alloc() team_id:%d",
                 segdata->team_id);

  int16_t segid;
  dart_seghash_elem_t *elem = NULL;
  if (type == DART_SEGMENT_ALLOC) {
    if (segdata->mem_freelist != NULL) {
      elem  = segdata->mem_freelist;
      segid = elem->data.seg_id;
      segdata->mem_freelist = elem->next;
    } else {
      if (segdata->memid == INT16_MAX || segdata->memid <= 0) {
        DART_LOG_ERROR(
            "Failed to allocate segment ID, "
            "too many segments already allocated? (memid: %i)", segdata->memid);
        return NULL;
      }
      segid = segdata->memid++;
      elem = calloc(1, sizeof(dart_seghash_elem_t));
      elem->data.seg_id = segid;
    }
  } else if (type == DART_SEGMENT_REGISTER) {
    if (segdata->reg_freelist != NULL) {
      elem  = segdata->reg_freelist;
      segid = elem->data.seg_id;
      segdata->reg_freelist = elem->next;
    } else {
      if (segdata->registermemid == INT16_MIN || segdata->registermemid >= 0) {
        DART_LOG_ERROR(
            "Failed to allocate segment ID, "
            "too many segments already registered? (registermemid: %i)",
            segdata->registermemid);
        return NULL;
      }
      segid = segdata->registermemid--;
      elem = calloc(1, sizeof(dart_seghash_elem_t));
      elem->data.seg_id = segid;
    }
  } else {
    // this should not happen!
    DART_ASSERT(type != DART_SEGMENT_REGISTER && type != DART_SEGMENT_ALLOC);
  }

  register_segment(segdata, elem);

  DART_LOG_DEBUG("dart_segment_alloc > segid:%d team_id:%d",
                 segid, segdata->team_id);
  return &(elem->data);
}

#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
dart_ret_t dart_segment_get_win(
    dart_segmentdata_t * segdata,
    int16_t              seg_id,
    MPI_Win            * win)
{
  dart_segment_info_t *segment = get_segment(segdata, seg_id);
  if (segment == NULL) {
    DART_LOG_ERROR("Invalid segment ID %i on team %i", seg_id, segdata->team_id);
    return DART_ERR_INVAL;
  }

  *win = segment->win;
  return DART_OK;
}
#endif

dart_ret_t dart_segment_get_disp(dart_segmentdata_t * segdata,
                                 int16_t              seg_id,
                                 dart_team_unit_t     rel_unitid,
                                 MPI_Aint           * disp_s)
{
  MPI_Aint trans_disp = 0;
  *disp_s  = 0;

  DART_LOG_TRACE("dart_segment_get_disp() "
                 "seq_id:%d rel_unitid:%d", seg_id, rel_unitid.id);

  dart_segment_info_t *segment = get_segment(segdata, seg_id);

  if (segment == NULL) {
    DART_LOG_ERROR("dart_segment_get_disp ! Invalid segment ID %i on team %i",
        seg_id, segdata->team_id);
    return DART_ERR_INVAL;
  }

  trans_disp = segment->disp[rel_unitid.id];
  *disp_s    = trans_disp;
  DART_LOG_TRACE("dart_segment_get_disp > disp:%"PRIu64"",
                 (unsigned long)trans_disp);
  return DART_OK;
}

#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
dart_ret_t dart_segment_get_baseptr(
  dart_segmentdata_t  * segdata,
  int16_t               seg_id,
  dart_team_unit_t      rel_unitid,
  char              **  baseptr_s)
{
  dart_segment_info_t *segment = get_segment(segdata, seg_id);
  if (segment == NULL) {
    DART_LOG_ERROR("dart_segment_get_baseptr ! Invalid segment ID %i on team %i",
                   seg_id, segdata->team_id);
    return DART_ERR_INVAL;
  }

  *baseptr_s = segment->baseptr[rel_unitid.id];
  return DART_OK;
}
#endif

dart_ret_t dart_segment_get_selfbaseptr(
  dart_segmentdata_t  * segdata,
  int16_t               seg_id,
  char               ** baseptr)
{
  *baseptr = NULL;
  dart_segment_info_t *segment = get_segment(segdata, seg_id);
  if (segment == NULL) {
    DART_LOG_ERROR("dart_segment_get_selfbaseptr ! "
                   "Invalid segment ID %i on team %i",
                   seg_id, segdata->team_id);
    return DART_ERR_INVAL;
  }

  *baseptr = segment->selfbaseptr;
  return DART_OK;
}

dart_ret_t dart_segment_get_size(
  dart_segmentdata_t  * segdata,
  int16_t               seg_id,
  size_t              * size)
{
  dart_segment_info_t *segment = get_segment(segdata, seg_id);
  if (segment == NULL) {
    DART_LOG_ERROR("dart_segment_get_size ! Invalid segment ID %i", seg_id);
    return DART_ERR_INVAL;
  }

  *size = segment->size;
  return DART_OK;
}

dart_ret_t dart_segment_get_flags(
  dart_segmentdata_t * segdata,
  int16_t              seg_id,
  uint16_t           * flags)
{

  dart_segment_info_t *segment = get_segment(segdata, seg_id);
  if (segment == NULL) {
    DART_LOG_ERROR("dart_segment_get_size ! Invalid segment ID %i", seg_id);
    return DART_ERR_INVAL;
  }

  *flags = segment->flags;
  return DART_OK;
}

dart_ret_t dart_segment_set_flags(
  dart_segmentdata_t * segdata,
  int16_t              seg_id,
  uint16_t             flags)
{

  dart_segment_info_t *segment = get_segment(segdata, seg_id);
  if (segment == NULL) {
    DART_LOG_ERROR("dart_segment_get_size ! Invalid segment ID %i", seg_id);
    return DART_ERR_INVAL;
  }

  segment->flags = flags;
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
 * Deallocates the segment identified by the segment ID.
 *
 * \return DART_OK on success.
 *         DART_ERR_INVAL if the segment was not found.
 *
 */
dart_ret_t dart_segment_free(
  dart_segmentdata_t  * segdata,
  dart_segid_t          segid)
{
  int slot = hash_segid(segid);
  dart_seghash_elem_t *pred = NULL;
  dart_seghash_elem_t *elem = segdata->hashtab[slot];

  // find the correct entry in this bucket
  pred = NULL;
  while (elem != NULL) {

    if (elem->data.seg_id == segid) {
      if (pred != NULL) {
        pred->next = elem->next;
      } else {
        segdata->hashtab[slot] = elem->next;
      }
      // no need for locking since operations on the same segmentdata
      // are not thread-safe
      if (segid > 0) {
        elem->next            = segdata->mem_freelist;
        segdata->mem_freelist = elem;
      } else if (segid < 0){
        elem->next            = segdata->reg_freelist;
        segdata->reg_freelist = elem;
      } else {
        // This should not happen!
        DART_ASSERT(segid != 0);
      }
      // set the segment ID again
      elem->data.seg_id = segid;
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
  dart_seghash_elem_t *elem = listhead;
  while (elem != NULL) {
    dart_seghash_elem_t *tmp = elem;
    elem = tmp->next;
    tmp->next = NULL;
    free_segment_info(&tmp->data);
    free(tmp);
  }
}

/**
 * @brief Clear the segment data hash table.
 */
dart_ret_t dart_segment_fini(
  dart_segmentdata_t  * segdata)
{
  // clear the hash table
  for (int i = 0; i < DART_SEGMENT_HASH_SIZE; i++) {
    clear_segdata_list(segdata->hashtab[i]);
    segdata->hashtab[i] = NULL;
  }
  clear_segdata_list(segdata->mem_freelist);
  segdata->mem_freelist = NULL;

  clear_segdata_list(segdata->reg_freelist);
  segdata->reg_freelist = NULL;
  return DART_OK;
}
