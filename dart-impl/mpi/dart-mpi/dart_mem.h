#ifndef DART_MEM_H_INCLUDED
#define DART_MEM_H_INCLUDED
#define MAX_TEAM_NUMBER (256)
extern char *mempool_localalloc;
//extern int tail1;
extern char *mempool_globalalloc[MAX_TEAM_NUMBER];
//extern int tail2;
struct dart_mempool_list_entry
{
	int offset;
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

extern dart_mempool localpool;
extern dart_mempool dart_mempool_create (size_t size);
extern void dart_mempool_destroy(dart_mempool pool);
extern int dart_mempool_alloc (dart_mempool pool, size_t size);
extern int dart_mempool_free (dart_mempool pool, int offset);

/* private functions*/

extern dart_mempool_list dart_remove_list_entry (dart_mempool_list list, dart_mempool_list prev, dart_mempool_list toRemove);
extern dart_mempool_list dart_push_front (dart_mempool_list list, dart_list_entry newEntry);
extern dart_mempool_list dart_insert_sorted (dart_mempool_list list, dart_list_entry newEntry);
extern dart_mempool_list dart_list_melt (dart_mempool_list list);
#endif /*DART_MEM_H_INCLUDED*/
