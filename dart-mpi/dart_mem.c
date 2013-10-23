#include<stdio.h>
#include"dart_mem.h"
void free_mempool_list(dart_mempool_list list)
{
	dart_mempool_list current = list;
	while (current != NULL )
	{
		dart_mempool_list next = current->next;
		free(current);
		current = next;
	}
}
dart_mempool dart_mempool_create( size_t size)
{
	dart_mempool result = (dart_mempool) malloc(sizeof(struct dart_opaque_mempool));
	result->free_mem = (dart_mempool_list) malloc(sizeof(dart_list_entry));
	result->allocated_mem = NULL;
	result->free_mem->offset = 0;
	result->free_mem->size = size;
	result->free_mem->next = NULL;
	result->size = size;
	return result;
}

void dart_mempool_destroy(dart_mempool pool)
{

	free_mempool_list(pool->free_mem);
	free_mempool_list(pool->allocated_mem);
	free(pool);
}

int dart_mempool_alloc (dart_mempool pool, size_t size)
{
	dart_mempool_list current = pool->free_mem;
	dart_mempool_list prev = NULL;
	while (current != NULL)
	{
		if (current->size >= size)
			break;
		prev = current;
		current = current->next;
	}
	if (current == NULL )
		return ((void*) 0);

	// add allocated element
	dart_list_entry newAllocEntry;					
	newAllocEntry.size = size;
	newAllocEntry.offset = current->offset;
	pool->allocated_mem = dart_push_front(pool->allocated_mem, newAllocEntry);
	// remove element from free list
					
	if (current->size == size)					
	{
		pool->free_mem = dart_remove_list_entry(pool->free_mem, prev, current);
	}
	else
	{
		current->size -= size;
		current->offset = current->offset + size;
	}
	return newAllocEntry.offset;
}
int dart_mempool_free (dart_mempool pool, int offset)
{
	dart_mempool_list current = pool->allocated_mem;
	dart_mempool_list prev = NULL;
	while (current != NULL )
	{
		if (current->offset == offset)
			break;
		prev = current;
		current = current->next;
	}
	if (current == NULL )
		return 1;
	size_t sizeOfAllocated = current->size;
	pool->allocated_mem = dart_remove_list_entry(pool->allocated_mem, prev, current);
	dart_list_entry newFreeEntry;
	newFreeEntry.size = sizeOfAllocated;
	newFreeEntry.offset = offset;
	pool->free_mem = dart_insert_sorted(pool->free_mem, newFreeEntry);
	dart_list_melt(pool->free_mem);
	return 0;
}

dart_mempool_list dart_push_front (dart_mempool_list list, dart_list_entry newEntry)
{
	dart_mempool_list newAlloc = (dart_mempool_list) malloc(sizeof(dart_list_entry));
	*newAlloc = newEntry;
	newAlloc->next = list;
	return newAlloc;
}

dart_mempool_list dart_insert_sorted (dart_mempool_list list, dart_list_entry newEntry)
{

	dart_mempool_list newAlloc = (dart_mempool_list) malloc(sizeof(dart_list_entry));
	*newAlloc = newEntry;
	dart_mempool_list current = list;
	dart_mempool_list prev = NULL;
	while (current != NULL )
	{
		if (newEntry.offset < current->offset)
			break;
		prev = current;
		current = current->next;
	}
	newAlloc->next = current;
	if (prev != NULL)
	{
		prev->next = newAlloc;
		return list;
	}
	return newAlloc;
}

dart_mempool_list dart_remove_list_entry (dart_mempool_list list, dart_mempool_list prev, dart_mempool_list toRemove)
{
	dart_mempool_list result = list;
	if (prev == NULL )
		result = list->next;
	else
		prev->next = toRemove->next;
	free(toRemove);
	return result;
}

dart_mempool_list dart_list_melt(dart_mempool_list list)
{
	if (list == NULL || list->next == NULL )
		return list;
	dart_mempool_list current = list->next;
	dart_mempool_list prev = list;
	while (current != NULL )					
	{
		if ((prev->offset + prev->size) == current->offset)
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
		}											 																							}
	return list;
}														
