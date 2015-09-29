/** @file dart_adapt_team_group.h
 *  @date 21 Nov 2013
 *  @brief Function prototypes for dart operations on team&group. 
 */

#ifndef DART_ADAPT_TEAM_GROUP_H_INCLUDED
#define DART_ADAPT_TEAM_GROUP_H_INCLUDED

#include <mpi.h>
#include "dart_deb_log.h"
#include "dart_team_group.h"
#ifdef __cplusplus
extern "C" {
#endif
  
#define DART_INTERFACE_ON

/** @brief Dart group type.
 */
struct dart_group_struct{
	MPI_Group mpi_group;
};

dart_ret_t dart_adapt_group_init(dart_group_t *group);

dart_ret_t dart_adapt_group_fini(dart_group_t *group);

dart_ret_t dart_adapt_group_copy(const dart_group_t *gin, 
			   dart_group_t *gout);


dart_ret_t dart_adapt_group_union(const dart_group_t *g1,
                            const dart_group_t *g2,
                            dart_group_t *gout);


dart_ret_t dart_adapt_group_intersect(const dart_group_t *g1,
                                const dart_group_t *g2,
                                dart_group_t *gout);

dart_ret_t dart_adapt_group_addmember(dart_group_t *g, dart_unit_t unitid);

dart_ret_t dart_adapt_group_delmember(dart_group_t *g, dart_unit_t unitid);


dart_ret_t dart_adapt_group_ismember(const dart_group_t *g, dart_unit_t unitid, int32_t *ismember);

dart_ret_t dart_adapt_group_size(const dart_group_t *g, size_t *size);


dart_ret_t dart_adapt_group_getmembers(const dart_group_t *g, dart_unit_t *unitids);


dart_ret_t dart_adapt_group_split(const dart_group_t *g, size_t n, dart_group_t *gout);


/* Get the size of the opaque object. */
dart_ret_t dart_adapt_group_sizeof(size_t *size);


/* Get the group associated with the specified team. */
dart_ret_t dart_adapt_team_get_group(dart_team_t teamid, dart_group_t *group);

/** @brief create sub-team out of group.
 *
 *  @param[in] teamid	Parent teamid.
 *  @param[in] group	Sub-group.
 *  @param[out] newteam	new generated sub-team.
 *  @return		Dart status.
 *
 *  The returned sub-team is the child of its parent team given by teamid
 *  from the perspective of team hierarchy.
 */
dart_ret_t dart_adapt_team_create(dart_team_t teamid, const dart_group_t *group, dart_team_t *newteam); 

/* Free up resources associated with the specified team. */
dart_ret_t dart_adapt_team_destroy(dart_team_t teamid);

/* Return the unit id of the caller in the specified team. */
dart_ret_t dart_adapt_team_myid(dart_team_t teamid, dart_unit_t *myid);

/* Return the size of the specified team. */
dart_ret_t dart_adapt_team_size(dart_team_t teamid, size_t *size);

/* Shorthand for id and size in the default team DART_TEAM_ALL. */
dart_ret_t dart_adapt_myid(dart_unit_t *myid);
dart_ret_t dart_adapt_size(size_t *size);


#define DART_INTERFACE_OFF

#ifdef __cplusplus
}
#endif

#endif /* DART_TEAM_GROUP_H_INCLUDED */
