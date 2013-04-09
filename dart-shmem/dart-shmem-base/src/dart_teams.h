#ifndef DART_TEAMS_H_INCLUDED
#define DART_TEAMS_H_INCLUDED

#include "dart_group.h"
#include "dart_mempool.h"

// maximum number of supported teams
#define MAXNUM_TEAMS 64

#define TEAM_STATE_NOTINIT  1
#define TEAM_STATE_INIT     2

struct team_impl_struct
{
	int id;    // the team id
	int state; // initialized or not

	// the members of the team
	dart_group_t group;
	// associated mempool (may be DART_MEMPOOL_NULL)
	dart_mempool mempool;
	int barrier_idx;
};

int dart_teams_init(int myid, int size);
int dart_teams_cleanup(int myid, int size);
int dart_team_getgroup(int teamid, dart_group_t *g);
int dart_team_create(int teamid, dart_group_t *group);

int dart_team_myid(int teamid);
int dart_team_size(int teamid);

int dart_team_l2g(int teamid, int id);
int dart_team_g2l(int teamid, int id);

void dart_team_attach_mempool(int teamid, size_t local_size);
void dart_team_detach_mempool(int teamid);
dart_mempool dart_team_mempool(int teamid);

int dart_barrier(int team);

#endif /* DART_TEAMS_H_INCLUDED */
