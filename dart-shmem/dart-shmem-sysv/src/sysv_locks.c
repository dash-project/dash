/*
 * sysv_locks.c
 *
 *  Created on: Apr 7, 2013
 *      Author: maierm
 */

#include "dart-shmem-base/shmif_locks.h"
#include "sysv_logger.h"
#include <pthread.h>
#ifndef _POSIX_THREAD_PROCESS_SHARED
#error "This platform does not support process shared mutex"
#endif

int shmif_lock_create_at(void* addr)
{
	pthread_mutexattr_t mutex_shared_attr;
	PTHREAD_SAFE(pthread_mutexattr_init(&mutex_shared_attr));
	PTHREAD_SAFE(
			pthread_mutexattr_setpshared(&mutex_shared_attr, PTHREAD_PROCESS_SHARED));
	PTHREAD_SAFE(
			pthread_mutex_init((pthread_mutex_t*)addr, &mutex_shared_attr));
	PTHREAD_SAFE(pthread_mutexattr_destroy(&mutex_shared_attr));
	return 0;
}

int shmif_lock_destroy(void* addr)
{
	PTHREAD_SAFE(pthread_mutex_destroy((pthread_mutex_t*)addr));
	return 0;
}

int shmif_lock_acquire(void* addr, int is_blocking)
{
	if (is_blocking)
	{
		PTHREAD_SAFE(pthread_mutex_lock((pthread_mutex_t*)addr));
	}
	else
	{
		int result = -1;
		PTHREAD_SAFE(result = pthread_mutex_trylock((pthread_mutex_t*)addr));
		if (result == EBUSY)
			return 1;
	}
	return 0;
}

int shmif_lock_release(void* addr)
{
	PTHREAD_SAFE(pthread_mutex_unlock((pthread_mutex_t*)addr));
	return 0;
}

int shmif_lock_size_of()
{
	return sizeof(pthread_mutex_t);
}
