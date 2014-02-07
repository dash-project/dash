/** @file dart_adapt_communication.c
 *  @date 21 Nov 2013
 *  @brief Implementations of all the dart communication operations. 
 *  
 *  All the following functions are implemented with the underling *MPI-3*
 *  one-sided runtime system.
 */
#ifndef ENABLE_DEBUG
#define ENABLE_DEBUG
#endif
#include <mpi.h>
#include <stdio.h>
#include "dart_types.h"
#include "dart_adapt_translation.h"
#include "dart_adapt_teamnode.h"
#include "dart_adapt_mem.h"
#include "dart_adapt_initialization.h"
#include "dart_adapt_globmem.h"
#include "dart_adapt_team_group.h"
#include "dart_adapt_communication.h"
#include "mpi_adapt_team_private.h"


/* -- Non-blocking dart one-sided operations -- */

dart_ret_t dart_adapt_get (void *dest, dart_gptr_t gptr, size_t nbytes, dart_handle_t *handle)
{
	MPI_Request mpi_req;
	dart_unit_t target_unitid = gptr.unitid;
	dart_unit_t unitid;
	int offset = gptr.addr_or_offs.offset;
	int flags = gptr. flags;
	MPI_Win win;

	/* The memory accessed is allocated with collective allocation. */
	if (flags == 1)
	{
		int begin;
	        dart_adapt_transtable_query (gptr.segid, offset, &begin, &win);
           	
		/* Difference: the offset relative to the base location of the sub-memory region
		 * spanned by win for unitid. */
		int difference = offset - begin; 

		/* MPI-3 newly added feature: request version of get call. */

		/** TODO: Check if MPI_Rget_accumulate (NULL, 0, MPI_BYTE, dest, nbytes, MPI_BYTE, 
		 *  target_unitid, difference, nbytes, MPI_BYTE, MPI_NO_OP, win, &mpi_req) could be an better alternative? 
		 */
		MPI_Rget (dest, nbytes, MPI_BYTE, target_unitid, difference, nbytes, MPI_BYTE, win, &mpi_req);
		debug_print ("GET	- %d bytes (allocated with collective allocation) from %d at the offset %d\n", 
				nbytes, target_unitid, offset);
	}

	/* The memory accessed is allocated with local allocation. */
	else if (flags == 0)
	{
		win = win_local_alloc;
		MPI_Rget (dest, nbytes, MPI_BYTE, target_unitid, offset, nbytes, MPI_BYTE, win, &mpi_req);
		debug_print ("GET	- %d bytes (allocated with local allocation) from %d at the offset %d\n",
			       	nbytes, target_unitid, offset);
	}
	
	(*handle) -> unitid = target_unitid;
	(*handle) -> mpi_win = win;
	(*handle) -> request = mpi_req;
}

dart_ret_t dart_adapt_put (dart_gptr_t gptr, void *src, size_t nbytes, dart_handle_t *handle)
{
	MPI_Request mpi_req;
	dart_unit_t target_unitid = gptr.unitid;
	int offset = gptr.addr_or_offs.offset;
	int flags = gptr.flags;
	MPI_Win win;
	if (flags == 1)
	{
		int begin;
		dart_adapt_transtable_query (gptr.segid, offset, &begin, &win);

		int difference = offset - begin;
		
		/** TODO: Check if MPI_Raccumulate (src, nbytes, MPI_BYTE, target_unitid, difference, nbytes, MPI_BYTE,
		 *  REPLACE, win, &mpi_req) could be a better alternative? 
		 */
		MPI_Rput (src, nbytes, MPI_BYTE, target_unitid, difference, nbytes, MPI_BYTE, win, &mpi_req);
		debug_print ("PUT	- %d bytes (allocated with collective allocation) to %d at the offset %d\n", 
				nbytes, target_unitid, offset);
	}
	else if (flags == 0)
	{
		win = win_local_alloc;
		
		MPI_Rput (src, nbytes, MPI_BYTE, target_unitid, offset, nbytes, MPI_BYTE, win, &mpi_req);
		debug_print ("PUT	- %d bytes (allocated with local allocation) to %d at the offset %d\n", 
				nbytes, target_unitid, offset);
	}

	(*handle) -> unitid = target_unitid;
	(*handle) -> mpi_win = win;
	(*handle) -> request = mpi_req;
}

/* -- Blocking dart one-sided operations -- */

/** TODO: Check if MPI_Get_accumulate (MPI_NO_OP) can bring better performance? 
 */
dart_ret_t dart_adapt_get_blocking (void *dest, dart_gptr_t gptr, size_t nbytes)
{
	dart_unit_t unitid;
	MPI_Win win;
	dart_handle_t dart_req = (dart_handle_t) malloc (sizeof(struct dart_handle_struct));
	dart_adapt_get (dest, gptr, nbytes, &dart_req);
	win = dart_req -> mpi_win;
	unitid = dart_req -> unitid;
	
	/* MPI-3 newly added feature: MPI_Win_flush, 
	 * light-weight synchronization compared to heavy-weight sycn - unlock. */
	MPI_Win_flush (unitid, win);  
	debug_print ("GET_BLOCKING	- finished\n");
}

/** TODO: Check if MPI_Accumulate (REPLACE) can bring better performance? 
 */
dart_ret_t dart_adapt_put_blocking (dart_gptr_t gptr, void *src, size_t nbytes)
{
	dart_unit_t unitid;
	MPI_Win win;
	dart_handle_t dart_req = (dart_handle_t) malloc (sizeof (struct dart_handle_struct));
	dart_adapt_put (gptr, src, nbytes, &dart_req);
	win = dart_req -> mpi_win;
	unitid = dart_req -> unitid;
	MPI_Win_flush (unitid, win);
	debug_print ("PUT_BLOCKING	- finished\n");
}

