/** @file dart_adapt_team_group.c
 *  @date 21 Nov 2013
 *  @brief implementation of dart operations on team&group.
 */

#ifndef ENABLE_DEBUG
#define ENABLE_DEBUG
#endif
#include <stdio.h>
#include "dart_types.h"
#include "dart_adapt_mem.h"
#include "dart_adapt_teamnode.h"
#include "dart_adapt_translation.h"
#include "dart_adapt_team_group.h"
#include "mpi_adapt_team_private.h"


dart_ret_t dart_adapt_group_init (dart_group_t *group)
{
	group->mpi_group = MPI_GROUP_EMPTY;
	return dart_ret_t(0);
}

dart_ret_t dart_adapt_group_fini (dart_group_t *group)
{
	MPI_Group_free (&(group -> mpi_group));
}

dart_ret_t dart_adapt_group_union (dart_group_t *g1, dart_group_t *g2, dart_group_t *g3)
{
	int i;
	i = MPI_Group_union (g1-> mpi_group, g2-> mpi_group, &(g3)->mpi_group);
	return dart_ret_t(i);
}

dart_ret_t dart_adapt_group_size(const dart_group_t *g, int *size)
{
	int i =  MPI_Group_size( g->mpi_group, size);
	return dart_ret_t(i);
}

dart_ret_t dart_adapt_group_intersect (dart_group_t *g1, dart_group_t *g2, dart_group_t *g3)
{
	int i;
	i =MPI_Group_intersection (g1 -> mpi_group, g2 -> mpi_group, &(g3 -> mpi_group));
	return dart_ret_t(i);
}

dart_ret_t dart_adapt_group_addmember(dart_group_t *g, dart_unit_t unitid)
{
	int i ;
	i = MPI_Comm_create(unitid, g->mpi_group, &(g)->mpi_group);
	return dart_ret_t(i);
}

dart_ret_t dart_adapt_group_delmember(dart_group_t *g, dart_unit_t unitid)
{
	int i, p;
	MPI_Comm_size(MPI_COMM_WORLD, &p);
	MPI_Comm_group(MPI_COMM_WORLD, &(g)->mpi_group);
	MPI_Group_excl(g->mpi_group, p-1, unitid, &(g)->mpi_group);
}
	
dart_ret_t dart_adapt_group_ismember (const dart_group_t *g, dart_unit_t unitids, int32_t *ismember)
{
	dart_unit_t id;
	MPI_Comm subcomm;
	int flag = 0;
	dart_myid (&id);

	/* Get the subcomm associated with the specified group. */
        MPI_Comm_create (MPI_COMM_WORLD, g->mpi_group, &subcomm);
	if (id == unitids)
	{
#if 0
		if (subcomm == MPI_COMM_NULL)
		{
			*ismember = 0;
		}
		else
		{	
			*ismember = 1;
		}
#endif
		*ismember = (subcomm != MPI_COMM_NULL);
		debug_print ("%2d: GROUP_ISMEMBER	- %s\n", unitids, (*ismember) ? "yes" : "no");
	}
}

dart_ret_t dart_adapt_group_copy( dart_group_t *gin,   dart_group_t *gout)
{
	*gout = *gin;
	return dart_ret_t(1);
}

dart_ret_t dart_adapt_team_get_group (dart_team_t teamid, dart_group_t *group)
{
	dart_teamnode_t p;
	dart_adapt_teamnode_query (teamid, &p);
	MPI_Comm comm = p -> mpi_comm;
	return MPI_Comm_group (comm, & (group->mpi_group));
}

/** TODO: Differentiate units belonging to team_id and that not belonging to team_id
 *  within the function or out of it?
 *
 *  The teamid stands for a superteam related to the new generated newteam.
 */
