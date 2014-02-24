#ifndef DART_TEAM_GROUP_H_INCLUDED
#define DART_TEAM_GROUP_H_INCLUDED

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


/* split the group into n groups of approx. same size,
   gout must be an array of dart_group_t objects of size at least n
*/
dart_ret_t dart_group_split(const dart_group_t *g, size_t n, 
			    dart_group_t *gout);


/* get the size of the opaque object */
dart_ret_t dart_group_sizeof(size_t *size);


/* the default team consisting of all units that comprise
   the program */
#define DART_TEAM_ALL  ((dart_team_t)0)


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
dart_ret_t dart_team_create(dart_team_t teamid, const dart_group_t *group, 
			    dart_team_t *newteam); 

/* Free up resources associated with the specified team */
dart_ret_t dart_team_destroy(dart_team_t teamid);

/* return the unit id of the caller in the specified team */
dart_ret_t dart_team_myid(dart_team_t teamid, dart_unit_t *myid);

/* return the size of the specified team */
dart_ret_t dart_team_size(dart_team_t teamid, size_t *size);

/* shorthand for id and size in the default team DART_TEAM_ALL */
dart_ret_t dart_myid(dart_unit_t *myid);
dart_ret_t dart_size(size_t *size);


#define DART_INTERFACE_OFF

#ifdef __cplusplus
}
#endif

#endif /* DART_TEAM_GROUP_H_INCLUDED */
