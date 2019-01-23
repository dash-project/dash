#ifndef DART_TASKING_HASHTAB_H_
#define DART_TASKING_HASHTAB_H_

#include <stdlib.h>

#include <dash/dart/base/stack.h>
#include <dash/dart/tasking/dart_tasking_priv.h>

// we know that the stack member entry is the first element of the struct
// so we can cast directly
#define DART_DEPHASH_ELEM_POP(__freelist) \
  (dart_dephash_elem_t*)((void*)dart__base__stack_pop(&__freelist))

#define DART_DEPHASH_ELEM_PUSH(__freelist, __elem) \
  dart__base__stack_push(&__freelist, &DART_STACK_MEMBER_GET(__elem))

#define DART_DEPHASH_MAX_LEVEL 4

struct dart_dephash_elem {
  union {
    // atomic list used for free elements
    DART_STACK_MEMBER_DEF;
    // double linked list
    struct {
      dart_dephash_elem_t *next;
      dart_dephash_elem_t *prev;
    };
  };
  dart_dephash_elem_t      *next_in_task; // list in the task struct
  uint64_t                  hash;
  dart_task_dep_t           taskdep;
  taskref                   task;    // the task referred to by the dependency
  dart_global_unit_t        origin;  // the unit this dependency originated from
};

struct dart_dephash_head {
  dart_tasklock_t lock;
  bool            has_next_level;
  bool            used;    // the bucket has been used
  union {
    dart_dephash_elem_t *head;    // the head of the bucket
    dart_dephash_tab_t  *hashtab; // the next level hash table, if has_next_level
  };
};

struct dart_dephash_tab {
  int                 size;
  int                 level;
  dart_dephash_head_t buckets[];
};


static inline uint64_t hash_gptr(dart_gptr_t gptr)
{
  // use larger types to accomodate for the shifts below
  // and unsigned to force logical shifts (instead of arithmetic)
  // NOTE: we ignore the teamid here because gptr in dependencies contain
  //       global unit IDs
  uint32_t segid  = gptr.segid;
  uint64_t hash   = gptr.addr_or_offs.offset;
  uint64_t unitid = gptr.unitid; // 64-bit required for shift
  // cut off the lower 2 bit, we assume that pointers are 4-byte aligned
  hash >>= 2;
  // mix in unit, team and segment ID
  //hash ^= (segid  << 16); // 16 bit segment ID
  hash ^= (unitid << 32); // 24 bit unit ID
  // using a prime number in modulo stirs reasonably well

  DART_LOG_TRACE("hash_gptr(u:%lu, s:%d, o:%p) => (%lu)",
                 unitid, segid, gptr.addr_or_offs.addr, hash);

  return (hash);
}

static dart_dephash_tab_t * hashtab_new(int size, int level)
{
  dart_dephash_tab_t *hashtab = malloc(sizeof(dart_dephash_tab_t) +
                                       sizeof(dart_dephash_head_t) * size);
  hashtab->level = level;
  hashtab->size  = size;
  memset(hashtab->buckets, 0, size*sizeof(dart_dephash_head_t));
  return hashtab;
}

static void hashtab_destroy(dart_dephash_tab_t *hashtab)
{
  for (int i = 0; i < hashtab->size; ++i) {
    if (hashtab->buckets[i].has_next_level) {
      hashtab_destroy(hashtab->buckets[i].hashtab);
      hashtab->buckets[i].head = NULL;
    }
  }
  free(hashtab);
}

/**
 * Recursively find the right hashtable based on an already generated hash.
 */
static dart_dephash_tab_t *hashtab_get_table_hash(
  dart_dephash_tab_t* hashtab,
  uint64_t            hash)
{
  int slot = hash%hashtab->size;
  dart_dephash_head_t *bucket = &hashtab->buckets[slot];
  if (bucket->has_next_level) {
    return hashtab_get_table_hash(bucket->hashtab, hash);
  }
  return hashtab;
}

/**
 * Find the right hashtable based on a dart_gptr_t.
 */
static dart_dephash_tab_t *hashtab_get_table_gptr(
  dart_dephash_tab_t* hashtab,
  dart_gptr_t gptr)
{
  uint64_t hash = hash_gptr(gptr);
  return hashtab_get_table_hash(hashtab, hash);
}

