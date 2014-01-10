#include <stdio.h>
#include <mpi.h>
#include "dart_types.h"
#include "dart_mem.h"
#include "mpi_team_private.h"
#include "dart_teamnode.h"
#include "dart_translation.h"
#include "dart_team_group.h"


char* mempool_localalloc;

char* mempool_globalalloc[MAX_TEAM_NUMBER];

dart_mempool localpool;
dart_mempool globalpool[MAX_TEAM_NUMBER];
MPI_Win win_local_alloc;

dart_ret_t dart_init (int* argc, char*** argv)
{
	
	MPI_Init (argc, argv);
	dart_header = dart_teamnode_create();
	dart_team_t team_medium = convertform[0].team;
        team_medium.team_id = 0;
	team_medium.parent_id = -1;
	team_medium.level = 0;
	convertform[0].flag = 1;
	
	int rank;
	MPI_Comm_rank (MPI_COMM_WORLD, &rank);
        printf ("dart_init: the value of rank is %d\n",rank);
	MPI_Alloc_mem (MAX_LENGTH, MPI_INFO_NULL, &mempool_localalloc);
	MPI_Alloc_mem (MAX_LENGTH, MPI_INFO_NULL, &(mempool_globalalloc[0]));
	localpool = dart_mempool_create (MAX_LENGTH);
	
	if (rank == 0)
	{
		globalpool[0] = dart_mempool_create (MAX_LENGTH);
	}

	MPI_Win_create (mempool_localalloc, MAX_LENGTH, sizeof (char), MPI_INFO_NULL, MPI_COMM_WORLD, &win_local_alloc);
	MPI_Win_lock_all (0, win_local_alloc);
}

dart_ret_t dart_exit ()
{
        dart_mempool_destroy (localpool);
	MPI_Free_mem (mempool_localalloc);
	int id;
	dart_myid (&id);
	if (id == 0)
	{
		dart_mempool_destroy (globalpool[0]);
	}
	MPI_Free_mem (mempool_globalalloc[0]);
        return MPI_Finalize();
}


