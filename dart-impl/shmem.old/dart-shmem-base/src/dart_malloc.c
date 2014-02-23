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

void* find_local_address(gptr_t ptr)
{
	int segid = ptr.segid % MAXNUM_TEAMS;
	int is_aligned = ptr.segid / MAXNUM_TEAMS;
	void* seg_begin = dart_team_memory_segment_begin(segid, is_aligned);
	return ((char*)seg_begin) + ptr.offset;
}

gptr_t dart_alloc(size_t nbytes)
{
	gptr_t result = GPTR_NULL;
	dart_mempool mempool = dart_team_mempool_aligned(DART_TEAM_ALL);
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
	result.segid = dart_team_unique_id(DART_TEAM_ALL);
	result.offset = ((char*) addr) - ((char*) mempool->shm_address);
	return result;
}

gptr_t dart_alloc_aligned(int teamid, size_t nbytes)
{
	gptr_t result = GPTR_NULL;
	dart_mempool mempool = dart_team_mempool_aligned(teamid);
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
	result.segid = dart_team_unique_id(teamid) + MAXNUM_TEAMS;
	result.offset = ((char*) addr) - ((char*) mempool->shm_address);
	result = dart_gptr_switch_unit(result, teamid, dart_team_myid(teamid), 0);
	dart_barrier(teamid);
	return result;
}

void dart_free(int teamid, gptr_t ptr)
{
	int is_aligned = ptr.segid >= MAXNUM_TEAMS;
	dart_mempool mempool =
			(is_aligned) ?
					dart_team_mempool_aligned(teamid) :
					dart_team_mempool_nonAligned(teamid);
	if (mempool == DART_MEMPOOL_NULL )
	{
		ERROR("Could not free memory in mempool DART_MEMPOOL_NULL%s", "");
		return;
	}

	char* shm_addr = (char*) mempool->shm_address;
	char* addr = shm_addr + ptr.offset;
	if (is_aligned)
	{
		int myid = dart_team_myid(teamid);
		addr += myid * mempool->size;
	}
	dart_mempool_free(mempool, addr);
	dart_barrier(teamid);
}

void dart_put(gptr_t ptr, void *src, size_t nbytes)
{
	char* dest = (char*) find_local_address(ptr);
	memcpy(dest, src, nbytes);
}

void dart_get(void *dest, gptr_t ptr, size_t nbytes)
{
	char* src = (char*) find_local_address(ptr);
	memcpy(dest, src, nbytes);
}

