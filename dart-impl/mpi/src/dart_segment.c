#include <string.h>
#include <dash/dart/mpi/dart_segment.h>
#include <stdlib.h>

#define DART_SEGMENT_HASH_SIZE 256
#define DART_SEGMENT_INVALID   ((int)-1)

// forward declaration to make the compiler happy
typedef struct dart_seghash_elem dart_seghash_elem_t;

struct dart_seghash_elem {
  dart_seghash_elem_t *next;
  dart_segment_t data;
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
void dart_segment_init()
{
  int i;
  memset(hashtab, 0, sizeof(dart_seghash_elem_t) * DART_SEGMENT_HASH_SIZE);

  for (i = 0; i < DART_SEGMENT_HASH_SIZE; i++) {
    hashtab[i].data.segid = DART_SEGMENT_INVALID;
  }
}

/**
 * @brief Allocates a new segment data struct. May be served from a freelist.
 *
 * @return A pointer to an empty segment data object.
 */
dart_segment_t * dart_segment_alloc(dart_segid_t segid)
{
  int slot = hash_segid(segid);
  dart_seghash_elem_t *elem = &hashtab[slot];
  
  if (elem->data.segid != DART_SEGMENT_INVALID) {
    dart_seghash_elem_t *pred = NULL;
    // we cannot use the first element --> go to the last element in the slot's list
    while (elem != NULL) {
      if (elem->data.segid == segid) {
        return &(elem->data);
      }
      pred = elem;
      elem = elem->next;
    }

    // add a new element to the list, either allocate new or take from freelist
    if (freelist_head != NULL) {
      elem = freelist_head;
      freelist_head = freelist_head->next;
      elem->next = NULL;
    } else {
      elem = calloc(1, sizeof(dart_seghash_elem_t));
    }
    pred->next = elem;

  }
  elem->data.segid = segid;

  return &(elem->data);

}

/**
 * @brief Returns the registered segment data for the segment ID.
 *
 * @return A pointer to an existing segment data object.
 *         NULL if the segment ID was not found.
 */
dart_segment_t * dart_segment_get(dart_segid_t segid)
{
  int slot = hash_segid(segid);
  dart_seghash_elem_t *elem = &hashtab[slot];

  while (elem != NULL) {
    if (elem->data.segid == segid) {
      return &(elem->data);
    }
    elem = elem->next;
  }

  // entry not found!
  return NULL;
}

/**
 * @brief Deallocates the segment identified by the segment ID.
 *
 * @return 0 on success.
 *        <0 if the segment was not found.
 */
int dart_segment_dealloc(dart_segid_t segid)
{
  int slot = hash_segid(segid);
  dart_seghash_elem_t *pred = NULL;
  dart_seghash_elem_t *elem = &hashtab[slot];

  // shortcut: the bucket head is not moved to the freelist
  if (elem->data.segid == segid) {
    elem->data.segid = DART_SEGMENT_INVALID;
    return 0;
  }

  // find the correct entry in this bucket
  pred = elem;
  elem = elem->next;
  while (elem != NULL) {

    if (elem->data.segid == segid) {
      elem->data.segid = DART_SEGMENT_INVALID;
      if (freelist_head == NULL) {
        // make it the new freelist head
        freelist_head = elem;
        freelist_head->next = NULL;
      } else {
        // insert after the freelist head
        elem->next = freelist_head->next;
        freelist_head->next = elem;
      }
      pred->next = elem->next;
      return 0;
    }

    pred = elem;
    elem = elem->next;
  }

  // element not found
  return -1;
}

static void clear_segdata_list(dart_seghash_elem_t *listhead)
{
  dart_seghash_elem_t *elem = listhead->next;
  while (elem != NULL) {
    dart_seghash_elem_t *tmp = elem;
    elem = tmp->next;
    tmp->next = NULL;
    free(tmp);
  }
  listhead->next = NULL;
}

/**
 * @brief Clear the segment data hash table.
 */
void dart_segment_clear()
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

}
