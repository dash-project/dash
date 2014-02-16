#ifndef SHMEM_MM_H_INCLUDED
#define SHMEM_MM_H_INCLUDED

/* Interface for SHMEM memory management */

/*
 * creates or retrieves shared memory segment
 */
int shmem_mm_create(size_t size);

/*
 * destroys the specified shared memory segment
 */
void shmem_mm_destroy(int key);

/*
 * attaches the shared-memory-segment at the position returned
 */
void* shmem_mm_attach(int shmem_key);

/*
 * detaches the shared-memory-segment
 */
void shmem_mm_detach(void* addr);

#endif /* SHMEM_MM_H_INCLUDED */
