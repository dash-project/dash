
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdio.h>
#include <unistd.h>

#include "dart_membucket.h"
#include "shmem_logger.h"

// static helpers
static void* add_to_pvoid(void* p, size_t size)
{
  return ((char*) p) + size; // TODO
}

static int pvoid_lt(void* p1, void* p2)
{
  return ((char*) p1) < ((char*) p2); // TODO
}

static int pvoid_eq(void* p1, void* p2)
{
  return ((char*) p1) == ((char*) p2); // TODO
}

static void free_membucket_list(dart_membucket_list list)
{
  dart_membucket_list current = list;
  while (current != NULL )
    {
      dart_membucket_list next = current->next;
      free(current);
      current = next;
    }
}


dart_membucket dart_membucket_create(void* pos, size_t size)
{
  dart_membucket result = malloc(sizeof(struct dart_opaque_membucket));
  
  result->free = (dart_membucket_list) 
    malloc(sizeof(dart_list_entry));

  result->allocated = NULL;
  result->free->pos = pos;
  result->free->size = size;
  result->free->next = NULL;
  result->size = size;
 
  return result;
}

void dart_membucket_destroy(dart_membucket bucket)
{
  int num_allocated = dart_membucket_list_size(bucket->allocated);

  /*
  if(num_allocated > 0) {
    ERROR("membucket: destroy called but number of "
	  "allocated chunks = %d", num_allocated);
  }
  */
  
  free_membucket_list(bucket->free);
  free_membucket_list(bucket->allocated);
  free(bucket);
}


int dart_membucket_free(dart_membucket bucket, void* pos)
{
  dart_membucket_list current = bucket->allocated;
  dart_membucket_list prev = NULL;
  while (current != NULL )
    {
      if (current->pos == pos)
	break;
      prev = current;
      current = current->next;
    }
  if (current == NULL )
    return 1;
  
  size_t sizeOfAllocated = current->size;
  bucket->allocated = dart_remove_list_entry(bucket->allocated, prev, current);
  
  dart_list_entry newFreeEntry;
  newFreeEntry.size = sizeOfAllocated;
  newFreeEntry.pos = pos;
  bucket->free = dart_insert_sorted(bucket->free, newFreeEntry);
  
  dart_list_melt(bucket->free);
  return 0;
}

void* dart_membucket_alloc(dart_membucket bucket, size_t size)
{
  // TODO: adjust size so that it's a multiple of some well aligned memory address
  dart_membucket_list current = bucket->free;
  dart_membucket_list prev = NULL;
  while (current != NULL )
    {
      if (current->size >= size)
	break;
      prev = current;
      current = current->next;
      
    }
  if (current == NULL )
    return ((void*) 0);
  
  // add allocated elem
  dart_list_entry newAllocEntry;
  newAllocEntry.size = size;
  newAllocEntry.pos = current->pos;
  bucket->allocated = dart_push_front(bucket->allocated, newAllocEntry);
  
  // remove elem from free list
  if (current->size == size)
    {
      bucket->free = dart_remove_list_entry(bucket->free, prev, current);
    }
  else
    {
      current->size -= size;
      current->pos = add_to_pvoid(current->pos, size);
    }
  
  return newAllocEntry.pos;
}

void dart_membucket_print(dart_membucket bucket, FILE* f)
{
  fprintf(f, "free:");
  dart_membucket_list_to_string(f, bucket->free);
  fprintf(f, "allocated:");
  dart_membucket_list_to_string(f, bucket->allocated);
}

///////////////////////////////////////////////////////////////////////////////////////
// private functions
///////////////////////////////////////////////////////////////////////////////////////

/**
 * @param prev my be null (which means that toRemove has no predecessor)
 * @param list != null
 * @param toRemove != null
 */
dart_membucket_list 
dart_remove_list_entry(dart_membucket_list list,
		       dart_membucket_list prev, dart_membucket_list toRemove)
{
  dart_membucket_list result = list;
  if (prev == NULL )
    result = list->next;
  else
    prev->next = toRemove->next;
  free(toRemove);
  return result;
}

/**
 * @param list may be null
 * @param newEntry
 */
dart_membucket_list dart_push_front(dart_membucket_list list,
				  dart_list_entry newEntry)
{
  dart_membucket_list newAlloc = (dart_membucket_list) malloc(
							  sizeof(dart_list_entry));
  *newAlloc = newEntry;
  newAlloc->next = list;
  return newAlloc;
}

dart_membucket_list dart_list_melt(dart_membucket_list list)
{
  if (list == NULL || list->next == NULL )
    return list;
  
  dart_membucket_list current = list->next;
  dart_membucket_list prev = list;
  while (current != NULL )
    {
      if (pvoid_eq(add_to_pvoid(prev->pos, prev->size), current->pos))
	{
	  // melt the two entries
	  prev->size += current->size;
	  prev->next = current->next;
	  free(current);
	  current = prev->next;
	}
      else
	{
	  prev = current;
	  current = current->next;
	}
    }
  return list;
}

/**
 * @param list may be null
 * @param newEntry
 */
dart_membucket_list dart_insert_sorted(dart_membucket_list list,
				     dart_list_entry newEntry)
{
  dart_membucket_list newAlloc = 
    (dart_membucket_list) malloc(sizeof(dart_list_entry));

  *newAlloc = newEntry;
  
  dart_membucket_list current = list;
  dart_membucket_list prev = NULL;
  while (current != NULL )
    {
      if (pvoid_lt(newEntry.pos, current->pos))
	break;
      prev = current;
      current = current->next;
    }
  
  newAlloc->next = current;
  if (prev != NULL )
    {
      prev->next = newAlloc;
      return list;
    }
  return newAlloc;
}

int dart_membucket_list_size(dart_membucket_list list)
{
  int result = 0;
  dart_membucket_list current = list;
  while (current != NULL )
    {
      result++;
      current = current->next;
    }
  return result;
}

void dart_membucket_list_to_string(FILE* f, dart_membucket_list list)
{
  dart_membucket_list current = list;
  while (current != NULL )
    {
      fprintf(f, "[pos:%p, size:%zu],", current->pos, current->size);
      current = current->next;
    }
}
