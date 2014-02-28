#include <stdio.h>
#include "dart_types.h"
#include "dart_team_group.h"

#include "adapt/dart_adapt_synchronization.h"
#include "adapt/dart_adapt_initialization.h"
#include "adapt/dart_adapt_globmem.h"
#include "adapt/dart_adapt_team_group.h"
#include "adapt/dart_adapt_communication.h"


dart_ret_t dart_group_init (dart_group_t *group)
{
		   return  dart_adapt_group_init (group);
}
dart_ret_t dart_group_fini (dart_group_t *group)
{
           return dart_adapt_group_fini (group);
}
/*dart_ret_t dart_group_copy (dart_group_t gin, dart_group_t gout)
{
           return dart_adapt_group_copy(gin, gout);
}
dart_ret_t dart_group_union (dart_group_t g1, dart_group_t g2, dart_group_t gout)
{
	       return dart_adapt_group_union (g1, g2, gout);
}
dart_ret_t dart_group_intersect (dart_group_t g1, dart_group_t g2, dart_group_t gout)
{
		   return dart_adapt_group_intersect (g1, g2, gout);
}*/
dart_ret_t dart_group_addmember(dart_group_t *g, dart_unit_t unitid)
{
           return dart_adapt_group_addmember(g, unitid);
}
dart_ret_t dart_group_delmember(dart_group_t *g, dart_unit_t unitid)
{
           return dart_adapt_group_delmember(g, unitid);
}
/*dart_ret_t dart_group_ismember (dart_group_t *g, dart_unit_t unitids)
{
	       return dart_adapt_group_ismember (g, unitids);
}*/
dart_ret_t dart_team_get_group (dart_team_t team_id, dart_group_t *group){
	       
	       return dart_adapt_team_get_group (team_id, group);
}                      
/*dart_ret_t dart_team_create (dart_team_t team_id, dart_group_t* group, dart_team_t *newteam)
{
	       return dart_adapt_team_create (team_id, group, newteam);
}*/
dart_ret_t dart_team_destroy (dart_team_t team_id)
{
    	   return dart_adapt_team_destroy (team_id);
}
dart_ret_t dart_myid(dart_unit_t *unitid)
{
	       return dart_adapt_myid(unitid);
}
/*dart_ret_t dart_size(int *size)
{
	       return dart_adapt_size(size);
}*/
dart_ret_t dart_team_myid (dart_team_t team_id, dart_unit_t *unitid)
{
           return dart_adapt_team_myid (team_id, unitid) ;
}
dart_ret_t dart_team_size (dart_team_t team_id, size_t *size)
{
           return dart_adapt_team_size (team_id, size);
}