dart_ret_t dart_adapt_team_create (dart_team_t teamid, const dart_group_t* group, dart_team_t *newteam)
{
	
	MPI_Comm subcomm;
	int rank, unique_id;
	dart_unit_t sub_unit;
        dart_teamnode_t p;

	dart_adapt_myid (&sub_unit);

	/* Query the team node according to teamid from the team hierarchy. */
	dart_adapt_teamnode_query (teamid, &p);
#if 0
	MPI_Comm_create (MPI_COMM_WORLD, group -> mpi_group, &subcomm);
	MPI_Comm_rank (MPI_COMM_WORLD, &sub_unit);
#endif
	subcomm = MPI_COMM_NULL;
	if ((p -> mpi_comm) != MPI_COMM_NULL)
	{
		MPI_Comm_create (p -> mpi_comm, group -> mpi_group, &subcomm);
	}

	if (subcomm != MPI_COMM_NULL)
	{
		MPI_Comm_rank (subcomm, &rank);
	}
	
	/** TODO: If both the team tree and convertform should be synchronized globally? 
	 */
	
	/* Add newteam node into the team tree hierarchy. */
	dart_adapt_teamnode_add (teamid, subcomm, newteam);

	/* Identify the newteam uniquely through static array convert from. */
	dart_adapt_convertform_add (*newteam);		
	
        /* Fetch the unique number (unique_id) associated with newteam from the
	 * static array convert form. */
	dart_adapt_team_uniqueid (*newteam, &unique_id);
	
	/* Reserve resources for the dart operations on newteam. */
	if (subcomm != MPI_COMM_NULL)
	{
		MPI_Alloc_mem (MAX_LENGTH, MPI_INFO_NULL, &(mempool_globalalloc[unique_id]));
		dart_adapt_transtable_create (unique_id);
	}
	
	if (rank == 0)
	{
		globalpool[unique_id] = dart_mempool_create(MAX_LENGTH);
	}
	if (subcomm != MPI_COMM_NULL)
	{
		debug_print ("%2d: TEAMCREATE	- create team %d\n", sub_unit, newteam -> team_id);
	}
}

dart_ret_t dart_adapt_team_destroy (dart_team_t teamid)
{
	MPI_Comm comm;
	dart_unit_t unitid, id;
	int unique_id;

	dart_teamnode_t p;
	dart_adapt_teamnode_query (teamid, &p);
	comm = p -> mpi_comm;

       	dart_adapt_team_myid (teamid, &unitid);
	dart_adapt_myid (&id);
      
	dart_adapt_team_uniqueid (teamid, &unique_id);

	/* -- Free up resources that were allocated for teamid before -- */
	if (unitid == 0)
	{
		dart_mempool_destroy (globalpool[unique_id]);
	}
	if (unitid >= 0)
	{
		MPI_Free_mem (mempool_globalalloc[unique_id]);
        }

	dart_adapt_convertform_remove (teamid);

	dart_adapt_teamnode_remove (teamid);


	/* -- Release the communicator associated with teamid -- */
	if (comm != MPI_COMM_NULL)
	{	
		MPI_Comm_free (&comm);
		debug_print ("%2d: TEAMDESTROY	- destroy team %d\n", id, teamid.team_id);
	}
}

dart_ret_t dart_adapt_myid(dart_unit_t *unitid)
{
	MPI_Comm_rank (MPI_COMM_WORLD, unitid);
}

dart_ret_t dart_adapt_size(size_t *size)
{
	MPI_Comm_size (MPI_COMM_WORLD, size);
}

dart_ret_t dart_adapt_team_myid (dart_team_t teamid, dart_unit_t *unitid)
{
	dart_teamnode_t p;
       	dart_adapt_teamnode_query (teamid, &p);
	MPI_Comm comm = p -> mpi_comm;

	/* The number of unit could be classified into two cases:
	 *
	 * - the unitid is negative (-1) for the unit who is not belonging to teamid. 
	 *
	 * - the unitid is positive (0,1,..., size-1) for the unit belonging to teamid.
	 *
	 * Hence, it can be used to differentiate between units belonging to teamid and 
	 * units belonging to teamid.
	 *
	 * Just like the usage of MPI_COMM_NULL in MPI.*/
	*unitid = -1;
	if (comm != MPI_COMM_NULL)
	{
		MPI_Comm_rank (comm, unitid); 
	}
}

dart_ret_t dart_adapt_team_size (dart_team_t teamid, size_t *size)
{
	dart_teamnode_t p;
	dart_adapt_teamnode_query (teamid, &p);
	MPI_Comm comm = p -> mpi_comm;
	*size = -1;
	if (comm != MPI_COMM_NULL)
	{
		MPI_Comm_size (comm, size);

	}
}


