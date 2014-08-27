/** @file dart_mem.h
 *  @date 25 Aug 2014
 *  @brief Function prototypes for dart shared memory region management.
 */
#ifndef DART_ADAPT_MEM_H_INCLUDED
#define DART_ADAPT_MEM_H_INCLUDED
#include <stdio.h>
#include <mpi.h>
#include <stdint.h>
#define DART_MAX_TEAM_NUMBER (256)
#define DART_MAX_LENGTH (1024 * 1024 * 16)
#define DART_INFINITE (1<<30)

extern char* dart_mempool_localalloc;
extern char* dart_mempool_globalalloc [DART_MAX_TEAM_NUMBER];

struct dart_mempool_list_entry
{
	uint64_t offset;
	size_t size;
	struct dart_mempool_list_entry* next;
};
typedef struct dart_mempool_list_entry dart_list_entry;
typedef dart_list_entry* dart_mempool_list;
struct dart_opaque_mempool
{
	dart_mempool_list free_mem;
	dart_mempool_list allocated_mem;
	size_t size;
};
typedef struct dart_opaque_mempool* dart_mempool;

extern dart_mempool dart_localpool;
extern dart_mempool dart_globalpool[DART_MAX_TEAM_NUMBER];
dart_mempool dart_mempool_create (size_t size);
void dart_mempool_destroy(dart_mempool pool);
uint64_t dart_mempool_alloc (dart_mempool pool, size_t size);
int dart_mempool_free (dart_mempool pool, uint64_t offset);

/* Private functions. */

dart_mempool_list dart_remove_list_entry (dart_mempool_list list, dart_mempool_list prev, dart_mempool_list toRemove);
dart_mempool_list dart_push_front (dart_mempool_list list, dart_list_entry newEntry);
dart_mempool_list dart_insert_sorted (dart_mempool_list list, dart_list_entry newEntry);
dart_mempool_list dart_list_melt (dart_mempool_list list);

#endif /*DART_ADAPT_MEM_H_INCLUDED*/
