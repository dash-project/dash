/** @file dart_adapt_initialization.c
 *  @date 21 Nov 2013
 *  @brief Implementations of the dart init and exit operations
 */

#include "dart_deb_log.h"
#ifndef ENABLE_DEBUG
#define ENABLE_DEBUG
#endif
#include <stdio.h>
#include <mpi.h>
#include "dart_types.h"
#include "dart_adapt_mem.h"
#include "mpi_adapt_team_private.h"
#include "dart_adapt_teamnode.h"
#include "dart_adapt_translation.h"
#include "dart_adapt_initialization.h"
#include "dart_adapt_team_group.h"

/* -- Global objects for dart memory management -- */

char* mempool_localalloc; /* Point to the base address of memory region for local allocation. */

char* mempool_globalalloc[MAX_TEAM_NUMBER]; /* Each item points to the base address of allocated memory region for related team. */

dart_mempool localpool; /* Help to do memory management work for local allocation/free. */

/* Each item helps to do memory management work on corresponding team for collective allocation/free. */
dart_mempool globalpool[MAX_TEAM_NUMBER];

dart_ret_t dart_adapt_init (int* argc, char*** argv)
{
	int rank;
	
	MPI_Init (argc, argv);

	/* Create the team hierarchy tree with the root associated with DART_TEAM_ALL. */
	dart_adapt_teamnode_create();

	dart_adapt_convertform_create();

	MPI_Comm_rank (MPI_COMM_WORLD, &rank);
       
	/* -- Reserve necessary resources for dart programme -- */
	
	/* Reserve a free memory block across all the running units
	 *for dart local allocation. */
	MPI_Alloc_mem (MAX_LENGTH, MPI_INFO_NULL, &mempool_localalloc);
	
	/* Reserve a free shared memory bock across all the running units 
	 * for collective memory allocation on DART_TEAM_ALL. */
	MPI_Alloc_mem (MAX_LENGTH, MPI_INFO_NULL, &(mempool_globalalloc[0]));
		
	localpool = dart_mempool_create (MAX_LENGTH);

	/* NOTE: rank 0 on behalf of other ranks in MPI_COMM_WORLD to provide 
	 * the information on the management of global memory pool (globalpool[0]). */
	if (rank == 0)
	{
		globalpool[0] = dart_mempool_create (MAX_LENGTH);
	}
	
	/* Create a single global win object for dart local allocation.
	 * Return in win_local_alloc. */
	MPI_Win_create (mempool_localalloc, MAX_LENGTH, sizeof (char), MPI_INFO_NULL, MPI_COMM_WORLD, &win_local_alloc);

	/* Start a shared access epoch in win_local_alloc, and later on all the units
	 * can access the memory region allocated by the local allocation
	 * function through win_local_alloc. */	
	MPI_Win_lock_all (0, win_local_alloc);

	debug_print ("%2d: INIT	- initialization finished\n", rank);
}

dart_ret_t dart_adapt_exit ()
{
	dart_unit_t unitid;
	dart_adapt_myid (&unitid);

	/* End the shared access epoch in win_local_alloc. */
 	MPI_Win_unlock_all (win_local_alloc); 

	/* -- Free up all the resources for dart programme -- */
	dart_mempool_destroy (localpool);
	MPI_Free_mem (mempool_localalloc);
	
        if (unitid == 0)
	{
		dart_mempool_destroy (globalpool[0]);
	}
	
	MPI_Free_mem (mempool_globalalloc[0]);
	
	dart_adapt_teamnode_destroy ();

	return MPI_Finalize();

	debug_print ("%2d: EXIT	- Finalization finished\n", unitid);
}


