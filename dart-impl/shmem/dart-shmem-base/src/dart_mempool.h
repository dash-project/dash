/*
 * dart_mempool.h
 *
 *  Created on: Mar 8, 2013
 *      Author: maierm
 *
 *  Simple class that manages memory allocation and deallocation for a given memory region.
 */

#ifndef DART_MEMPOOL_H_
#define DART_MEMPOOL_H_
#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdio.h>

struct dart_opaque_mempool;
typedef struct dart_opaque_mempool* dart_mempool;
#define DART_MEMPOOL_NULL ((dart_mempool)0)

dart_mempool dart_mempool_create(void* pos, size_t size);

void dart_mempool_destroy(dart_mempool pool);

/**
 *	@return 0 on success
 */
int dart_mempool_free(dart_mempool pool, void* pos);

void* dart_mempool_alloc(dart_mempool pool, size_t size);

void dart_mempool_print(dart_mempool pool, FILE* f);

#ifdef __cplusplus
}
#endif
#endif /* DART_MEMPOOL_H_ */
