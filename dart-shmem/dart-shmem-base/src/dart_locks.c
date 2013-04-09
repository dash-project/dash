/*
 * dart_locks.c
 *
 *  Created on: Apr 7, 2013
 *      Author: maierm
 */

#include "dart_locks.h"
#include "shmif_locks.h"
#include <malloc.h>
#include "dart_teams.h"
#include "dart_malloc.h"
#include "dart_logger.h"

struct dart_opaque_lock_t
{
	gptr_t gptr;
	int team_id;
};

int dart_lock_team_init(int team_id, dart_lock* lock)
{
	gptr_t gptr = dart_alloc_aligned(team_id, shmif_lock_size_of()); // TODO alloc memory only at team-member 0
	*lock = malloc(sizeof(struct dart_opaque_lock_t));
	(*lock)->gptr = gptr;
	(*lock)->team_id = team_id;
	int myid = dart_team_myid(team_id);
	if (myid == 0)
	{
		void* addr = DART_ADDRESSOF(gptr);
		DEBUG("creating lock at address: %p", addr);
		if (shmif_lock_create_at(addr) != 0)
			return DART_ERR_OTHER;
	}
	dart_barrier(team_id);
	return DART_OK;
}

int dart_lock_free(dart_lock* lock)
{
	gptr_t gptr = (*lock)->gptr;
	int team_id = (*lock)->team_id;
	int myid = dart_team_myid((*lock)->team_id);
	free(*lock);
	if (myid == 0)
	{
		void* addr = DART_ADDRESSOF(gptr);
		DEBUG("freeing lock at address: %p", addr);
		if (shmif_lock_destroy(addr) != 0)
			return DART_ERR_OTHER;
	}
	*lock = NULL;
	dart_barrier(team_id);
	return DART_OK;
}

int dart_lock_acquire(dart_lock lock)
{
	void* addr = getAdress(lock->gptr);
	if (shmif_lock_acquire(addr, 1) != 0)
		return DART_ERR_OTHER;
	return DART_OK;
}

int dart_lock_try_acquire(dart_lock lock)
{
	void* addr = getAdress(lock->gptr);
	if (shmif_lock_acquire(addr, 0) != 0)
		return DART_ERR_OTHER;
	return DART_OK;
}

int dart_lock_release(dart_lock lock)
{
	void* addr = getAdress(lock->gptr);
	if (shmif_lock_release(addr) != 0)
		return DART_ERR_OTHER;
	return DART_OK;
}

