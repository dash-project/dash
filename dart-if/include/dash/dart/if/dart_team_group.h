#ifndef DART_TEAM_GROUP_H_INCLUDED
#define DART_TEAM_GROUP_H_INCLUDED

#include "dart_types.h"


/**
 * \file dart_team_group.h
 * \defgroup  DartGroupTeam    Management of groups and teams
 * \ingroup   DartInterface
 *
 * Routines for managing groups of units and to form teams.
 *
 * DART groups are objects with local meaning only. They are
 * essentially objects representing sets of units, out of which later
 * teams can be formed. The operations to manipulate groups are local
 * (and cheap).  The operations to create teams are collective and can
 * be expensive.
 *
 * CLARIFICATION: A group does need to keep it's member units in a
 * *ascending* order, i.e., a call to \ref dart_group_getmembers will
 * return the list of member units in ascending order. Similarly, a
 * call to \ref dart_group_split will split the group according to an
 * ascending ordering of the member units.
 *
 * CLARIFICATION: Groups and teams interact in two ways. First, when a
 * team is created and a group specification is passed in. Second,
 * through the call \ref dart_team_get_group, where the group associated
 * with the team can be derived. In both cases, the group *always*
 * contains the global Unit IDs, i.e., the unit IDs relative to
 * \ref DART_TEAM_ALL.
 *
 */


