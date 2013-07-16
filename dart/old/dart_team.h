/*
 * dart_team.h
 *
 *  Created on: Apr 9, 2013
 *      Author: maierm
 */

#ifndef DART_TEAM_H_
#define DART_TEAM_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "dart_group.h"

int dart_teams_init(int myid, int size);
int dart_teams_cleanup(int myid, int size);
int dart_team_getgroup(int teamid, dart_group_t *g);

/* create a team from the specified group */
/* this is a collective call, all members of team have to call the */
/* function with an equivalent specification of group */
// Q: is the team id globally unique??
int dart_team_create(int teamid, dart_group_t *group);

int dart_team_myid(int teamid);
int dart_team_size(int teamid);
int dart_myid();
int dart_size();

/*
 barrier of all members of the team, usual semantics as in MPI
 we could think about including a split-phase barrier as well...
 */
int dart_barrier(int team);

#ifdef __cplusplus
}
#endif

#endif /* DART_TEAM_H_ */