#if 0
dart_ret_t dart_fence (dart_handle_t handle)
{
	int id;
        MPI_Win_unlock_all (handle.mpi_win);
	MPI_Win_lock_all (0, handle.mpi_win);
}
#endif

dart_ret_t dart_adapt_wait (dart_handle_t handle)
{
#if 0
	MPI_Status mpi_sta;
	MPI_Request mpi_req = handle -> request;
	MPI_Win win;
	win = handle -> mpi_win;
	MPI_Wait (&mpi_req, &mpi_sta);
	handle -> request = mpi_req; 
#endi#000059
	MPI_Win win;
	dart_unit_t target_unitid;
	win = handle -> mpi_win;
	target_unitid = h#000000#000000andle -> unitid;
	MPI_Win_flush (target_unitid, win);
	printf ("WAIT	- target %d finished\n", target_unitid);
}

dart_ret_t dart_adapt_test (dart_handle_t handle)
{
	MPI_Status mpi_sta;
	int flag;
	MPI_Request mpi_req = handle -> request;
	MPI_Win win;
	win = handle -> mpi_win;
	MPI_Test (&mpi_req, &flag, &mpi_sta);
	handle -> request = mpi_req;
	debug_print ("TEST	- target %d finished\n", handle -> unitid);
	return flag;
}


dart_ret_t dart_adapt_waitall (dart_handle_t *handle, size_t n)
{
#if 0
	MPI_Request *mpi_req;
	MPI_Status *mpi_sta;
	mpi_req = (MPI_Request *)malloc (n * sizeof (MPI_Request));
	mpi_sta = (MPI_Status *)malloc (n * sizeof (MPI_Status));
	int i;
	for (i = 0; i < count; i++)
	{
		mpi_req[i] = handle[i].request;
	}
	MPI_Waitall (n, mpi_req, mpi_sta);
	for (i = 0; i < n; i++)
	{
		handle[i].request = mpi_req[i];
      
	}
#endif
	int i;
	dart_unit_t target_unitid;
	MPI_Win win;
	for(i = 0; i < n; i++)
	{
		win = handle[i] -> mpi_win;
		target_unitid = handle[i] -> unitid;
		MPI_Win_flush (target_unitid, win);
	}
	debug_print ("WAITALL	- finished\n");

}

/** TODO: Rectify its return val type or add another new param in which returns the testing result. 
 */
dart_ret_t dart_adapt_testall (dart_handle_t *handle, size_t n)
{
	MPI_Request *mpi_req;
	MPI_Status *mpi_sta;
	mpi_req = (MPI_Request *)malloc (n * sizeof (MPI_Request));
	mpi_sta = (MPI_Status *)malloc (n * sizeof (MPI_Status));
	int i, flag;
	for (i = 0; i < n; i++)
	{
		mpi_req[i] = handle[i] -> request;
	}
	MPI_Testall (n, mpi_req, &flag, mpi_sta);
	for (i = 0; i < n; i++)
	{
		handle[i] -> request = mpi_req[i];
	}
	debug_print ("TESTALL	- finished\n");
	return flag;
}

/* -- Dart collective operations -- */

dart_ret_t dart_adapt_barrier (dart_team_t teamid)
{
	MPI_Comm comm;

	dart_teamnode_t p;
	dart_adapt_teamnode_query (teamid, &p);
        comm = p -> mpi_comm; /* Get the corresponding communicator from the team hierarchy. */

	if (comm != MPI_COMM_NULL)
	{
		MPI_Barrier (comm);
	}
}

dart_ret_t dart_adapt_bcast (void *buf, size_t nbytes, int root, dart_team_t teamid)
{
	MPI_Comm comm;

	dart_teamnode_t p;
	dart_adapt_teamnode_query(teamid, &p);
	comm = p -> mpi_comm;

	if (comm != MPI_COMM_NULL)
	{
		MPI_Bcast (buf, nbytes, MPI_BYTE, root, comm);
	}
}

dart_ret_t dart_adapt_scatter (void *sendbuf, void *recvbuf, size_t nbytes, int root, dart_team_t teamid)
{
	MPI_Comm comm;

	dart_teamnode_t p;
	dart_adapt_teamnode_query(teamid, &p);
	comm = p -> mpi_comm;

	if (comm != MPI_COMM_NULL)
	{
		MPI_Scatter (sendbuf, nbytes, MPI_BYTE, recvbuf, nbytes, MPI_BYTE, root, comm);
	}
}

dart_ret_t dart_adapt_gather (void *sendbuf, void *recvbuf, size_t nbytes, int root, dart_team_t teamid)
{
	MPI_Comm comm;

	dart_teamnode_t p;
	dart_adapt_teamnode_query (teamid, &p);
	comm = p -> mpi_comm;

        if (comm != MPI_COMM_NULL)
	{
		MPI_Gather (sendbuf, nbytes, MPI_BYTE, recvbuf, nbytes, MPI_BYTE, root, comm);
	}
}

dart_ret_t dart_adapt_allgather (void *sendbuf, void *recvbuf, size_t nbytes, dart_team_t teamid)
{
	MPI_Comm comm;
	dart_teamnode_t p;
	dart_adapt_teamnode_query (teamid, &p);

	comm = p -> mpi_comm;

	if (comm != MPI_COMM_NULL)
	{
		MPI_Allgather (sendbuf, nbytes, MPI_BYTE, recvbuf, nbytes, MPI_BYTE, comm);
	}
}
