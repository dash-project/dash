#ifndef DART_TEAM_GROUP_H_INCLUDED
#define DART_TEAM_GROUP_H_INCLUDED

#include "dart_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DART_INTERFACE_ON

/*
  DART groups are objects with local meaning only. They are
  essentially objects representing sets of units, out of which later
  teams can be formed. The operations to manipulate groups are local
  (and cheap).  The operations to create teams are collective and can
  be expensive.

  CLARIFICATION: A group does need to keep it's member units in a
  *ascending* order. I.e., a call to dart_group_getmembers() will
  return the list of member units in ascending order. Similarly, a
  call to dart_group_split() will split the group according to an
  ascending ordering of the member units.

  CLARIFICATION: Groups and teams interact in two ways. First, when a
  team is created and a group specification is passed in. Second,
  through the call dart_team_get_group(), where the group associated
  with the team can be derived. In both cases, the group *always*
  contains the global Unit IDs (i.e., the unit IDs relative to
  DART_TEAM_ALL).

 */

/* DART groups are represented by an opaque struct dart_group_struct */
typedef struct dart_group_struct dart_group_t;


/*
  dart_group_init must be called before any other function on the
  group object, dart_group_fini reclaims resources that might be
  associated with the group (if any).
*/
dart_ret_t dart_group_init(dart_group_t *group);
dart_ret_t dart_group_fini(dart_group_t *group);


/* make a copy of the group */
dart_ret_t dart_group_copy(const dart_group_t *gin,
                           dart_group_t *gout);


/* set union */
dart_ret_t dart_group_union(const dart_group_t *g1,
                            const dart_group_t *g2,
                            dart_group_t *gout);


/* set intersection */
dart_ret_t dart_group_intersect(const dart_group_t *g1,
                                const dart_group_t *g2,
                                dart_group_t *gout);


/* add a member */
dart_ret_t dart_group_addmember(dart_group_t *g, dart_unit_t unitid);


/* delete a member */
dart_ret_t dart_group_delmember(dart_group_t *g, dart_unit_t unitid);


/* test if unitid is a member */
dart_ret_t dart_group_ismember(const dart_group_t *g,
                               dart_unit_t unitid, int32_t *ismember);


/* determine the size of the group */
dart_ret_t dart_group_size(const dart_group_t *g,
                           size_t *size);


/* get all the members of the group, unitids must be large enough
   to hold dart_group_size() members */
dart_ret_t dart_group_getmembers(const dart_group_t *g,
                                 dart_unit_t *unitids);


/* split the group into \c n groups of approx. same size,
   \c gout must be an array of \c dart_group_t objects of size at least \c n.
*/
dart_ret_t dart_group_split(const dart_group_t  * g,
                            size_t                n,
                            dart_group_t       ** gout);

/* split the group \c g into \c n groups by the specified locality scope.
   For example, a locality split in socket scope creates at least one new
   group for every socket containing all units in the original group that
   have affinity with the respective socket.
   Size of array \c gout must have a capacity of at least \c n
   \c dart_group_t objects.
*/
dart_ret_t dart_group_locality_split(const dart_group_t      * g,
                                     dart_locality_scope_t     scope,
                                     size_t                    n,
                                     dart_group_t           ** gout);

/* get the size of the opaque object */
dart_ret_t dart_group_sizeof(size_t *size);


/* the default team consisting of all units that comprise
   the program */
#define DART_TEAM_ALL   ((dart_team_t)0)
#define DART_TEAM_NULL  ((dart_team_t)-1)


/* get the group associated with the specified team */
dart_ret_t dart_team_get_group(dart_team_t teamid, dart_group_t *group);


/*
  Create a new team from the specified group

  This is a collective call: All members of the parent team have to
  call this function with an equivalent specification of the new team
  to be formed (even those that do not participate in the new
  team). Units not participating in the new team may pass a null
  pointer for the group specification.

  The returned integer team ID does *not need* to be globally unique.

  However, the following guarantees are made:

  1) Each member of the new team will receive the same numerical team
     ID.

  2) The team ID of the returned team will be unique with respect to
    the parent team.

  3) If a unit is part of several teams, all these teams will have
    different team IDs

  Example:

  DART_TEAM_ALL: 0, 1, 2, 3, 4, 5, 6, 7, 8, 9

  Form two sub-teams of equal size (0-4, 5-9):

  dart_team_create(DART_TEAM_ALL, {0,1,2,3,4}) -> TeamID=1
  dart_team_create(DART_TEAM_ALL, {5,6,7,8,9}) -> TeamID=2

  (1,2 are unique ID's with respect to the parent team (DART_TEAM_ALL)

  Build further sub-teams:

  dart_team_create(1, {0,1,2}) -> TeamID=2
  dart_team_create(1, {3,4})   -> TeamID=3

  (2,3 are unique with respect to the parent team (1)).
 */
dart_ret_t dart_team_create(dart_team_t          teamid,
                            const dart_group_t * group,
                            dart_team_t        * newteam);

/* Free up resources associated with the specified team */
dart_ret_t dart_team_destroy(dart_team_t teamid);

/* return the unit id of the caller in the specified team */
dart_ret_t dart_team_myid(dart_team_t teamid, dart_unit_t *myid);

/* CLARIFICATION on dart_team_myid():

   dart_team_myid(team) returns the relative ID for the calling unit
   in the specified team( [0...n-1] , where n is the size of team n)

   The following guarantees are made with respect to the relationship
   between the global IDs and the local IDs.

   Consider the following example:

   DART_TEAM_ALL = {0,1,2,3,4,5}

   t1 = dart_team_create(DART_TEAM_ALL, {4,2,0}}

   Global ID | ID in t1 (V1)    | ID in t1 (V2)
   ---------------------------------------------
   0         | 0                | 2
   1         | not a member     | not a member
   2         | 1                | 1
   3         | not a member     | not a member
   4         | 2                | 0
   5         | not a member     | not a member

   The order as in V1 is guaranteed (I.e., the unit with ID 0 is the
   member with the smallest global ID, regardless of the order in
   which the members are specified in the group spec.

   RATIONALE: SPMD code often diverges based on rank/unit ID. It will
   be useful to know the new master (local ID 0) of a newly created
   team before actually creating it.x

*/

/* return the size of the specified team */
dart_ret_t dart_team_size(dart_team_t teamid, size_t *size);

/* shorthand for id and size in the default team DART_TEAM_ALL */
dart_ret_t dart_myid(dart_unit_t *myid);
dart_ret_t dart_size(size_t *size);


/* convert between local and global unit IDs

   local means the ID with respect to the specified team
   global means the ID with respect to DART_TEAM_ALL

   these calls are *not* collective calls on the specified teams
*/
dart_ret_t dart_team_unit_l2g(dart_team_t team,
                              dart_unit_t localid,
                              dart_unit_t *globalid);

dart_ret_t dart_team_unit_g2l(dart_team_t team,
                              dart_unit_t globalid,
                              dart_unit_t *localid);



#define DART_INTERFACE_OFF

#ifdef __cplusplus
}
#endif

#endif /* DART_TEAM_GROUP_H_INCLUDED */
