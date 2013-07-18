#ifndef DART_TEAMS_H_INCLUDED
#define DART_TEAMS_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#define DART_INTERFACE_ON

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
  
  - Each member of the new team will receive the same numerical team
    ID.

  - The team ID of the returned team will be unique with respect to
    the parent team
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

#endif /* DART_TEAMS_H_INCLUDED */
