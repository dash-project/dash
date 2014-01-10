#include <stdio.h>
#include "dart_app_privates.h"
#include "../dart/dart.h"
#define PTINFO(string) {if (unitid == 0) printf (string);}

int main (int argc, char ** argv)
{
	dart_unit_t unitid, unitid2, unitid3, unitid4, size, size2;
	int src[5];
	int dest1[4];
	int dest2[4];
	int send[15];
	int receive[5];
	int i;
	dart_gptr_t p, point;
	dart_gptr_t ptr, ptr2;
	dart_gptr_t p_store;

	dart_init (&argc, &argv);
	dart_myid(&unitid);

	PTINFO("\n******* test \"dart_myid \" *******\n")
	printf ("the unitid of dart_comm 0 is %d\n", unitid);

	PTINFO("\n******* test \" dart_alloc \" *******\n")

	dart_memalloc(100, &point);
	if (unitid == 0)
	{
		dart_memalloc(100, &point);
	}

       /*create sub-team by using newgroup*/

	dart_group_t *group, *newgroup;
	int array1 [2];
	array1 [0] = 1;
	array1 [1] = 2;
	int array2 [3];
	array2 [0] = 1;
	array2 [1] = 2;
	array2 [2] = 3;
	int array3 [2];
	array3 [0] = 0;
	array3 [1] = 1;
	 
	/*using MPI sub-group creation functionality instead*/
	
	MPI_Group group2, newgroup2, newgroup3, newgroup4;
	MPI_Comm_group (MPI_COMM_WORLD, &group2);
	MPI_Group_incl (group2, 2, array1, &newgroup2);
	MPI_Group_incl (group2, 3, array2, &newgroup3);
	MPI_Group_incl (newgroup3, 2, array3, &newgroup4);
	dart_group_t dgroup1, dgroup2, dgroup3;
	dart_group_init (&dgroup1);
	dart_group_init (&dgroup2);
	dart_group_init (&dgroup3);
	dgroup1.mpi_group = newgroup2;
	dgroup2.mpi_group = newgroup3;
	dgroup3.mpi_group = newgroup4;

	/* return team_id to represent this new sub-team*/

	dart_team_t team_id, team_id2, team_id3; 

       	dart_team_create (DART_TEAM_ALL, &dgroup1, &team_id);
        dart_team_create (DART_TEAM_ALL, &dgroup2, &team_id2);
	dart_team_create (team_id2, &newgroup4, &team_id3);
	

        PTINFO("\n******* test \"dart_team_myid \" *******\n")
	

	dart_team_myid (team_id, &unitid2);
	dart_team_myid (team_id2, &unitid3);
	dart_team_myid (team_id3, &unitid4);
	dart_team_destroy (team_id3);

	printf ("\nthe unitid of sub-team 1 is %d\n",unitid2);

	PTINFO("\n******* test \" dart_group_ismember \" *******\n")
	
	if (dart_group_ismember (&dgroup1, unitid))
	{
		printf ("\nonly the unitid %d enter\n", unitid);
		printf ("\nthe unitid %d is %d in newteam\n", unitid, unitid2);
	}
        PTINFO("\n*******the unit not included in b-team will return a negtive value for dart_team_myid*******\n")

	if (unitid2 >= 0)
	{
		printf ("\nthe unitid2 %d is unitid %d\n", unitid2, unitid);
	}
	else 
	{
		printf ("\nthe unitid2 %d is unitid %d\n", unitid2, unitid);
	}

        PTINFO ("\n ******* test \" dart_alloc_aligned \" *******\n")

	dart_team_memalloc_aligned (team_id, 100, &p);
	

        dart_barrier (DART_TEAM_ALL);
        dart_team_memalloc_aligned (DART_TEAM_ALL, 100, &ptr);
	dart_barrier (DART_TEAM_ALL);

   
	dart_team_memalloc_aligned (team_id, 100, &p);
	

	dart_barrier (DART_TEAM_ALL);
	
	dart_team_memalloc_aligned (team_id2, 100, &ptr2);

	dart_barrier (DART_TEAM_ALL);

	

	dart_handle_t handle1, handle2;
	dart_handle_t handleset [3];

	DART_GPTR_COPY (p_store, point);
	dart_barrier (DART_TEAM_ALL);

	for (i = 0; i < 5; i++)
	{
		src[i] = i + 1;
	}
        
	if (unitid == 1)
	{
		p_store.addr_or_offs.offset = 100;
		p_store.unitid = 0;
		p_store.flags = 0;
		dart_put (p_store, src, 2 * sizeof (int), &handle1);
		dart_wait (&handle1);
		dart_fence (handle1);
		dart_get (dest1, p_store, 2 * sizeof (int), &handle1);

		dart_fence (handle1);
		for (i = 0; i < 2 ; i ++)
		{
			printf ("local alloc: the dest1 [%d] is %d\n",i, dest1[i]);
		}
	}

	if (unitid == 0)
	{
		dart_memalloc (50, &point);
		point.addr_or_offs.offset -= 100;
		dart_memfree (point);
		dart_memalloc (50, &point);
	}

	dart_barrier (DART_TEAM_ALL);

	if (unitid == 0)
	{
	
		printf ("local alloc: the mempool_localalloc[100] is %d\n", mempool_localalloc[100]);
		printf ("local alloc: the mempool_localalloc[104] is %d\n", mempool_localalloc[104]);
	
	}
        DART_GPTR_COPY (p_store, ptr2);
	if (unitid3 == 2)
	{
		dart_put (p_store, src, 2 * sizeof (int), &handle1);
		p_store.addr_or_offs.offset += 8;
		dart_put (p_store, src + 2, 2 * sizeof (int), &handle2);
		dart_wait (&handle1);
		dart_wait (&handle2);
		dart_fence (handle1);
	}

	dart_barrier (DART_TEAM_ALL);
	if (unitid3 == 2)
	{
		p_store.addr_or_offs.offset -= 8;
		dart_get (dest1, p_store, 2 * sizeof (int), &handle1);
		p_store.addr_or_offs.offset += 8;
		dart_get (dest1 + 2, p_store, sizeof (int), &handle2);
		dart_wait (&handle1);
		dart_test (&handle1);
		dart_wait (&handle2);
		dart_fence (handle1);
		for (i = 0;i < 3; i++)
		{
			printf ("team 3: dest1[%d] is %d\n", i, dest1[i]);
		}

	}

	dart_barrier(DART_TEAM_ALL);

	DART_GPTR_COPY(p_store, ptr);
	if (unitid == 1)
	{
		dart_put_blocking (p_store, src, 2 * sizeof (int), &handle1);
		*src = 3;
		*(src + 1) = 4;
         	p_store.addr_or_offs.offset += 8;
		dart_put (p_store, src, 2 * sizeof (int), &handle2);
		dart_wait (&handle2);
	}
	if (unitid == 2)
	{
		p_store.addr_or_offs.offset += 24;
	        dart_put (p_store, src + 2, 2 * sizeof (int), &handle1);

		dart_wait (&handle1);

		printf ("unitid %d, the dart_put is finished\n", unitid);
	}


        if (unitid == 1 || unitid == 2)
	{
		dart_fence (handle1);
	}

	if (unitid == 1)
	{
		dart_get (dest1, p_store, 2 * sizeof (int), &handle2);
		dart_wait (&handle2);
	}

	if (unitid == 2)
	{
        	dart_get (dest1, p_store, 2 * sizeof(int), &handle2);
		dart_wait (&handle2);
	}
	if (unitid == 1 || unitid == 2)
	{
		dart_fence (handle2);
	}

	if (unitid == 1)
	{
		for (i = 0; i < 2; i++)
		{
			printf ("unitid %d: the dest [%d] is %d\n", unitid, i, dest1[i]);
		}
	}

	if (unitid == 2)
	{
		for (i = 0; i< 2; i++)
		{
			printf ("unitid %d: the dest [%d] is %d\n", unitid, i,  dest1[i]);
		}
	}


        dart_barrier (DART_TEAM_ALL);

        DART_GPTR_COPY(p_store, p);
        if (unitid2 == 1)
	{
	       	dart_put (p_store, src, 2 * sizeof (int), &handle1);
		p_store.addr_or_offs.offset += 8;
		dart_put (p_store, src+2, 2 * sizeof (int), &handle2);

	        handleset [0] = handle1;
	        handleset [1] = handle2;	
		dart_waitall (handleset, 2);
		int val = dart_testall (handleset, 2);
		printf ("the dart_testall 's return result is %d\n", val);
                dart_fence (handle1);
	}

        dart_barrier (team_id);

	if (unitid2 == 1)
	{
		p_store.addr_or_offs.offset -= 8;
		dart_get_blocking (dest2, p_store, 2 * sizeof (int), &handle1);
		p_store.addr_or_offs.offset += 8;
		dart_get (dest2 + 2, p_store, 2 * sizeof (int), &handle2);
		int val = dart_test (&handle2);
		dart_wait (&handle2);	//	dart_memfree (point);

	}
	
	if (unitid2 == 1)
	{
		dart_fence (handle1);
	}
	if (unitid2 == 1)
	{
		for (i = 0; i < 4; i++)
		{
			printf ("unitid %d: the dest [%d] is %d\n",unitid2, i, dest2[i]);
		}
	}
	
        dart_barrier (DART_TEAM_ALL);       

	if (unitid2 == 0)
	{
		printf ("the value of p.segid is %d\n", p.segid);
		int a = mempool_globalalloc[p.segid][100];
		int b = mempool_globalalloc[p.segid][104];
		printf ("the local data is %d, %d\n", a, b);
	}

	dart_barrier (DART_TEAM_ALL);

	if (unitid3 == 1)
	{
		ptr2.addr_or_offs.offset += 4;
		dart_get (dest2, ptr2, 2 * sizeof (int), &handle1);
		dart_wait (&handle1);
		dart_fence (handle1);
		printf ("finally, the dest2[0] is %d\n", dest2[0]);
		ptr2.addr_or_offs.offset -= 4;
	}

	dart_barrier (DART_TEAM_ALL);
        dart_memfree (point);

	printf ("dart_free: the value of p.segid is %d\n", p.segid);
//	printf ("the team is p %d\n", p.segid);
	dart_team_memfree (team_id, p);
	p.addr_or_offs.offset -= 100;
	dart_team_memfree (team_id, p);
	
	if (ptr2.segid >= 0)
	{
		printf ("dart_free: the value of p.segid is %d\n", ptr2.segid);
 //        	printf ("the team is ptr2 %d\n", ptr2.segid);
		dart_team_memfree (team_id2, ptr2);
        }
	if (ptr.segid >= 0)
	{
		printf ("dart_free: the value of p.segid is %d\n", ptr.segid);
//		printf ("the team is ptr %d\n", ptr.segid);
        	dart_team_memfree (DART_TEAM_ALL, ptr);
	}
	dart_team_destroy (team_id2);
	dart_team_destroy (team_id);
	dart_exit ();
	return 0;
}