#ifdef __cplusplus
extern "C" {
#endif

/** \cond DART_HIDDEN_SYMBOLS */
#define DART_INTERFACE_ON
/** \endcond */

/**
 * \name Group management operations
 * Non-collectove operations to create, destroy, and manipulate teams.
 *
 * Note that \c dart_group_t is an opaque datastructure that is allocated by all
 * functions creating a group (marked as \c [out]).
 * This memory has to be released by calling \ref dart_group_destroy after use.
 */

/** \{ */

/**
 * DART groups are represented by an opaque struct \c dart_group_struct
 *
 * \ingroup DartGroupTeam
 */
typedef struct dart_group_struct* dart_group_t;


/**
 * Allocate and initialize a DART group object.
 * Must be called before any other function on the group object.
 *
 * \param[out] group Pointer to a group to be created.
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \threadsafe
 * \ingroup DartGroupTeam
 */
dart_ret_t dart_group_create(dart_group_t *group);

/**
 * Reclaim resources that might be associated with the group.
 *
 * \param group Pointer to a group to be finalized.
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \threadsafe
 * \ingroup DartGroupTeam
 */
dart_ret_t dart_group_destroy(dart_group_t *group);


/**
 * Create a copy of the group \c gin, allocating resources for \c gout.
 *
 * \param      gin  Pointer to a group to be copied.
 * \param[out] gout Pointer to the target group object (will be allocated).
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \threadsafe
 * \ingroup DartGroupTeam
 */
dart_ret_t dart_group_clone(const dart_group_t   gin,
                            dart_group_t       * gout);


/**
 * Create a union of two groups
 *
 * \param      g1   Pointer to the first group to join.
 * \param      g2   Pointer to the second group to join.
 * \param[out] gout Pointer to the target group object (will be allocated).
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \threadsafe_none
 * \ingroup DartGroupTeam
 */
dart_ret_t dart_group_union(const dart_group_t   g1,
                            const dart_group_t   g2,
                            dart_group_t       * gout);


/**
 * Create an intersection of the two groups
 *
 * \param      g1   Pointer to the first group to intersect.
 * \param      g2   Pointer to the second group to intersect.
 * \param[out] gout Pointer to the target group object (will be allocated).
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \threadsafe_none
 * \ingroup DartGroupTeam
 */
dart_ret_t dart_group_intersect(const dart_group_t   g1,
                                const dart_group_t   g2,
                                dart_group_t       * gout);


/**
 * Add a member to the group.
 *
 * \param g      Pointer to the target group object.
 * \param unitid Unit to add to group \c g.
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \threadsafe_none
 * \ingroup DartGroupTeam
 */
dart_ret_t dart_group_addmember(dart_group_t       g,
                                dart_global_unit_t unitid);


/**
 * Remove a member from the group.
 *
 * \param unitid Unit to remove from group \c g.
 * \param g      Pointer to the target group object.
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \threadsafe_none
 * \ingroup DartGroupTeam
 */
dart_ret_t dart_group_delmember(dart_group_t        g,
                                dart_global_unit_t  unitid);


/**
 * Test if a unit is a member of the group.
 *
 * \param unitid Unit to test in group \c g.
 * \param g      Pointer to the target group object.
 * \param[out] ismember  True if \c unitid is member of group \c g, false otherwise.
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \threadsafe_none
 * \ingroup DartGroupTeam
 */
dart_ret_t dart_group_ismember(const dart_group_t   g,
                               dart_global_unit_t   unitid,
                               int32_t            * ismember);


/**
 * Determine the size of the group.
 *
 * \param g         Pointer to the group object.
 * \param[out] size The number of units in the group.
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \threadsafe_none
 * \ingroup DartGroupTeam
 */
dart_ret_t dart_group_size(const dart_group_t   g,
                           size_t             * size);


/**
 * Get all the members of the group, \c unitids must be large enough
 * to hold the number of members returned by \ref dart_group_size.
 *
 * \param       g        Pointer to the group object.
 * \param[out]  unitids  An array large enough to hold the number of units
 *                       as returned by \ref dart_group_size.
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \threadsafe_none
 * \ingroup DartGroupTeam
 */
dart_ret_t dart_group_getmembers(const dart_group_t   g,
                                 dart_global_unit_t * unitids);


/**
 * Split the group into \c n groups of approx. the same size,
 * \c gout must be an array of \c dart_group_t objects of size at least \c n.
 *
 * \param       g      Pointer to the group object.
 * \param       n      The requested number of groups to split \c g into.
 * \param[out]  nout   The actual number of groups that \c g has been split
 *                     into.
 * \param[out]  gout   An array of at least \c n pointers to the opaque
 *                     \ref dart_group_t (the first \c nout objects will be allocated).
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \threadsafe_none
 * \ingroup DartGroupTeam
 */
dart_ret_t dart_group_split(const dart_group_t    g,
                            size_t                n,
                            size_t              * nout,
                            dart_group_t        * gout);

/**
 * Split the group \c g into \c n groups by the specified locality scope.
 * For example, a locality split in socket scope creates at least one new
 * group for every socket containing all units in the original group that
 * have affinity with the respective socket.
 * Size of array \c gout must have a capacity of at least \c n
 * \c dart_group_t objects.
 *
 * \param g           Pointer to the group object.
 * \param domain      The domain to use for the split.
 * \param scope       The scope to use for the split.
 * \param n           The requested number of groups to split \c g into.
 * \param[out] nout   The actual number of groups that \c g has been split
 *                    into.
 * \param[out] gout   An array of at least \c n pointers to the opaque
 *                    \ref dart_group_t (the first \c nout will be allocated).
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \threadsafe_none
 * \ingroup DartGroupTeam
*/
dart_ret_t dart_group_locality_split(const dart_group_t        g,
                                     dart_domain_locality_t  * domain,
                                     dart_locality_scope_t     scope,
                                     size_t                    n,
                                     size_t                  * nout,
                                     dart_group_t            * gout);

/** \} */

/**
 * \name Team management operations
 * Operations to create, destroy, and query team information.
 *
 * Teams are created based on DART groups.
 *
 * Note that team creation and destruction are collective operations.
 *
 * Functions returning DART groups allocate these opaque objects, which
 * then have to be destroyed by the user using \ref dart_group_destroy.
 */

/** \{ */

/**
 * The default team consisting of all units
 * that run the application.
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 * \ingroup DartGroupTeam
 */
#define DART_TEAM_ALL   ((dart_team_t)0)
/** \cond DART_HIDDEN_SYMBOLS */
#define DART_TEAM_NULL  ((dart_team_t)-1)

#define DART_GROUP_NULL ((struct dart_group_struct*)NULL)
/** \endcond */


/**
 * Query the group associated with the specified team
 *
 * \param      teamid The team to use.
 * \param[out] group  Pointer to a group object (will be allocated).
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \threadsafe_none
 * \ingroup DartGroupTeam
 */
dart_ret_t dart_team_get_group(dart_team_t teamid, dart_group_t *group);


/**
 * Create a new team from the specified group
 *
 * This is a collective call: All members of the parent team have to
 * call this function with an equivalent specification of the new team
 * to be formed (even those that do not participate in the new
 * team). Units not participating in the new team may pass a null
 * pointer for the group specification.
 *
 * The returned integer team ID does *not need* to be globally unique.
 *
 * However, the following guarantees are made:
 *
 * 1) Each member of the new team will receive the same numerical team ID.
 * 2) The team ID of the returned team will be unique with respect to the parent team.
 * 3) If a unit is part of several teams, all these teams will have different team IDs
 *
 * Example:
 *
 * DART_TEAM_ALL: 0, 1, 2, 3, 4, 5, 6, 7, 8, 9
 *
 * Form two sub-teams of equal size (0-4, 5-9):
 *
 * dart_team_create(DART_TEAM_ALL, {0,1,2,3,4}) -> TeamID=1
 * dart_team_create(DART_TEAM_ALL, {5,6,7,8,9}) -> TeamID=2
 *
 * (1,2 are unique ID's with respect to the parent team (DART_TEAM_ALL)
 *
 * Build further sub-teams:
 *
 * dart_team_create(1, {0,1,2}) -> TeamID=2
 * dart_team_create(1, {3,4})   -> TeamID=3
 *
 * (2,3 are unique with respect to the parent team (1)).
 *
 * \param teamid The parent team to use whose units participate in the collective operation.
 * \param group  The group object to build the new team from.
 * \param[out] newteam Will contain the new team ID upon successful return.
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \threadsafe_none
 * \ingroup DartGroupTeam
 */
dart_ret_t dart_team_create(dart_team_t          teamid,
                            const dart_group_t   group,
                            dart_team_t        * newteam);

/**
 * Free up resources associated with the specified team
 *
 * \param teamid The team to deallocate.
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \threadsafe_none
 * \ingroup DartGroupTeam
 */
dart_ret_t dart_team_destroy(dart_team_t * teamid);


/**
 * Clone a DART team object by duplicating the underlying team information.
 *
 * \param      team     The source team to duplicate
 * \param[out] newteam  The target team to duplicate to.
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 * \threadsafe_none
 * \ingroup DartGroupTeam
 */
dart_ret_t dart_team_clone(dart_team_t team, dart_team_t *newteam);

/**
 * Return the unit id of the caller in the specified team.
 *
 * CLARIFICATION on dart_team_myid():
 *
 * dart_team_myid(team) returns the relative ID for the calling unit
 * in the specified team( [0...n-1] , where n is the size of team n)
 *
 * The following guarantees are made with respect to the relationship
 * between the global IDs and the local IDs.
 *
 * Consider the following example:
 *
 * DART_TEAM_ALL = {0,1,2,3,4,5}
 *
 * t1 = dart_team_create(DART_TEAM_ALL, {4,2,0}}
 *
 * Global ID | ID in t1 (V1)    | ID in t1 (V2)
 * ---------------------------------------------
 * 0         | 0                | 2
 * 1         | not a member     | not a member
 * 2         | 1                | 1
 * 3         | not a member     | not a member
 * 4         | 2                | 0
 * 5         | not a member     | not a member
 *
 * The order as in V1 is guaranteed (I.e., the unit with ID 0 is the
 * member with the smallest global ID, regardless of the order in
 * which the members are specified in the group spec.
 *
 * RATIONALE: SPMD code often diverges based on rank/unit ID. It will
 * be useful to know the new master (local ID 0) of a newly created
 * team before actually creating it.x
 *
 * \param teamid The team for which the unit ID should be determined.
 * \param[out] myid The unit ID of the calling unit in the respective team.
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \threadsafe_none
 * \ingroup DartGroupTeam
 */
dart_ret_t dart_team_myid(dart_team_t teamid, dart_team_unit_t *myid);

/**
 * Return the size of the specified team.
 *
 * \param teamid The team for which the size should  be determined.
 * \param[out] size The size of the team.
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \threadsafe_none
 * \ingroup DartGroupTeam
 */
dart_ret_t dart_team_size(dart_team_t teamid, size_t *size);

/**
 * Return the id in the default team \ref DART_TEAM_ALL
 *
 * \param[out] myid The global unit ID of the calling unit.
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \threadsafe_none
 * \ingroup DartGroupTeam
 */
dart_ret_t dart_myid(dart_global_unit_t *myid);

/**
 * Return the size of the default team \ref DART_TEAM_ALL
 *
 * \param[out] size The size of the team.
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \threadsafe_none
 * \ingroup DartGroupTeam
 */
dart_ret_t dart_size(size_t *size);


/**
 * Convert from a local to a global unit ID
 *
 * \c local means the ID with respect to the specified team whereas
 * \c global means the ID with respect to \ref DART_TEAM_ALL
 *
 * This call is *not collective* on the specified team.
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \threadsafe_none
 * \ingroup DartGroupTeam
 */
dart_ret_t dart_team_unit_l2g(dart_team_t          team,
                              dart_team_unit_t     localid,
                              dart_global_unit_t * globalid);

/**
 * Convert from a global to a local unit ID
 *
 * \c local means the ID with respect to the specified team whereas
 * \c global means the ID with respect to \ref DART_TEAM_ALL.
 *
 * This call is *not collective* on the specified team.
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \threadsafe_none
 * \ingroup DartGroupTeam
 */
dart_ret_t dart_team_unit_g2l(dart_team_t         team,
                              dart_global_unit_t  globalid,
                              dart_team_unit_t * localid);

/** \} */


/** \cond DART_HIDDEN_SYMBOLS */
#define DART_INTERFACE_OFF
/** \endcond */

#ifdef __cplusplus
}
#endif

#endif /* DART_TEAM_GROUP_H_INCLUDED */
