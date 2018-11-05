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

#include <dash/dart/base/macro.h>

// forward declaration
struct dart_buddy;
extern char* dart_mempool_localalloc DART_INTERNAL;
extern struct dart_buddy* dart_localpool DART_INTERNAL;

/**
 * Create a new buddy allocator instance.
 *
 * The amount of memory allocatable through the allocator
 * depends on the number of levels in the binary tree.
 * The maximum number of bytes managed by the allocator is
 * 2**(level). The internal memory requirements are
 * O(2**(2*level)).
 *
 * \param size The size of the memory pool managed by the buddy allocator.
 */
struct dart_buddy *
dart_buddy_new(size_t size) DART_INTERNAL;

/**
 * Delete the given buddy allocator instance.
 */
void dart_buddy_delete(struct dart_buddy *) DART_INTERNAL;

/**
 * Allocate memory from the external memory pool.
 *
 * \return The offset relative to the starting adddress of the external
 *         memory block where the allocated memory begins.
 */
ssize_t dart_buddy_alloc(struct dart_buddy *, size_t size) DART_INTERNAL;

/**
 * Return the previously allocated memory chunk to the allocator for reuse.
 */
int dart_buddy_free(struct dart_buddy *, uint64_t offset) DART_INTERNAL;

/**
 * ???
 */
int buddy_size(struct dart_buddy *, uint64_t offset) DART_INTERNAL;
void buddy_dump(struct dart_buddy *) DART_INTERNAL;

#endif
