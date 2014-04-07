/** @file dart_adapt_communication.c
 *  @date 25 Mar 2014
 *  @brief Implementations of all the dart communication operations. 
 *  
 *  All the following functions are implemented with the underling *MPI-3*
 *  one-sided runtime system.
 */
#include "dart_deb_log.h"
#ifndef ENABLE_DEBUG
#define ENABLE_DEBUG
#endif
#ifndef ENABLE_LOG
#define ENABLE_LOG
#endif
#include <stdio.h>
#include <mpi.h>
#include "dart_types.h"
#include "dart_translation.h"
#include "dart_team_private.h"
#include "dart_mem.h"
#include "dart_initialization.h"
#include "dart_globmem.h"
#include "dart_team_group.h"
#include "dart_communication.h"
#include "dart_communication_priv.h"

/* -- Non-blocking dart one-sided operations -- */

dart_ret_t dart_get (void *dest, dart_gptr_t gptr, size_t nbytes, dart_handle_t *handle)
{
	MPI_Request mpi_req;
	dart_unit_t target_unitid;
//	dart_unit_t unitid;
	int offset = gptr.addr_or_offs.offset;
	int flags = gptr. flags;
	MPI_Win win;

	/* The memory accessed is allocated with collective allocation. */
	if (flags == 1)
	{
		int index, begin;
		int teamid = gptr.segid;
		int result = dart_adapt_teamlist_convert (teamid, &index);

		if (result == -1)
		{
			return DART_ERR_INVAL;
		}
	        if (dart_adapt_transtable_query (index, offset, &begin, &win) == -1)
		{
			ERROR ("Invalid accessing operation");
			return DART_ERR_INVAL;
		}
		
		/* Difference: the offset relative to the base location of the sub-memory region
		 * spanned by win for unitid. */
		int difference = offset - begin; 
		
		/* Translate local unitID (relative to teamid) into global unitID (relative to DART_TEAM_ALL).
		 *
		 * Note: target_unitid should not be the global unitID but rather the local unitID relative to
		 * the team associated with the specified win object.
		 */
		dart_team_unit_g2l (teamid, gptr.unitid, &target_unitid);

		/* MPI-3 newly added feature: request version of get call. */

		/** TODO: Check if MPI_Rget_accumulate (NULL, 0, MPI_BYTE, dest, nbytes, MPI_BYTE, 
		 *  target_unitid, difference, nbytes, MPI_BYTE, MPI_NO_OP, win, &mpi_req) could be an better alternative? 
		 */
		MPI_Rget (dest, nbytes, MPI_BYTE, target_unitid, difference, nbytes, MPI_BYTE, win, &mpi_req);
		DEBUG ("GET	- %d bytes (allocated with collective allocation) from %d at the offset %d", 
				nbytes, target_unitid, offset);
	}

	/* The memory accessed is allocated with local allocation. */
	else if (flags == 0)
	{
		win = win_local_alloc;
		target_unitid = gptr.unitid;
		MPI_Rget (dest, nbytes, MPI_BYTE, target_unitid, offset, nbytes, MPI_BYTE, win, &mpi_req);
		DEBUG ("GET	- %d bytes (allocated with local allocation) from %d at the offset %d",
			       	nbytes, target_unitid, offset);
	}
	
	(*handle) -> request = mpi_req;
	return DART_OK;
}

dart_ret_t dart_put (dart_gptr_t gptr, void *src, size_t nbytes, dart_handle_t *handle)
{
	MPI_Request mpi_req;
	dart_unit_t target_unitid;
	int offset = gptr.addr_or_offs.offset;
	int flags = gptr.flags;
	MPI_Win win;
	if (flags == 1)
	{
		int index, begin;
		int teamid = gptr.segid;
		int result = dart_adapt_teamlist_convert (teamid, &index);
		if (dart_adapt_transtable_query (index, offset, &begin, &win) == -1)
		{
			ERROR ("Invalid accessing operation");
			return DART_ERR_INVAL;
		}

		int difference = offset - begin;
				
		dart_team_unit_g2l (teamid, gptr.unitid, &target_unitid);
		/** TODO: Check if MPI_Raccumulate (src, nbytes, MPI_BYTE, target_unitid, difference, nbytes, MPI_BYTE,
		 *  REPLACE, win, &mpi_req) could be a better alternative? 
		 */
		MPI_Rput (src, nbytes, MPI_BYTE, target_unitid, difference, nbytes, MPI_BYTE, win, &mpi_req);
		DEBUG ("PUT	- %d bytes (allocated with collective allocation) to %d at the offset %d", 
				nbytes, target_unitid, offset);
	}
	else if (flags == 0)
	{
		win = win_local_alloc;
		target_unitid = gptr.unitid;
		
		MPI_Rput (src, nbytes, MPI_BYTE, target_unitid, offset, nbytes, MPI_BYTE, win, &mpi_req);
		DEBUG ("PUT	- %d bytes (allocated with local allocation) to %d at the offset %d", 
				nbytes, target_unitid, offset);
	}

	(*handle) -> request = mpi_req;
	return DART_OK;
}

/* -- Blocking dart one-sided operations -- */

/** TODO: Check if MPI_Get_accumulate (MPI_NO_OP) can bring better performance? 
 */
