/*
 * shmif_memory_manager.h
 *
 *  Created on: Apr 4, 2013
 *      Author: maierm
 */

#ifndef DART_SHMIF_MEMORY_MANAGER_H_
#define DART_SHMIF_MEMORY_MANAGER_H_

#include <stddef.h>

/**
 * creates or retrieves shared memory segment
 */
int shmif_mm_create(size_t size);

/**
 * destroys the specified shared memory segment
 */
void shmif_mm_destroy(int key);

/**
 * attaches the shared-memory-segment at the position returned
 */
void* shmif_mm_attach(int shmem_key);

/**
 * detaches the shared-memory-segment
 */
void shmif_mm_detach(void* addr);

#endif /* SHMIF_MEMORY_MANAGER_H_ */