static void hashtab_extend_bucket_nolock(
  dart_dephash_head_t *bucket,
  dart_dephash_tab_t  *parent_tab)
{
  // double the size of hashtable
  uint64_t hash = bucket->head->hash;
  // grow the hash table by factor 1.5
  int size = (((parent_tab->size + 1) * 3) / 2) -1;
  DART_LOG_TRACE("Adding level %d hashtab to slot %ld",
                 parent_tab->level + 1, hash%parent_tab->size);
  // save the existing elements
  dart_dephash_elem_t *elems = bucket->head;
  // allocate new table
  bucket->hashtab = hashtab_new(size, parent_tab->level + 1);
  bucket->has_next_level = true;
  // re-insert existing elements
  int slot = hash % size;
  bucket->hashtab->buckets[slot].head = elems;
}

static void
hashtab_insert_head_nolock(
  dart_dephash_head_t *bucket,
  dart_dephash_elem_t *elem)
{
  if (bucket->head != NULL) {
    bucket->head->prev = elem;
  }
  elem->next = bucket->head;
  elem->prev = NULL;
  bucket->head = elem;
  bucket->used = true;
}

static void hashtab_insert_elem_hash(
  dart_dephash_tab_t  *hashtab,
  dart_dephash_elem_t *elem,
  uint64_t             hash)
{
  int slot;
  dart_dephash_head_t *bucket;
  elem->hash = hash;
  while (1) {
    slot   = hash%hashtab->size;
    bucket = &hashtab->buckets[slot];
    DART_LOG_TRACE("Inserting elem with hash %lu into hashtab %p of size %d",
                  hash, hashtab, hashtab->size);
    if (bucket->has_next_level) {
      // recurse to the next level
      //hashtab_insert_elem_hash(bucket->hashtab, elem, hash);
      //return;
      hashtab = bucket->hashtab;
      continue;
    }
    LOCK_TASK(bucket);
    if (!bucket->has_next_level && bucket->head != NULL) {
      if (!DART_GPTR_EQUAL(bucket->head->taskdep.gptr, elem->taskdep.gptr) &&
          hashtab->level < DART_DEPHASH_MAX_LEVEL) {
        DART_LOG_TRACE("Need to extend: head:{o:%p, u:%d} vs elem:{o:%p, u:%d}",
                       bucket->head->taskdep.gptr.addr_or_offs.addr,
                       bucket->head->taskdep.gptr.unitid,
                       elem->taskdep.gptr.addr_or_offs.addr,
                       elem->taskdep.gptr.unitid);
        hashtab_extend_bucket_nolock(bucket, hashtab);
        // unlock and continue with next level
        UNLOCK_TASK(bucket);
        hashtab = bucket->hashtab;
        continue;
      }
    }

    // done, keep lock and insert
    break;
  }
  // make the element the new head and unlock the bucket
  DART_LOG_TRACE("Inserting task into bucket %d (%p)", slot, bucket);
  hashtab_insert_head_nolock(bucket, elem);
  UNLOCK_TASK(bucket);
}

static void hashtab_insert_elem_hash_nolock(
  dart_dephash_tab_t  *hashtab,
  dart_dephash_elem_t *elem,
  uint64_t             hash)
{
  elem->hash = hash;
  int slot = hash%hashtab->size;
  dart_dephash_head_t *bucket = &hashtab->buckets[slot];
  while (1) {
    if (bucket->has_next_level) {
      // recurse to the next level
      hashtab_insert_elem_hash_nolock(bucket->hashtab, elem, hash);
      return;
    }
    if (!bucket->has_next_level) {
      if (!DART_GPTR_EQUAL(bucket->head->taskdep.gptr, elem->taskdep.gptr) &&
          hashtab->level < DART_DEPHASH_MAX_LEVEL) {
        hashtab_extend_bucket_nolock(bucket, hashtab);
        // unlock and continue with next level
        continue;
      }
    }
    break;
  }
  // make the element the new head and unlock the bucket
  hashtab_insert_head_nolock(bucket, elem);
}

static void hashtab_insert_elem(
  dart_dephash_tab_t  *hashtab,
  dart_dephash_elem_t *elem)
{
  uint64_t hash = hash_gptr(elem->taskdep.gptr);
  return hashtab_insert_elem_hash(hashtab, elem, hash);
}

typedef int (hashtab_iterator_fn)(
  dart_dephash_tab_t  *hashtab,
  dart_dephash_elem_t *elem,
  void                *user_data);

