#include <stdio.h>
#include <stddef.h>

#include "dart_return_codes.h"
#include "dart_teams.h"
#include "dart_mempool_private.h"
#include "dart_logger.h"
#include "dart_malloc.h"
#include "shmif_multicast.h"
#include "shmif_barriers.h"
#include "shmif_memory_manager.h"

static int _glob_size;
static int _next_teamid = 1;
static struct team_impl_struct teams[MAXNUM_TEAMS];

// this should be called by dart_init to initialize the team data
// structures and to set up the default team (consisting of all units)
int dart_teams_init(int myid, int size)
{
	int i;

	_glob_myid = myid;
	_glob_size = size;

	// init teams data structure
	for (i = 0; i < MAXNUM_TEAMS; i++)
	{
		teams[i].id = -1;
		teams[i].state = TEAM_STATE_NOTINIT;
		dart_group_init(&(teams[i].group));
		teams[i].mempool = DART_MEMPOOL_NULL;
		teams[i].barrier_idx = -1;
	}

	// set up the default team
	teams[DART_TEAM_ALL].id = DART_TEAM_ALL;
	for (i = 0; i < size; i++)
	{
		dart_group_addmember(&(teams[DART_TEAM_ALL].group), i);
	}
	teams[DART_TEAM_ALL].state = TEAM_STATE_INIT;
	teams[DART_TEAM_ALL].barrier_idx = 0;  // by convention
	shmif_multicast_init_multicast_group(DART_TEAM_ALL, myid, size);

	return DART_OK;
}

int dart_teams_cleanup(int myid, int size)
{
	int i;
	for (i = 0; i < MAXNUM_TEAMS; i++)
	{
		if (teams[i].id >= 0 && teams[i].state == TEAM_STATE_INIT)
		{
			int group_rank = dart_team_myid(i);
			int group_size = dart_team_size(i);
			shmif_multicast_release_multicast_group(i, group_rank, group_size);
			dart_team_detach_mempool(i);
		}
	}

	return DART_OK;
}

// create a subteam of the specified team, 
int dart_team_create(int superteam, dart_group_t *group)
{
	int newid = _next_teamid;
	_next_teamid++;

	teams[newid].id = newid;
	dart_group_copy(group, &(teams[newid].group));
	teams[newid].state = TEAM_STATE_INIT;
	shmif_multicast_init_multicast_group(newid, newid, group->nmem);

	int super_tid = dart_team_myid(superteam);
	int barrier_index = -1;
	if (super_tid == 0) // someone has to create the barrier...
	{
		barrier_index = shmif_barriers_create_barrier(group->nmem);
		DEBUG("Created new barrier %d for team %d", barrier_index, newid);
	}
	shmif_multicast_bcast(&barrier_index, sizeof(int), 0,
			teams[superteam].barrier_idx, super_tid,
			teams[superteam].group.nmem);
	// TODO: only team-members receive valid result value???
	return newid;
}

int dart_team_myid(int teamid)
{
	int res;

	if (teamid < 0 || teamid >= MAXNUM_TEAMS)
	{
		return DART_ERR_INVAL;
	}
	if (teams[teamid].id < 0 || teams[teamid].state != TEAM_STATE_INIT)
	{
		return DART_ERR_INVAL;
	}

	res = teams[teamid].group.g2l[_glob_myid];
	return res;
}

int dart_team_size(int teamid)
{
	int res;

	if (teamid < 0 || teamid >= MAXNUM_TEAMS)
	{
		return DART_ERR_INVAL;
	}
	if (teams[teamid].id < 0 || teams[teamid].state != TEAM_STATE_INIT)
	{
		return DART_ERR_INVAL;
	}

	res = teams[teamid].group.nmem;
	return res;
}

int dart_team_getgroup(int teamid, dart_group_t *g)
{
	if (teamid < 0 || teamid >= MAXNUM_TEAMS)
	{
		return DART_ERR_INVAL;
	}
	if (teams[teamid].id < 0 || teams[teamid].state != TEAM_STATE_INIT)
	{
		return DART_ERR_INVAL;
	}

	dart_group_copy(&(teams[teamid].group), g);

	return DART_OK;
}

int dart_team_l2g(int teamid, int id)
{
	int ret;

	ret = teams[teamid].group.l2g[id];

	return ret;
}

int dart_team_g2l(int teamid, int id)
{
	int ret;

	ret = teams[teamid].group.g2l[id];

	return ret;
}

dart_mempool dart_team_mempool(int teamid)
{
	return teams[teamid].mempool;
}

dart_mempool create_mempool(int team, int my_team_id, int team_size,
		size_t local_size)
{
	size_t mempoolSize = team_size * local_size; // TODO: alignment: e.g. 0x1000 * sysconf(_SC_PAGESIZE);
	int attach_key;
	if (my_team_id == 0)
		attach_key = shmif_mm_create(mempoolSize);
	int multicast_group = teams[team].barrier_idx;
	shmif_multicast_bcast(&attach_key, sizeof(int), 0, multicast_group,
			my_team_id, team_size);

	void* addr = shmif_mm_attach(attach_key);
	int offset = my_team_id * local_size;
	void* myMempoolBegin = ((char*) addr) + offset;
	dart_mempool mempool = dart_mempool_create(myMempoolBegin, local_size);
	mempool->shm_address = addr;
	mempool->shm_id = attach_key;

	putAddress(team, attach_key, addr);
	DEBUG("create_mempool: at %p of size %u (shm_id: %d)",
			myMempoolBegin, local_size, attach_key);
	return mempool;
}

void destroy_mempool(int team, int my_team_id, dart_mempool mempool)
{
	if (mempool == DART_MEMPOOL_NULL )
		return;
	void* addr = mempool->shm_address;
	int shm_id = mempool->shm_id;
	dart_mempool_destroy(mempool);
	shmif_mm_detach(addr);
	dart_barrier(team);
	if (my_team_id == 0)
		shmif_mm_destroy(shm_id);
	putAddress(team, 0, (void*) NULL );
}

void dart_team_attach_mempool(int teamid, size_t local_size)
{
	int myId = dart_team_myid(teamid);
	int team_size = dart_team_size(teamid);
	destroy_mempool(teamid, myId, teams[teamid].mempool);
	teams[teamid].mempool = create_mempool(teamid, myId, team_size, local_size);
}

void dart_team_detach_mempool(int teamid)
{
	int myId = dart_team_myid(teamid);
	destroy_mempool(teamid, myId, teams[teamid].mempool);
}

int dart_barrier(int team)
{
	int idx = teams[team].barrier_idx;
	if (idx < 0)
		return DART_ERR_OTHER;
	shmif_barriers_barrier_wait(idx);
	return DART_OK;
}

