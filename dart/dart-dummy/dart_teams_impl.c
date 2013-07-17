
#include "dart_types.h"
#include "dart_groups_impl.h"
#include "dart_teams.h"

#include <stdio.h>

dart_ret_t dart_team_create(dart_team_t teamid, const dart_group_t *group, 
			    dart_team_t *newteam)
{
  fprintf(stderr, "In dart_team_create: the groups object is %zu bytes big\n", 
	  sizeof(dart_group_t));
  *newteam = 33;

  return DART_OK;
}

dart_ret_t dart_team_destroy(dart_team_t teamid)
{
  return DART_OK;
}

dart_ret_t dart_team_myid(dart_team_t teamid, dart_unit_t *myid)
{
  *myid=3;
  return DART_OK;
}

dart_ret_t dart_team_size(dart_team_t teamid, size_t *size)
{
  *size=8;
  return DART_OK;
}

dart_ret_t dart_myid(dart_unit_t *myid)
{
  *myid=3;
  return DART_OK;
}

dart_ret_t dart_size(size_t *size)
{
  *size=8;
  return DART_OK;
}