static void hashtab_foreach_in_bucket_nolock(
  dart_dephash_tab_t  *hashtab,
  uint64_t             hash,
  hashtab_iterator_fn *fn,
  void                *user_data)
{
  int slot = hash%hashtab->size;
  dart_dephash_head_t *bucket = &hashtab->buckets[slot];
  if (bucket->has_next_level) {
    hashtab_foreach_in_bucket_nolock(bucket->hashtab, hash, fn, user_data);
    return;
  }
  for (dart_dephash_elem_t *elem = bucket->head;
                            elem != NULL;
                            elem = elem->next) {
    int ret = fn(hashtab, elem, user_data);
    if (ret != 0) {
      break;
    }
  }
}

static void hashtab_bucket_insert_before_elem_hash_nolock(
  dart_dephash_head_t *bucket,
  dart_dephash_elem_t *elem,
  dart_dephash_elem_t *new_elem)
{
  // NOTE: no need to check for collision in the bucket
  new_elem->hash = elem->hash;
  if (elem->prev == NULL) {
    // insert at the head of the bucket
    hashtab_insert_head_nolock(bucket, new_elem);
  } else {
    new_elem->next = elem;
    new_elem->prev = elem->prev;
    new_elem->prev->next = new_elem;
    elem->prev = new_elem;
  }
}

static void hashtab_insert_before_elem_hash_nolock(
  dart_dephash_tab_t  *hashtab,
  dart_dephash_elem_t *elem,
  dart_dephash_elem_t *new_elem,
  uint64_t             hash)
{
  // NOTE: no need to check for collision in the bucket
  new_elem->hash = hash;
  if (elem->prev == NULL) {
    // insert at the head of the bucket
    int slot = hash%hashtab->size;
    dart_dephash_head_t *bucket = &hashtab->buckets[slot];
    hashtab_insert_head_nolock(bucket, new_elem);
  } else {
    new_elem->prev = elem->prev;
    new_elem->prev->next = new_elem;
    new_elem->next = elem;
    elem->prev = new_elem;
  }
}

static dart_dephash_head_t * hashtab_get_locked_bucket(
  dart_dephash_tab_t  *hashtab,
  uint64_t             hash)
{
  int slot = hash%hashtab->size;

  dart_dephash_head_t *bucket = &hashtab->buckets[slot];
  if (bucket->has_next_level) {
    return hashtab_get_locked_bucket(bucket->hashtab, hash);
  }

  LOCK_TASK(bucket);

  return bucket;
}

static void hashtab_unlock_bucket(
  dart_dephash_head_t *bucket)
{
  UNLOCK_TASK(bucket);
}

static void hashtab_remove_elem(
  dart_dephash_tab_t  *hashtab,
  dart_dephash_elem_t *elem,
  uint64_t             hash)
{
  // we need to lock the bucket even if the element is not the head
  dart_dephash_head_t *bucket = hashtab_get_locked_bucket(hashtab, hash);

  if (elem == bucket->head) {
    // replace bucket head
    bucket->head = elem->next;
    if (bucket->head != NULL) {
      bucket->head->prev = NULL;
    }
    elem->next = elem->prev = NULL;
  } else {
    elem->prev->next = elem->next;
    if (elem->next != NULL) {
      elem->next->prev = elem->prev;
    }
    elem->next = elem->prev = NULL;
  }

  hashtab_unlock_bucket(bucket);
}

void hashtab_gather_stats(
  dart_dephash_tab_t  *hashtab,
  size_t *used,
  size_t *unused,
  int    *max_level)
{
  if (*max_level < hashtab->level) *max_level = hashtab->level;
  for (size_t i = 0; i < hashtab->size; ++i) {
    dart_dephash_head_t *bucket = &hashtab->buckets[i];
    if (bucket->has_next_level) {
      *used += 1;
      hashtab_gather_stats(bucket->hashtab, used, unused, max_level);
    } else if (bucket->used) {
      *used += 1;
    } else {
      *unused += 1;
    }
  }
}

void hashtab_print_stats(dart_dephash_tab_t  *hashtab)
{
  size_t used_buckets = 0, free_buckets = 0;
  int max_level = 0;
  hashtab_gather_stats(hashtab, &used_buckets, &free_buckets, &max_level);

  DART_LOG_INFO("hashtab %p: used buckets: %ld, unused buckets: %ld, max level: %d",
                hashtab, used_buckets, free_buckets, max_level);
}

#endif /* DART_TASKING_HASHTAB_H_ */