dart_ret_t dart_blocking (void *dest, dart_gptr_t gptr, size_t nbytes)
{
	dart_handle_t dart_req = (dart_handle_t) malloc (sizeof(struct dart_handle_struct));
	MPI_Status mpi_sta;
	if (dart_get (dest, gptr, nbytes, &dart_req) != DART_OK)
	{
		return DART_ERR_INVAL;
	}
		
	MPI_Wait (&(dart_req -> request), &mpi_sta);
	LOG ("GET_BLOCKING	- finished");
	return DART_OK;
}

/** TODO: Check if MPI_Accumulate (REPLACE) can bring better performance? 
 */
dart_ret_t dart_put_blocking (dart_gptr_t gptr, void *src, size_t nbytes)
{
	dart_handle_t dart_req = (dart_handle_t) malloc (sizeof (struct dart_handle_struct));
	MPI_Status mpi_sta;
	if (dart_put (gptr, src, nbytes, &dart_req) != DART_OK)
	{
		return DART_ERR_INVAL;
	}

	MPI_Wait (&(dart_req -> request), &mpi_sta);
	LOG ("PUT_BLOCKING	- finished");
	return DART_OK;
}


dart_ret_t dart_wait (dart_handle_t handle)
{
	MPI_Status mpi_sta;
	MPI_Wait (&(handle -> request), &mpi_sta);

	LOG ("WAIT	- finished");
	return DART_OK;
}

dart_ret_t dart_test (dart_handle_t handle, int* finished)
{
	MPI_Status mpi_sta;
	MPI_Test (&(handle -> request), finished, &mpi_sta);

	LOG ("TEST	- finished");
	return DART_OK;
}


dart_ret_t dart_waitall (dart_handle_t *handle, size_t n)
{
	int i;
	MPI_Status *mpi_sta;
	MPI_Request *mpi_req;
	mpi_req = (MPI_Request *)malloc (n * sizeof (MPI_Request));
	mpi_sta = (MPI_Status *)malloc (n * sizeof (MPI_Status));

	for (i = 0; i < n; i++)
	{
		mpi_req [i] = handle[i] -> request;
	}
	MPI_Waitall (n, mpi_req, mpi_sta);
	for (i = 0; i < n; i++)
	{
		handle[i] -> request = mpi_req [i];
	}

	free (mpi_req);
	free (mpi_sta);
	LOG ("WAITALL	- finished");
	return DART_OK;
}

/** TODO:	Rectify its return val type or add another new param in which returns the testing result.\n 
 *  FIX:	Adding a flag param here.
 */
dart_ret_t dart_testall (dart_handle_t *handle, size_t n, int* finished)
{
	int i;
	MPI_Status *mpi_sta;
	MPI_Request *mpi_req;
	mpi_req = (MPI_Request *)malloc (n * sizeof (MPI_Request));
	mpi_sta = (MPI_Status *)malloc (n * sizeof (MPI_Status));

	for (i = 0; i < n; i++)
	{
		mpi_req [i] = handle[i] -> request;
	}
	MPI_Testall (n, mpi_req, finished, mpi_sta);
	for (i = 0; i < n; i++)
	{
		handle[i] -> request = mpi_req[i];
	}

	free (mpi_req);
	free (mpi_sta);
	LOG ("TESTALL	- finished");
	return DART_OK;
}

/* -- Dart collective operations -- */

dart_ret_t dart_barrier (dart_team_t teamid)
{
	MPI_Comm comm;	
	int index;
	int result = dart_adapt_teamlist_convert (teamid, &index);
	if (result == -1)
	{
		return DART_ERR_INVAL;
	}

	/* Fetch proper communicator from teams. */
	comm = teams[index];	
	return MPI_Barrier (comm);
}

dart_ret_t dart_bcast (void *buf, size_t nbytes, int root, dart_team_t teamid)
{
	MPI_Comm comm;
	int index;
	int result = dart_adapt_teamlist_convert (teamid, &index);
	if (result == -1)
	{
		return DART_ERR_INVAL;
	}

	comm = teams[index];
	return MPI_Bcast (buf, nbytes, MPI_BYTE, root, comm);
}

dart_ret_t dart_scatter (void *sendbuf, void *recvbuf, size_t nbytes, int root, dart_team_t teamid)
{
	MPI_Comm comm;
	int index;
	int result = dart_adapt_teamlist_convert (teamid, &index);
	if (result == -1)
	{
		return DART_ERR_INVAL;
	}
	comm = teams[index];

	return MPI_Scatter (sendbuf, nbytes, MPI_BYTE, recvbuf, nbytes, MPI_BYTE, root, comm);
}

dart_ret_t dart_gather (void *sendbuf, void *recvbuf, size_t nbytes, int root, dart_team_t teamid)
{
	MPI_Comm comm;
	int index;

	int result = dart_adapt_teamlist_convert (teamid, &index);
	if (result == -1)
	{
		return DART_ERR_INVAL;
	}
	comm = teams[index];

	return MPI_Gather (sendbuf, nbytes, MPI_BYTE, recvbuf, nbytes, MPI_BYTE, root, comm);
}

dart_ret_t dart_allgather (void *sendbuf, void *recvbuf, size_t nbytes, dart_team_t teamid)
{
	MPI_Comm comm;
	int index;
	int result = dart_adapt_teamlist_convert (teamid, &index);
	if (result == -1)
	{
		return DART_ERR_INVAL;
	}
	comm = teams[index];

	return MPI_Allgather (sendbuf, nbytes, MPI_BYTE, recvbuf, nbytes, MPI_BYTE, comm);
}
