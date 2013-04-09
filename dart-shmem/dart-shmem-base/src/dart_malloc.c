/*
 * dart_malloc.c
 *
 *  Created on: Apr 3, 2013
 *      Author: maierm
 */

#include "shmem_teams.h"
#include "dart_logger.h"
#include "dart_mempool_private.h"
#include <stddef.h>
#include "shmem_malloc.h"

shmid_address shmid2address[MAXNUM_TEAMS];

static void* find_shm_address(int shm_id, int offset)
{
	for (int i = 0; i < MAXNUM_TEAMS; i++)
	{
		if (shmid2address[i].key == shm_id
				&& shmid2address[i].value != ((void*) 0))
			return ((char*) shmid2address[i].value) + offset;
	}
	return ((void*) 0);
}

void putAddress(int teamid, int key, void* value)
{
	shmid_address toAdd =
	{ .key = key, .value = value };
	shmid2address[teamid] = toAdd;
	// TODO sort and binarySearch
}

void* getAdress(gptr_t gptr)
{
	return find_shm_address(gptr.unitid, gptr.offset);
}

gptr_t dart_alloc_aligned(int teamid, size_t nbytes)
{
	gptr_t result = GPTR_NULL;
	dart_mempool mempool = dart_team_mempool(teamid);
	if (mempool == DART_MEMPOOL_NULL )
	{
		ERROR("Could not alloc memory in mempool: DART_MEMPOOL_NULL%s", "");
		return result;
	}

	void* addr = dart_mempool_alloc(mempool, nbytes);
	if (addr == ((void*) 0))
	{
		ERROR("Could not alloc memory in mempool%s", "");
		return result;
	}

	int myid = dart_team_myid(teamid);
	result.unitid = mempool->shm_id;
	result.offset = ((char*) addr) - ((char*) mempool->shm_address);
	result.offset -= myid * mempool->size;
	dart_barrier(teamid);
	return result;
}

void dart_free(int teamid, gptr_t ptr)
{
	dart_mempool mempool = dart_team_mempool(teamid);
	if (mempool == DART_MEMPOOL_NULL )
	{
		ERROR("Could not free memory in mempool DART_MEMPOOL_NULL%s", "");
		return;
	}
	if (ptr.unitid != mempool->shm_id)
	{
		ERROR("Could not free memory: %s", "pointer has wrong segment");
		return;
	}
	char* shm_addr = (char*) mempool->shm_address;
	char* addr = shm_addr + ptr.offset;
	int myid = dart_team_myid(teamid);
	addr += myid * mempool->size;
	dart_mempool_free(mempool, addr);
	dart_barrier(teamid);
}

void dart_put(gptr_t ptr, void *src, size_t nbytes)
{
	char* dest = (char*) find_shm_address(ptr.unitid, ptr.offset);
	memcpy(dest, src, nbytes);
}

void dart_get(void *dest, gptr_t ptr, size_t nbytes)
{
	char* src = (char*) find_shm_address(ptr.unitid, ptr.offset);
	memcpy(dest, src, nbytes);
}

