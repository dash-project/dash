/*
 * dart_mempool_test.h
 *
 *  Created on: Mar 8, 2013
 *      Author: maierm
 */

#ifndef DART_MEMPOOL_PRIVATE_H_
#define DART_MEMPOOL_PRIVATE_H_

#include <stddef.h>

typedef struct dart_mempool_list_entry* dart_mempool_list;
typedef struct dart_mempool_list_entry dart_list_entry;
struct dart_mempool_list_entry
{
	void* pos;
	size_t size;
	dart_mempool_list next;
};
struct dart_opaque_mempool
{
	dart_mempool_list free;
	dart_mempool_list allocated;
	void* shm_address;
	int shm_id;
	size_t size;
};

dart_mempool_list dart_remove_list_entry(dart_mempool_list list,
		dart_mempool_list prev, dart_mempool_list toRemove);

dart_mempool_list dart_push_front(dart_mempool_list list,
		dart_list_entry newEntry);

dart_mempool_list dart_list_melt(dart_mempool_list list);

dart_mempool_list dart_insert_sorted(dart_mempool_list list,
		dart_list_entry newEntry);

void dart_mempool_list_to_string(FILE* f, dart_mempool_list list);

int dart_mempool_list_size(dart_mempool_list list);

#endif /* DART_MEMPOOL_PRIVATE_H_ */
