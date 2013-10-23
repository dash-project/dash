#include <stdio.h>
#include <mpi.h>
#include "dart_types.h"
#include "dart_mem.h"
#include "dart_teamnode.h"
#include "dart_translation.h"
#include "dart_team_group.h"


dart_ret_t dart_group_init (dart_group_t *group)
{
	group -> mpi_group = MPI_GROUP_EMPTY;
	return 0;
}

dart_ret_t dart_group_delete (dart_group_t *group)
{
	MPI_Group_free (&(group -> mpi_group));
}

dart_ret_t dart_group_union (dart_group_t *g1, dart_group_t *g2, dart_group_t *g3)
{
	return MPI_Group_union (g1-> mpi_group, g2-> mpi_group, &(g3-> mpi_group));
}

dart_ret_t dart_group_intersect (dart_group_t *g1, dart_group_t *g2, dart_group_t *g3)
{
	return MPI_Group_intersection (g1 -> mpi_group, g2 -> mpi_group, &(g3 -> mpi_group));

}

dart_ret_t dart_group_ismember (dart_group_t *g, dart_unit_t unitids)
{
	dart_unit_t id;
	dart_myid (&id);
	int flag = 0;

	MPI_Comm subcomm;
        MPI_Comm_create (MPI_COMM_WORLD, g->mpi_group, &subcomm);
	if (id == unitids)
	{
		if (subcomm == MPI_COMM_NULL)
			flag = 0;
		else 
			flag = 1;
	}
	MPI_Barrier(MPI_COMM_WORLD);
	return flag;

}

dart_ret_t dart_team_get_group (dart_team_t team_id, dart_group_t *group)
{
	dart_teamnode_t p = dart_teamnode_query (team_id);
	MPI_Comm comm = p -> mpi_comm;
	return MPI_Comm_group (comm, & (group->mpi_group));
}

//info_t trantable_localalloc[MAX_NUMBER];
//int index1 = 0;

const dart_team_t DART_TEAM_ALL = {-1, 0, 0};
dart_teamnode_t dart_header;


dart_ret_t dart_team_create (dart_team_t team_id, dart_group_t* group, dart_team_t *newteam)
{// the teamid stand for a superteam.
	MPI_Comm subcomm;
	int id;
	int sub_unit;
        dart_teamnode_t p;
	p = dart_teamnode_query (team_id);
	int result = 0;
	MPI_Comm_create (MPI_COMM_WORLD, group -> mpi_group, &subcomm);
	MPI_Comm_rank (MPI_COMM_WORLD, &sub_unit);
	int rank;
	if (subcomm != MPI_COMM_NULL)
	{
		MPI_Comm_rank (subcomm, &rank);
	}

	if (result < 0)
	{
		return result;
	}
	
	dart_teamnode_add (team_id, subcomm, newteam);
	dart_convertform_add (*newteam);		
	
	int unique_id;
	
       
	dart_team_uniqueid (*newteam, &unique_id);
	if (subcomm != MPI_COMM_NULL)
	{
//		printf ("dart_team_create: the inserted team_id = %d, the parent_id = %d, the level = %d\n", newteam -> team_id, newteam -> parent_id, newteam -> level);
//		printf ("unique_id is %d\n", unique_id);
		MPI_Alloc_mem (MAX_LENGTH, MPI_INFO_NULL, &(mempool_globalalloc[unique_id]));
		dart_transtable_create (unique_id);
	}
	
	if (rank == 0)
	{
		globalpool[unique_id] = dart_mempool_create(MAX_LENGTH);
	}

}
dart_ret_t dart_team_destroy (dart_team_t team_id)
{
	MPI_Comm comm;
	dart_teamnode_t p = dart_teamnode_query (team_id);

	comm = p -> mpi_comm;
	int intra_id;
       	dart_team_myid (team_id, &intra_id);
        int unique_id;
	
	dart_team_uniqueid (team_id, &unique_id);
//	printf ("dart_team_destroy : unique_id = %d\n", unique_id);
	if (intra_id == 0)
	{
		dart_mempool_destroy (globalpool[unique_id]);
	}
	if (intra_id >= 0)
	{
		MPI_Free_mem (mempool_globalalloc[unique_id]);
        	printf ("the teamid %d is destryed\n", team_id.team_id);
        }
	if (comm != MPI_COMM_NULL)
	{	
		MPI_Comm_free (&comm);
	}
	dart_teamnode_remove (team_id);
}

dart_ret_t dart_myid(dart_unit_t *unitid)
{
	MPI_Comm_rank (MPI_COMM_WORLD, unitid);
}

dart_ret_t dart_size(size_t *size)
{
	MPI_Comm_size (MPI_COMM_WORLD, size);
}

dart_ret_t dart_team_myid (dart_team_t team_id, dart_unit_t *unitid)
{
//	printf ("dart_team_myid enter\n");
//	printf ("team_id: %d, parent_id: %d, level: %d\n", team_id.team_id, team_id.parent_id, team_id.level);
	dart_teamnode_t p;
       	p = dart_teamnode_query (team_id);
	MPI_Comm comm = p -> mpi_comm;
	*unitid = -1;
	if (comm != MPI_COMM_NULL)
	{
		MPI_Comm_rank (comm, unitid);
	}
}

dart_ret_t dart_team_size (dart_team_t team_id, size_t *size)
{
	dart_teamnode_t p = dart_teamnode_query (team_id);
	MPI_Comm comm = p -> mpi_comm;
	*size = -1;
	if (comm != MPI_COMM_NULL)
		MPI_Comm_size (comm, size);
}


