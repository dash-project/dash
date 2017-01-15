#ifndef BUDDY_MEMORY_ALLOCATION_H
#define BUDDY_MEMORY_ALLOCATION_H

/* TODO: Needs refactoring, implementation from
 *       https://github.com/cloudwu/buddy
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <inttypes.h>

#define DART_MAX_TEAM_NUMBER (256)
#define DART_MAX_LENGTH (1024*1024*16)

struct dart_buddy;


extern char* dart_mempool_localalloc;
extern struct dart_buddy* dart_localpool;

/**
 * Create a new buddy allocator instance.
 *
 * The amount of memory allocatable through the allocator
 * depends on the number of levels in the binary tree.
 * The maximum number of bytes managed by the allocator is
 * 2**(level). The internal memory requirements are
 * O(2**(2*level)).
 *
 * \param level The number of levels of the internal binary tree.
 */
struct dart_buddy *
dart_buddy_new(int level);

/**
 * Delete the given buddy allocator instance.
 */
void     dart_buddy_delete(struct dart_buddy *);

/**
 * Allocate memory from the external memory pool.
 *
 * \return The offset relative to the starting adddress of the external
 *         memory block where the allocated memory begins.
 */
uint64_t dart_buddy_alloc(struct dart_buddy *, size_t size);

/**
 * Return the previously allocated memory chunk to the allocator for reuse.
 */
int      dart_buddy_free(struct dart_buddy *, uint64_t offset);

/**
 * ???
 */
int      buddy_size(struct dart_buddy *, uint64_t offset);
void     buddy_dump(struct dart_buddy *);

#endif
