#include <stdio.h>
#include <stddef.h>

#include "shmem_teams.h"
#include "dart_mempool_private.h"
#include "dart_logger.h"
#include "shmem_malloc.h"
#include "shmif_multicast.h"
#include "shmif_barriers.h"
#include "shmif_memory_manager.h"

static int _glob_size;
static int _next_teamid = DART_TEAM_ALL;
static struct team_impl_struct teams[MAXNUM_TEAMS];
typedef struct team_impl_struct team_t;
int teamid_2_unique_teamid[MAXNUM_TEAMS];

static team_t* get_team(int team_id)
{
	int unique_id = teamid_2_unique_teamid[team_id];
	if(unique_id < 0 || unique_id >= MAXNUM_TEAMS)
		return (team_t*)0;
	return &(teams[unique_id]);
}

void* dart_team_memory_segment_begin(int unique_id, int is_aligned)
{
	dart_mempool mempool = teams[unique_id].mempools[(is_aligned) ? 0 : 1];
	if (mempool == DART_MEMPOOL_NULL )
		return (void*) 0;
	return mempool->shm_address;
}

int dart_team_unique_id(int teamid)
{
	return teamid_2_unique_teamid[teamid];
}

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
		teams[i].mempools[0] = DART_MEMPOOL_NULL;
		teams[i].mempools[1] = DART_MEMPOOL_NULL;
		teams[i].unique_id = -1;

		teamid_2_unique_teamid[i] = -1;
	}

	// set up the default team
	dart_group_t all_group;
	dart_group_init(&all_group);
	for (i = 0; i < size; i++)
		dart_group_addmember(&all_group, i);
	dart_team_create(-1, &all_group);

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
			int unique_team_id = teamid_2_unique_teamid[i];
			shmif_multicast_release_multicast_group(unique_team_id, group_rank, group_size);
			dart_team_detach_mempool(i);
		}
	}

	return DART_OK;
}

// create a subteam of the specified team, 
int dart_team_create(int superteam_id, dart_group_t *group)
{
	int newid = _next_teamid;
	_next_teamid++;
	int barrier_index = -1;

	if(superteam_id >= 0)
	{
		team_t* superteam = get_team(superteam_id);
		int super_myid = dart_team_myid(superteam_id);
		if (super_myid == 0) // someone has to create the barrier...
		{
			barrier_index = shmif_barriers_create_barrier(group->nmem);
			DEBUG("Created new barrier %d for team %d", barrier_index, newid);
		}
		shmif_multicast_bcast(&barrier_index, sizeof(int), 0,
				superteam->unique_id, super_myid,
				(superteam->group).nmem);
	}
	else
		barrier_index = 0; // by convention

	teamid_2_unique_teamid[newid] = barrier_index;
	team_t* newteam = get_team(newid);
	newteam->id = newid;
	dart_group_copy(group, &(newteam->group));
	newteam->state = TEAM_STATE_INIT;
	newteam->unique_id = barrier_index;

	int my_team_id = dart_team_myid(newid);
	if(my_team_id >= 0)
	{
		DEBUG("Creating new multicast group %d (team-member %d)", newteam->unique_id, my_team_id);
		shmif_multicast_init_multicast_group(newteam->unique_id, my_team_id, group->nmem);
	}
	dart_barrier((superteam_id >= 0)? superteam_id : DART_TEAM_ALL);

	return newid;
}

int dart_team_myid(int teamid)
{
	int res;
	if (teamid < 0 || teamid >= MAXNUM_TEAMS)
		return DART_ERR_INVAL;
	team_t* team = get_team(teamid);
	if (team->id < 0 || team->state != TEAM_STATE_INIT)
		return DART_ERR_INVAL;

	res = (team->group).g2l[_glob_myid];
	return res;
}

int dart_team_size(int teamid)
{
	if (teamid < 0 || teamid >= MAXNUM_TEAMS)
		return DART_ERR_INVAL;
	team_t* team = get_team(teamid);
	if (team->id < 0 || team->state != TEAM_STATE_INIT)
		return DART_ERR_INVAL;

	return (team->group).nmem;
}

