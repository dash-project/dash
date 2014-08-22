/** @file dart_adapt_initialization.c
 *  @date 25 Mar 2014
 *  @brief Implementations of the dart init and exit operations.
 */

#include "dart_deb_log.h"
#ifndef ENABLE_DEBUG
#define ENABLE_DEBUG
#endif

#include <stdio.h>
#include <mpi.h>
#include "dart_types.h"
#include "dart_mem.h"
#include "dart_team_private.h"
#include "dart_translation.h"
#include "dart_initialization.h"
#include "dart_team_group.h"

#define MAX_LENGTH_SHARED 1<<23

/* -- Global objects for dart memory management -- */

char* dart_mempool_localalloc; /* Point to the base address of memory region for local allocation. */

char* dart_mempool_globalalloc[DART_MAX_TEAM_NUMBER]; /* Each item points to the base address of allocated memory region for related team. */

dart_mempool dart_localpool; /* Help to do memory management work for local allocation/free. */

/* Each item helps to do memory management work on corresponding team for collective allocation/free. */
dart_mempool dart_globalpool[DART_MAX_TEAM_NUMBER];


dart_ret_t dart_init (int* argc, char*** argv)
{
	int i, j;
	int rank, index, size;
        
	MPI_Win win;
	
	MPI_Init (argc, argv);

	MPI_Info win_info;
	MPI_Info_create (&win_info);
	MPI_Info_set (win_info, "alloc_shared_noncontig", "true");

	
	/* Initialize the teamlist. */
	dart_adapt_teamlist_init ();

	dart_next_availteamid = 0;
	int result = dart_adapt_teamlist_alloc (DART_TEAM_ALL, &index);
	if (result == -1)
	{
		return DART_ERR_OTHER;
	}
	dart_teams[index] = MPI_COMM_WORLD;
	
	dart_next_availteamid ++;
	

	MPI_Comm_rank (MPI_COMM_WORLD, &rank);
	MPI_Comm_size (MPI_COMM_WORLD, &size);	
	dart_localpool = dart_mempool_create (DART_MAX_LENGTH);

	/* NOTE: rank 0 on behalf of other ranks in MPI_COMM_WORLD to provide 
	 * the information on the management of global memory pool (globalpool[0]). 
	 *
	 * Actually there is no statically memory block reserved for dart collective global memory allocation, and dart collective global memory is managed dynamically, which the collective global memory is allocated only when the dart_team_memalloc_aligned is called. Therefore, the size of the collective memory pool can be seen as an infinite pool. 
	 * The reason why the memory pool mechanism is still adopted by dart collective memory management is that the adaptive 'offset' should be returned in the global pointer. 
	 */
	if (rank == 0)
	{
		dart_globalpool[index] = dart_mempool_create (DART_INFINITE);
	}

	/* -- Generate separated numa node and Reserve neccessary resources for dart programm -- */
	MPI_Comm numa_comm;

	/* Splits the communicator into subcommunicators, each of which can create a shared memory region (numa domain) */
	MPI_Comm_split_type (MPI_COMM_WORLD, MPI_COMM_TYPE_SHARED, 1, MPI_INFO_NULL, &numa_comm);
	
	dart_sharedmem_comm_list[index] = numa_comm;

	MPI_Group group_all, numa_group;

	if (numa_comm != MPI_COMM_NULL)
	{
		/* Reserve a free shared memory block for non-collective global memory allocation. */	
		MPI_Win_allocate_shared (DART_MAX_LENGTH, sizeof (char), win_info, numa_comm,
				&(dart_mempool_localalloc), &dart_numa_win_local_alloc);

	
		MPI_Comm_size (numa_comm, &(dart_sharedmemnode_size[index]));
	
		MPI_Comm_group (numa_comm, &numa_group);
		MPI_Comm_group (MPI_COMM_WORLD, &group_all);

	//	dart_unit_mapping[index] = (int *)malloc (sizeof (int) * dart_numa_size[index]);
		
		/* The length of this hash table is set to be the size of DART_TEAM_ALL. */
		dart_sharedmem_table[index] = (int *)malloc (sizeof (int) * size);

		int* dart_unit_mapping = (int *)malloc (sizeof (int) * dart_sharedmemnode_size[index]);
		int* numa_ranks = (int *)malloc (sizeof (int) * dart_sharedmemnode_size[index]);

		for (i = 0; i < dart_sharedmemnode_size[index]; i++)
		{
			numa_ranks[i] = i;
		}
		for (i = 0; i < size; i++)
		{
			dart_sharedmem_table[index][i] = -1;
		}

		/* Generate the set (dart_unit_mapping) of units with absolut IDs, 
		 * which are located in the same numa domain. 
		 */
//		MPI_Group_translate_ranks (numa_group, dart_numa_size[index], 
//					numa_ranks, group_all, dart_unit_mapping[index]);

		
		MPI_Group_translate_ranks (numa_group, dart_sharedmemnode_size[index],
				numa_ranks, group_all, dart_unit_mapping);
	
		/* The non-negative elements in the array 'dart_hash_table[index]' consist of
		 * a node, where the units can communicate via shared memory
		 * and i is the relative position in the node for dart_unit_mapping[i].
		 */
		for (i = 0; i < dart_sharedmemnode_size[index]; i++)
		{
			dart_sharedmem_table[index][dart_unit_mapping[i]] = i;
		}
		free (numa_ranks);
		free (dart_unit_mapping);
	}

	/* Create a single global win object for dart local allocation based on
	 * the aboved allocated shared memory.
	 *
	 * Return in win_local_alloc. */
	MPI_Win_create (dart_mempool_localalloc, DART_MAX_LENGTH, sizeof (char), MPI_INFO_NULL, 
			MPI_COMM_WORLD, &dart_win_local_alloc);

	/* Create a dynamic win object for all the dart collective allocation based on MPI_COMM_WORLD.
	 * Return in win. */
	MPI_Win_create_dynamic (MPI_INFO_NULL, MPI_COMM_WORLD, &win);
	dart_win_lists[index] = win;

	/* Start a shared access epoch in win_local_alloc, and later on all the units
	 * can access the memory region allocated by the local allocation
	 * function through win_local_alloc. */	
	MPI_Win_lock_all (0, dart_win_local_alloc);
	/* Start a shared access epoch in win, and later on all the units can access
	 * the attached memory region allocated by the collective allocation function
	 * through win. */
	MPI_Win_lock_all (0, win);

	MPI_Info_free (&win_info);
	LOG ("%2d: INIT	- initialization finished", rank);

	return DART_OK;
}

dart_ret_t dart_exit ()
{
	int index;
	dart_unit_t unitid;

	dart_myid (&unitid);
 	
	int result = dart_adapt_teamlist_convert (DART_TEAM_ALL, &index);

//	printf ("unitid %d: the teamid is %d, the index is %d and the result is %d\n", unitid, DART_TEAM_ALL, index, result);
	MPI_Win_unlock_all (dart_win_lists[index]);

	/* End the shared access epoch in win_local_alloc. */
	MPI_Win_unlock_all (dart_win_local_alloc);
	
	/* -- Free up all the resources for dart programme -- */
	MPI_Win_free (&dart_win_local_alloc);
	MPI_Win_free (&dart_numa_win_local_alloc);

	MPI_Win_free (&dart_win_lists[index]);
	
	dart_mempool_destroy (dart_localpool);
	if (unitid == 0)
	{
		dart_mempool_destroy (dart_globalpool[index]);
	}
	

	free (dart_sharedmem_table[index]);

	dart_adapt_teamlist_destroy ();
	LOG ("%2d: EXIT - Finalization finished", unitid);
        return MPI_Finalize();

//	return DART_OK;
}


