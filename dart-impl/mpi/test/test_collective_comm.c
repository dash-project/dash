#include <stdio.h>
#include "dart_app_privates.h"
#include "../dart/dart.h"
#define PTINFO(string) {if (unitid == 0) printf (string);}

int main (int argc, char ** argv)
{
	dart_unit_t unitid, unitid2, size, size2;
	int src[5];
	int dest1[4];
	int dest2[4];
	int send[15];
	int receive[5];
	int i;

	dart_gptr_t point, p, p1;

	dart_init (&argc, &argv);
	dart_myid(&unitid);

	dart_memalloc (100, &point);


        /* Create sub-team by using newgroup */

	dart_group_t *group, *newgroup;
	int array1 [2];

	int array2 [3];
	int array3 [2];
	int array4 [2];
	array1 [0] = 1;
	array1 [1] = 2;
	array2 [0] = 0;
	array2 [1] = 1;
	array2 [2] = 2;
	array3 [0] = 0;
	array3 [1] = 1;
	array4 [0] = 1;
	array4 [1] = 2;

        
	 
	/* Dart has not provided the following sub-group creating functions or similar. */
	
	MPI_Group group2, newgroup2, newgroup3, newgroup4, newgroup5;
	MPI_Comm_group (MPI_COMM_WORLD, &group2);
	MPI_Group_incl (group2, 2, array1, &newgroup2);
	dart_group_t dgroup1;
	dgroup1.mpi_group = newgroup2;
	
	dart_group_t dgroup2;
        MPI_Group_incl (group2, 3, array2, &newgroup3);
	dgroup2.mpi_group = newgroup3;

	dart_group_t dgroup3;
	MPI_Group_incl (newgroup3, 2, array3, &newgroup4);
	dgroup3.mpi_group = newgroup4;

	dart_group_t dgroup4;
	MPI_Group_incl (newgroup3, 2, array4, &newgroup5);
	dgroup4.mpi_group = newgroup5;

	/* Return team_id to represent this new sub-team. */

	dart_team_t team_id, team_id2, team_id3, team_id4;

	dart_team_create (DART_TEAM_ALL, &dgroup1, &team_id);
	dart_barrier (DART_TEAM_ALL);
	dart_team_create (DART_TEAM_ALL, &dgroup2, &team_id2);
        dart_barrier (DART_TEAM_ALL);
	dart_team_create (team_id2, &dgroup3, &team_id3);
        dart_barrier (DART_TEAM_ALL);
	dart_team_destroy (team_id3);
	dart_barrier (DART_TEAM_ALL);
	dart_team_create (team_id2, &dgroup4, &team_id4);
	dart_barrier (DART_TEAM_ALL);
        
        dart_team_myid (team_id4, &unitid2);

	dart_team_memalloc_aligned (team_id4, 100, &p);

	
	dart_barrier (DART_TEAM_ALL);
        
	dart_team_memalloc_aligned (team_id, 200, &p1);

	if (unitid2 >= 0)
	{
		for (i=0;i<5;i++)
	        {
			receive[i] = i + unitid2*2;
		}
		printf ("%2d: TESTTEAM %d	- infos of p: p.offset = %d, p.flags = %d, p.segid = %d\n", 
				unitid2, team_id4.team_id, unitid2, p.addr_or_offs.offset, p.flags, p.segid);
	}
	dart_barrier (DART_TEAM_ALL);
	
	if (unitid2 >= 0)
	{
		dart_allgather (receive, send, 5*sizeof (int), team_id4);
	}
        PTINFO("\n******* Test \"dart_gather\" *******\n")

	if (unitid2 == 0)
	{
		for (i=0;i<10;i++)
		{
			printf ("%2d: TESTTEAM %d	- send[%d] = %d\n", unitid2, team_id4.team_id, i, send[i]);
		}
	}
 
	dart_barrier (DART_TEAM_ALL);
        PTINFO ("\n******* Test \"dart_broadcast \" *******\n")
	
	dart_gptr_t p_copy;
	DART_GPTR_COPY (p_copy, point);
	if (unitid == 1)
	{
		printf ("%2d: Before broadcast, the info of p.offset, p.segid, p.unitid and p.flags are %d, %d, %d, %d\n",
			       	unitid, p_copy.addr_or_offs.offset, p_copy.segid, p_copy.unitid, p_copy.flags);
	}


	dart_bcast (&p_copy, sizeof (dart_gptr_t), 0, DART_TEAM_ALL);

	
	if (unitid == 1)
	{
		printf ("%2d: After broadcast: the info of p.offset, p.segid, p.unitid and p.flags are %d, %d, %d, %d\n", 
				unitid, p_copy.addr_or_offs.offset, p_copy.segid, p_copy.unitid, p_copy.flags);
	}
		
        dart_barrier (DART_TEAM_ALL);
	
        
	dart_team_memfree (team_id4, p1);
	
	dart_barrier (DART_TEAM_ALL);
        
	dart_team_memfree (team_id, p);
	dart_memfree (point);
       
	dart_team_destroy (team_id4);
        dart_team_destroy (team_id2);
	
	dart_team_destroy (team_id);
	
	dart_exit ();
	return 0;
}