int dart_myid()
{
	return dart_team_myid(DART_TEAM_ALL);
}

int dart_size()
{
	return dart_team_size(DART_TEAM_ALL);
}

int dart_team_getgroup(int teamid, dart_group_t *g)
{
	if (teamid < 0 || teamid >= MAXNUM_TEAMS)
		return DART_ERR_INVAL;
	team_t* team = get_team(teamid);
	if (team->id < 0 || team->state != TEAM_STATE_INIT)
		return DART_ERR_INVAL;

	dart_group_copy(&(team->group), g);
	return DART_OK;
}

int dart_team_l2g(int teamid, int id)
{
	int ret;
	ret = (get_team(teamid)->group).l2g[id];
	return ret;
}

int dart_team_g2l(int teamid, int id)
{
	int ret;
	ret = (get_team(teamid))->group.g2l[id];
	return ret;
}

dart_mempool dart_team_mempool_aligned(int teamid)
{
	return (get_team(teamid))->mempools[0];
}

dart_mempool dart_team_mempool_nonAligned(int teamid)
{
	return (get_team(teamid))->mempools[1];
}

void create_mempools(int teamid, int my_team_id, int team_size,
		size_t local_size)
{
	team_t* team = get_team(teamid);
	size_t mempoolSize = team_size * local_size; // TODO: alignment: e.g. 0x1000 * sysconf(_SC_PAGESIZE);
	int attach_key[2];
	if (my_team_id == 0)
	{
		attach_key[0] = shmif_mm_create(mempoolSize);
		attach_key[1] = shmif_mm_create(mempoolSize);
	}
	shmif_multicast_bcast(&attach_key, 2*sizeof(int), 0, team->unique_id,
			my_team_id, team_size);

	int offset = my_team_id * local_size;
	void* addr[2];
	addr[0] = shmif_mm_attach(attach_key[0]);
	addr[1] = shmif_mm_attach(attach_key[1]);
	team->mempools[0] = dart_mempool_create(((char*) addr[0]) + offset, local_size);
	team->mempools[1] = dart_mempool_create(((char*) addr[1]) + offset, local_size);
	team->mempools[0]->shm_address = addr[0];
	team->mempools[1]->shm_address = addr[1];
	team->mempools[0]->shm_id = attach_key[0];
	team->mempools[1]->shm_id = attach_key[1];

	DEBUG("create_mempools: at %p of size %u (shm_id: %d), offset: %d", addr[0], local_size, attach_key[0], offset);
	DEBUG("create_mempools: at %p of size %u (shm_id: %d), offset: %d", addr[1], local_size, attach_key[1], offset);
}

void destroy_mempool(int team, int my_team_id, dart_mempool mempool)
{
	if (mempool == DART_MEMPOOL_NULL )
		return;
	void* addr = mempool->shm_address;
	int shm_id = mempool->shm_id;
	DEBUG("destroy_mempools: at %p  (shm_id: %d)", addr, shm_id);
	dart_mempool_destroy(mempool);
	shmif_mm_detach(addr);
	dart_barrier(team);
	if (my_team_id == 0)
		shmif_mm_destroy(shm_id);
}

void dart_team_attach_mempool(int teamid, size_t local_size)
{
	int myId = dart_team_myid(teamid);
	int team_size = dart_team_size(teamid);
	team_t* team = get_team(teamid);
	destroy_mempool(teamid, myId, team->mempools[0]);
	destroy_mempool(teamid, myId, team->mempools[1]);
	create_mempools(teamid, myId, team_size, local_size);
}

void dart_team_detach_mempool(int teamid)
{
	int myId = dart_team_myid(teamid);
	team_t* team = get_team(teamid);
	destroy_mempool(teamid, myId, team->mempools[0]);
	destroy_mempool(teamid, myId, team->mempools[1]);
	team->mempools[0] = DART_MEMPOOL_NULL;
	team->mempools[1] = DART_MEMPOOL_NULL;
}

int dart_barrier(int teamid)
{
	team_t* team = get_team(teamid);
	int idx = team->unique_id;
	if (idx < 0)
		return DART_ERR_OTHER;
	shmif_barriers_barrier_wait(idx);
	return DART_OK;
}

