/*
 * dart_internal_sync.c
 *
 * internal synchronization based on (process-spanning) pthreads-synchronization
 *
 *  Created on: Mar 26, 2013
 *      Author: maierm
 */

#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <pthread.h>
#ifndef _POSIX_THREAD_PROCESS_SHARED
#error "This platform does not support process shared mutex"
#endif
#include "dart-shmem-base/shmif_barriers.h"
#include "sysv_logger.h"
#define MAX_NUM_GROUPS 64

struct dart_barrier
{
	pthread_mutex_t mutex;
	pthread_cond_t cond;
	int num_waiting;
	int num_procs;
};

typedef struct dart_barrier* dart_barrier_t;
#define DART_BARRIER_NULL ((dart_barrier_t)0)

struct sync_area_struct
{
	pthread_mutex_t lock;
	int num_barriers;
	// don't be confused: index is NOT a teamid !!! -> since teamids are NOT globally unique
	struct dart_barrier barriers[MAX_NUM_GROUPS];
};
typedef struct sync_area_struct* sync_area_t;
static sync_area_t area = (sync_area_t) 0;

int dart_barrier_create(dart_barrier_t barrier, int num_procs)
{

	pthread_mutexattr_t mutex_shared_attr;
	PTHREAD_SAFE(pthread_mutexattr_init(&mutex_shared_attr));
	PTHREAD_SAFE(
			pthread_mutexattr_setpshared(&mutex_shared_attr, PTHREAD_PROCESS_SHARED));
	PTHREAD_SAFE(pthread_mutex_init(&(barrier->mutex), &mutex_shared_attr));
	PTHREAD_SAFE(pthread_mutexattr_destroy(&mutex_shared_attr));

	pthread_condattr_t cond_shared_attr;
	PTHREAD_SAFE(pthread_condattr_init(&cond_shared_attr));
	PTHREAD_SAFE(
			pthread_condattr_setpshared(&cond_shared_attr, PTHREAD_PROCESS_SHARED));
	PTHREAD_SAFE(pthread_cond_init(&(barrier->cond), &cond_shared_attr));
	PTHREAD_SAFE(pthread_condattr_destroy(&cond_shared_attr));

	barrier->num_procs = num_procs;
	barrier->num_waiting = 0;

	return 0;
}

int dart_barrier_destroy(dart_barrier_t barrier)
{
	PTHREAD_SAFE(pthread_cond_destroy(&(barrier->cond)));
	PTHREAD_SAFE(pthread_mutex_destroy(&(barrier->mutex)));
	return 0;
}

int dart_barrier_await(dart_barrier_t barrier)
{
	PTHREAD_SAFE(pthread_mutex_lock(&(barrier->mutex)));
	(barrier->num_waiting)++;
	if (barrier->num_waiting < barrier->num_procs)
	{
		PTHREAD_SAFE(pthread_cond_wait(&(barrier->cond), &(barrier->mutex)));
	}
	else
	{
		barrier->num_waiting = 0;
		PTHREAD_SAFE(pthread_cond_broadcast(&(barrier->cond)));
	}
	PTHREAD_SAFE(pthread_mutex_unlock(&(barrier->mutex)));
	return 0;
}

int shmif_barriers_prolog(int numprocs, void* shm_addr)
{
	area = (sync_area_t) shm_addr;
	pthread_mutexattr_t mutex_shared_attr;
	PTHREAD_SAFE(pthread_mutexattr_init(&mutex_shared_attr));
	PTHREAD_SAFE(
			pthread_mutexattr_setpshared(&mutex_shared_attr, PTHREAD_PROCESS_SHARED));
	PTHREAD_SAFE(pthread_mutex_init(&(area->lock), &mutex_shared_attr));
	PTHREAD_SAFE(pthread_mutexattr_destroy(&mutex_shared_attr));
	dart_barrier_create(&(area->barriers[0]), numprocs);
	area->num_barriers = 1;
	return 0;
}

int shmif_barriers_epilog(int numprocs, void* shm_addr)
{
	// TODO: destroy mutex and barriers
	return 0;
}

void shmif_barriers_init(int numprocs, void* shm_addr)
{
	area = (sync_area_t) shm_addr;
}

void shmif_barriers_destroy()
{
	// do nothing
}

shmif_barrier_t shmif_barriers_create_barrier(int num_procs_to_wait)
{
	PTHREAD_SAFE_NORET(pthread_mutex_lock(&(area->lock)));
	if (area->num_barriers >= MAX_NUM_GROUPS)
	{
		PTHREAD_SAFE_NORET(pthread_mutex_unlock(&(area->lock)));
		ERROR("Could not create barrier: %s", "maxnum exceeded");
		return -1;
	}
	dart_barrier_create(&(area->barriers[area->num_barriers]),
			num_procs_to_wait);
	int result = area->num_barriers;
	area->num_barriers = area->num_barriers + 1;
	PTHREAD_SAFE_NORET(pthread_mutex_unlock(&(area->lock)));
	return result;
}

void shmif_barriers_barrier_wait(shmif_barrier_t barrier)
{
	dart_barrier_await(&(area->barriers[barrier]));
}
