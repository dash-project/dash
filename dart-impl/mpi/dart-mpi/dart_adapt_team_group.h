/** @file dart_adapt_team_group.h
 *  @date 25 Mar 2014
 *  @brief Function prototypes for dart operations on team&group. 
 *
 *  Method (steps) of judging if certain unit is in the specified team or not:
 *  - The teamid is initialized as "DART_TEAM_NULL".
 *  - If (units belonging to this team) {teamid = xx} else {do nothing} (by means of MPI).
 *  - If (teamid != DART_TEAM_NULL) {enter then operate across this team}.
 */

#ifndef DART_ADAPT_TEAM_GROUP_H_INCLUDED
#define DART_ADAPT_TEAM_GROUP_H_INCLUDED

#include "dart_deb_log.h"
#include "dart_adapt_group_priv.h"
#ifdef __cplusplus
extern "C" {
#endif
  
#define DART_INTERFACE_ON

extern int next_availteamid;

dart_ret_t dart_adapt_group_init (dart_group_t *group);

dart_ret_t dart_adapt_group_fini (dart_group_t *group);

dart_ret_t dart_adapt_group_copy (const dart_group_t *gin, 
		dart_group_t *gout);

/* The ordering of the processes in gout is identical with the ordering in g1 instead of g2,
 * this union operation just behaves like appending g2 to g1 and skipping the repeated processes.
 * Fence, in such a case a group in dart can be in any order. 
 */
dart_ret_t dart_adapt_group_union (const dart_group_t *g1,
		const dart_group_t *g2,
                dart_group_t *gout);

/* This union operation does not merely append g2 to g1, it will sort gout up at the end so that gout
 * is arranged in an increasing order based on the global unit id. Fence, in such a case a group
 * in dart should be ordered. 
 */
dart_ret_t dart_adapt_group_union (const dart_group_t *g1,
		const dart_group_t *g2,
		dart_group_t *gout);

dart_ret_t dart_adapt_group_intersect(const dart_group_t *g1,
                const dart_group_t *g2,
                dart_group_t *gout);

dart_ret_t dart_adapt_group_addmember (dart_group_t *g, dart_unit_t unitid);

dart_ret_t dart_adapt_group_delmember (dart_group_t *g, dart_unit_t unitid);


dart_ret_t dart_adapt_group_ismember (const dart_group_t *g, dart_unit_t unitid, int32_t *ismember);

dart_ret_t dart_adapt_group_size (const dart_group_t *g, size_t *size);


dart_ret_t dart_adapt_group_getmembers (const dart_group_t *g, dart_unit_t *unitids);


dart_ret_t dart_adapt_group_split (const dart_group_t *g, size_t n, dart_group_t *gout);


/* Get the size of the opaque object. */
dart_ret_t dart_adapt_group_sizeof (size_t *size);


/* Get the group associated with the specified team. */
dart_ret_t dart_adapt_team_get_group (dart_team_t teamid, dart_group_t *group);

/** @brief create sub-team out of group.
 *
 *  @param[in] teamid	Parent teamid.
 *  @param[in] group	Sub-group.
 *  @param[out] newteam	new generated sub-team.
 *  @return		Dart status.
 */
dart_ret_t dart_adapt_team_create (dart_team_t teamid, const dart_group_t *group, dart_team_t *newteam); 

/* Free up resources associated with the specified team. */
dart_ret_t dart_adapt_team_destroy (dart_team_t teamid);

/* Return the unit id of the caller in the specified team. */
dart_ret_t dart_adapt_team_myid (dart_team_t teamid, dart_unit_t *myid);

/* Return the size of the specified team. */
dart_ret_t dart_adapt_team_size (dart_team_t teamid, size_t *size);

/* Shorthand for id and size in the default team DART_TEAM_ALL. */
dart_ret_t dart_adapt_myid (dart_unit_t *myid);
dart_ret_t dart_adapt_size (size_t *size);

dart_ret_t dart_adapt_team_unit_l2g (dart_team_t teamid, dart_unit_t localid, dart_unit_t *globalid);
dart_ret_t dart_adapt_team_unit_g2l (dart_team_t teamid, dart_unit_t globalid, dart_unit_t *localid);


#define DART_INTERFACE_OFF

#ifdef __cplusplus
}
#endif

#endif /* DART_TEAM_GROUP_H_INCLUDED */
