#include <stdio.h>
#include "dart_app_privates.h"
#include "../dart/dart.h"

int main (int argc, char **argv)
{
	int unitid1, unitid;
	dart_gptr_t gptr;
	dart_lock_t lock, lock_all;
	int array[3];
	MPI_Group group, group2;
	dart_group_t dgroup;
	dart_team_t teamid;
	int i, j;
	j = 0;
	int mutex = 0;
	
	dart_init (&argc, &argv);
	dart_myid (&unitid1);
		
	/* Create criitcal region located in unitid 0 */
	if (unitid1 == 0)
	{
		dart_memalloc (sizeof (int), &gptr);
	}
	dart_bcast (&gptr, sizeof (gptr), 0, DART_TEAM_ALL);
	
	array [0] = 1;
	array [1] = 2;
	array [2] = 3;
	
	MPI_Comm_group (MPI_COMM_WORLD, &group);
	MPI_Group_incl (group, 3, array, &group2);
	dart_group_init(&dgroup);
	dgroup.mpi_group = group2;
	
	dart_team_create (DART_TEAM_ALL, &dgroup, &teamid);
	dart_team_myid (teamid, &unitid);

	/* Initialize the two locks that across two distinct teams */
	dart_team_lock_init (DART_TEAM_ALL, &lock_all);
	
	if (unitid >= 0)
	{
		dart_team_lock_init (teamid, &lock);
	}

	dart_barrier (DART_TEAM_ALL);
	/* Concurrent operations on the two locks */
	if (unitid1 == 0)
	{
		dart_lock_try_acquire (lock_all);
	
		dart_put_blocking (gptr, &unitid1, sizeof (int));
		dart_lock_release (lock_all);
	}

	if (unitid1 == 1)
	{
		dart_lock_acquire (lock_all);
		dart_put_blocking (gptr, &unitid1, sizeof (int));
		dart_lock_release (lock_all);
	}

	if (unitid1 == 2)
	{
		dart_lock_acquire (lock_all);
		dart_put_blocking (gptr, &unitid1, sizeof (int));
		dart_lock_release (lock_all);
	}

	dart_barrier (DART_TEAM_ALL);
	if (unitid == 0)
	{
		dart_lock_acquire (lock);
		for ( i =0 ; i < 100; i++)
		{
			j = j + i;
		}
	}

	if (unitid == 1)
	{
		if (dart_lock_acquire (lock))
		{

			for ( i = 0; i < 100; i++)
			{
				j = j + i;
			}
		}
	}


	if (unitid == 2)
	{
		dart_lock_acquire (lock);
		for ( i = 0 ; i < 100; i++)
		{
			j = j + i;
		}
		dart_lock_release (lock);
	}

	if (unitid == 1 || unitid == 0)
	{
		for ( i = 0; i < 1000; i++)
		{
			j = j + i;
		}
		dart_lock_release (lock);
	}

	dart_barrier (DART_TEAM_ALL);

	/* Free up the two locks */
        if (unitid >= 0)
	{
		dart_team_lock_free (teamid, &lock);
	}
	dart_team_lock_free (DART_TEAM_ALL, &lock_all);
	if (unitid1 == 0)
	{
		int *addr;
		dart_gptr_getaddr (gptr, &addr);
		
		dart_memfree (gptr);
	}	

	dart_exit ();
	return 0;
}
