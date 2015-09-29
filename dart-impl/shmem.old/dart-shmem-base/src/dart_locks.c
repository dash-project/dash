/*
 * dart_locks.c
 *
 *  Created on: Apr 7, 2013
 *      Author: maierm
 */

#include "dart/dart_locks.h"
#include "dart/dart_communication.h"
#include "shmif_locks.h"
#include <malloc.h>
#include "shmem_teams.h"
#include "dart_logger.h"
#include "shmem_malloc.h"

struct dart_opaque_lock_t
{
	gptr_t gptr;
	int team_id;
};

int dart_lock_team_init(int team_id, dart_lock* lock)
{
	int result = DART_ERR_OTHER;
	int myteamid = dart_team_myid(team_id);
	*lock = malloc(sizeof(struct dart_opaque_lock_t));
	(*lock)->team_id = team_id;
	if (myteamid == 0)
	{
		(*lock)->gptr = dart_alloc(shmif_lock_size_of());
		void* addr = find_local_address((*lock)->gptr);
		DEBUG("creating lock at address: %p", addr);
		if ((result = shmif_lock_create_at(addr)) != DART_OK)
			return result;
	}
	result = dart_bcast(&((*lock)->gptr), sizeof(gptr_t), 0, team_id);
	return result;
}

int dart_lock_free(dart_lock* lock)
{
	gptr_t gptr = (*lock)->gptr;
	int team_id = (*lock)->team_id;
	int myid = dart_team_myid(team_id);
	free(*lock);
	if (myid == 0)
	{
		// TODO: dart_free??? (see comment in dart_malloc.h)
		void* addr = find_local_address(gptr);
		DEBUG("freeing lock at address: %p", addr);
		if (shmif_lock_destroy(addr) != 0)
			return DART_ERR_OTHER;
	}
	*lock = NULL;
	return DART_OK;
}

int dart_lock_acquire(dart_lock lock)
{
	int result = DART_ERR_OTHER;
	void* addr = find_local_address(lock->gptr);
	result = shmif_lock_acquire(addr, 1);
	return result;
}

int dart_lock_try_acquire(dart_lock lock)
{
	int result = DART_ERR_OTHER;
	void* addr = find_local_address(lock->gptr);
	result = shmif_lock_acquire(addr, 0);
	return result;
}

int dart_lock_release(dart_lock lock)
{
	void* addr = find_local_address(lock->gptr);
	return shmif_lock_release(addr);
}

