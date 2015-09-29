/*
 * shmem_malloc.h
 *
 *  Created on: Apr 11, 2013
 *      Author: maierm
 */

#ifndef SHMEM_MALLOC_H_
#define SHMEM_MALLOC_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "dart/dart_malloc.h"

void* find_local_address(gptr_t ptr);

#ifdef __cplusplus
}
#endif
#endif /* SHMEM_MALLOC_H_ */
