/*
 * dart_mempool.c
 *
 *  Created on: Mar 8, 2013
 *      Author: maierm
 */

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <stdio.h>
#include "dart_mempool.h"
#include "dart_mempool_private.h"
#include "dart_logger.h"

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

static void free_mempool_list(dart_mempool_list list)
{
	dart_mempool_list current = list;
	while (current != NULL )
	{
		dart_mempool_list next = current->next;
		free(current);
		current = next;
	}
}

// END: static helpers

dart_mempool dart_mempool_create(void* pos, size_t size)
{
	dart_mempool result = malloc(sizeof(struct dart_opaque_mempool));
	result->free = (dart_mempool_list) malloc(sizeof(dart_list_entry));
	result->allocated = NULL;
	result->free->pos = pos;
	result->free->size = size;
	result->free->next = NULL;
	result->size = size;
	return result;
}

void dart_mempool_destroy(dart_mempool pool)
{
	int num_allocated = dart_mempool_list_size(pool->allocated);
	if(num_allocated > 0)
		ERROR("mempool: destroy called but number of allocated chunks = %d", num_allocated);
	free_mempool_list(pool->free);
	free_mempool_list(pool->allocated);
	free(pool);
}

int dart_mempool_free(dart_mempool pool, void* pos)
{
	dart_mempool_list current = pool->allocated;
	dart_mempool_list prev = NULL;
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
	pool->allocated = dart_remove_list_entry(pool->allocated, prev, current);

	dart_list_entry newFreeEntry;
	newFreeEntry.size = sizeOfAllocated;
	newFreeEntry.pos = pos;
	pool->free = dart_insert_sorted(pool->free, newFreeEntry);

	dart_list_melt(pool->free);
	return 0;
}

void* dart_mempool_alloc(dart_mempool pool, size_t size)
{
	// TODO: adjust size so that it's a multiple of some well aligned memory address
	dart_mempool_list current = pool->free;
	dart_mempool_list prev = NULL;
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
	pool->allocated = dart_push_front(pool->allocated, newAllocEntry);

	// remove elem from free list
	if (current->size == size)
	{
		pool->free = dart_remove_list_entry(pool->free, prev, current);
	}
	else
	{
		current->size -= size;
		current->pos = add_to_pvoid(current->pos, size);
	}

	return newAllocEntry.pos;
}

void dart_mempool_print(dart_mempool pool, FILE* f)
{
	fprintf(f, "free:");
	dart_mempool_list_to_string(f, pool->free);
	fprintf(f, "allocated:");
	dart_mempool_list_to_string(f, pool->allocated);
}

///////////////////////////////////////////////////////////////////////////////////////
// private functions
///////////////////////////////////////////////////////////////////////////////////////

/**
 * @param prev my be null (which means that toRemove has no predecessor)
 * @param list != null
 * @param toRemove != null
 */
dart_mempool_list dart_remove_list_entry(dart_mempool_list list,
		dart_mempool_list prev, dart_mempool_list toRemove)
{
	dart_mempool_list result = list;
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
dart_mempool_list dart_push_front(dart_mempool_list list,
		dart_list_entry newEntry)
{
	dart_mempool_list newAlloc = (dart_mempool_list) malloc(
			sizeof(dart_list_entry));
	*newAlloc = newEntry;
	newAlloc->next = list;
	return newAlloc;
}

dart_mempool_list dart_list_melt(dart_mempool_list list)
{
	if (list == NULL || list->next == NULL )
		return list;

	dart_mempool_list current = list->next;
	dart_mempool_list prev = list;
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
dart_mempool_list dart_insert_sorted(dart_mempool_list list,
		dart_list_entry newEntry)
{
	dart_mempool_list newAlloc = (dart_mempool_list) malloc(
			sizeof(dart_list_entry));
	*newAlloc = newEntry;

	dart_mempool_list current = list;
	dart_mempool_list prev = NULL;
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

int dart_mempool_list_size(dart_mempool_list list)
{
	int result = 0;
	dart_mempool_list current = list;
	while (current != NULL )
	{
		result++;
		current = current->next;
	}
	return result;
}

void dart_mempool_list_to_string(FILE* f, dart_mempool_list list)
{
	dart_mempool_list current = list;
	while (current != NULL )
	{
		fprintf(f, "[pos:%p, size:%u],", current->pos, current->size);
		current = current->next;
	}
}
