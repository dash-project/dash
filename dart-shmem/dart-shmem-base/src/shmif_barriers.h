/*
 * shmif_barriers.h
 *
 * provides methods used to synchronize processes/threads
 *
 *  Created on: Apr 4, 2013
 *      Author: maierm
 */

#ifndef DART_SHMIF_BARRIERS_H_
#define DART_SHMIF_BARRIERS_H_

typedef int shmif_barrier_t;

/**
 * called by ONE process (e.g. before starting the actual workers)
 */
int shmif_barriers_prolog(int numprocs, void* shm_addr);
int shmif_barriers_epilog(int numprocs, void* shm_addr);

/**
 * called by each of the worker procs
 */
void shmif_barriers_init(int numprocs, void* shm_addr);
void shmif_barriers_destroy();

/**
 * @return identifier of a barrier that waits for num_procs_to_wait processes (last process restores the barrier)
 */
shmif_barrier_t shmif_barriers_create_barrier(int num_procs_to_wait);

void shmif_barriers_barrier_wait(shmif_barrier_t barrier);

#endif /* SHMIF_BARRIERS_H_ */
