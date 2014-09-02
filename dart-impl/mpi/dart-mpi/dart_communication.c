/** @file dart_communication.c
 *  @date 25 Aug 2014
 *  @brief Implementations of all the dart communication operations. 
 *  
 *  All the following functions are implemented with the underling *MPI-3*
 *  one-sided runtime system.
 */

/*
#ifndef ENABLE_DEBUG
#define ENABLE_DEBUG
#endif
#ifndef ENABLE_LOG
#define ENABLE_LOG
#endif
*/

#include "dart_deb_log.h"
#include <stdio.h>
#include <mpi.h>
#include <string.h>
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
	MPI_Aint disp_s, disp_rel;
	dart_unit_t target_unitid_abs;
	uint64_t base, offset = gptr.addr_or_offs.offset;
	uint16_t flags = gptr. flags;
	MPI_Win win;

	*handle = (dart_handle_t) malloc (sizeof (struct dart_handle_struct));
	target_unitid_abs = gptr.unitid;
	
	/* The memory accessed is allocated with collective allocation. */
	if (flags == 1)
	{
		int index, teamid, target_unitid_rel;

		teamid = gptr.segid;
		int result = dart_adapt_teamlist_convert (teamid, &index);

		if (result == -1)
		{
			return DART_ERR_INVAL;
		}
		/*
	        if (dart_adapt_transtable_get_win (index, offset, &begin, &win) == -1)
		{
			ERROR ("Invalid accessing operation");
			return DART_ERR_INVAL;
		}
	        */
		win = dart_win_lists[index];
		/* Difference: the offset relative to the base location of the sub-memory region
		 * spanned by win for unitid. */

		
		/* Translate local unitID (relative to teamid) into global unitID (relative to DART_TEAM_ALL).
		 *
		 * Note: target_unitid should not be the global unitID but rather the local unitID relative to
		 * the team associated with the specified win object.
		 */
		dart_team_unit_g2l (teamid, target_unitid_abs, &target_unitid_rel);

		if (dart_adapt_transtable_get_disp (index, offset, target_unitid_rel, &base, &disp_s)== -1)
		{
			return DART_ERR_INVAL;
		}

		disp_rel = disp_s + (offset - base);
		 
		/* MPI-3 newly added feature: request version of get call. */

		/** TODO: Check if MPI_Rget_accumulate (NULL, 0, MPI_BYTE, dest, nbytes, MPI_BYTE, 
		 *  target_unitid, disp_rel, nbytes, MPI_BYTE, MPI_NO_OP, win, &mpi_req) could be an better alternative? 
		 */
	//	MPI_Rget (dest, nbytes, MPI_BYTE, target_unitid_rel, disp_rel, nbytes, MPI_BYTE, win, &mpi_req);
		MPI_Rget (dest, nbytes, MPI_BYTE, target_unitid_rel, disp_rel, nbytes, MPI_BYTE, win, &mpi_req);

		DEBUG ("GET	- %d bytes (allocated with collective allocation) from %d at the offset %d", 
				nbytes, target_unitid_abs, offset);
	}

	/* The memory accessed is allocated with local allocation. */
	else if (flags == 0)
	{
		win = dart_win_local_alloc;
		MPI_Rget (dest, nbytes, MPI_BYTE, target_unitid_abs, offset, nbytes, MPI_BYTE, win, &mpi_req);
		DEBUG ("GET	- %d bytes (allocated with local allocation) from %d at the offset %d",
			       	nbytes, target_unitid_abs, offset);
	}
	
	(*handle) -> request = mpi_req;
	return DART_OK;
}

dart_ret_t dart_put (dart_gptr_t gptr, void *src, size_t nbytes, dart_handle_t *handle)
{
	int i, j;
	MPI_Request mpi_req;
	MPI_Aint disp_s, disp_rel;
	dart_unit_t target_unitid_abs;
	uint64_t base, offset = gptr.addr_or_offs.offset;
	uint16_t flags = gptr.flags;
	MPI_Win win;
	
	*handle = (dart_handle_t) malloc (sizeof (struct dart_handle_struct));
	target_unitid_abs = gptr.unitid;

	if (flags == 1)
	{
		int index, teamid, target_unitid_rel;
	
		teamid = gptr.segid;
		int result = dart_adapt_teamlist_convert (teamid, &index);
		if (result == -1)
		{
			return DART_ERR_INVAL;
		}
		/*
		if (dart_adapt_transtable_get_win (index, offset, &begin, &win) == -1)
		{
			ERROR ("Invalid accessing operation");
			return DART_ERR_INVAL;
		}
		*/

		win = dart_win_lists[index];		
		dart_team_unit_g2l (teamid, target_unitid_abs, &target_unitid_rel);
		if (dart_adapt_transtable_get_disp (index, offset, target_unitid_rel, &base, &disp_s) == -1)
		{
			return DART_ERR_INVAL;
		}	

		disp_rel = disp_s + (offset - base);
		/** TODO: Check if MPI_Raccumulate (src, nbytes, MPI_BYTE, target_unitid, disp_rel, nbytes, MPI_BYTE,
		 *  REPLACE, win, &mpi_req) could be a better alternative? 
		 */
//		MPI_Rput (src, nbytes, MPI_BYTE, target_unitid_rel, disp_rel, nbytes, MPI_BYTE, win, &mpi_req);
		MPI_Rput (src, nbytes, MPI_BYTE, target_unitid_rel, disp_rel, nbytes, MPI_BYTE, win, &mpi_req);
		DEBUG ("PUT	-%d bytes (allocated with collective allocation) to %d at the offset %d",
				nbytes, target_unitid_abs, offset);

	}
	else if (flags == 0)
	{
		win = dart_win_local_alloc;
		
		MPI_Rput (src, nbytes, MPI_BYTE, target_unitid_abs, offset, nbytes, MPI_BYTE, win, &mpi_req);
		DEBUG ("PUT	- %d bytes (allocated with local allocation) to %d at the offset %d", 
				nbytes, target_unitid_abs, offset);
	}

	(*handle) -> request = mpi_req;
	return DART_OK;
}


/* -- Blocking dart one-sided operations -- */

/** TODO: Check if MPI_Get_accumulate (MPI_NO_OP) can bring better performance? 
 */

dart_ret_t dart_put_blocking (dart_gptr_t gptr, void *src, size_t nbytes)
{
	int i, is_sharedmem = 0;
	int index, teamid, result;

	MPI_Win win;
	MPI_Status mpi_sta;
	MPI_Request mpi_req;
	MPI_Aint disp_s, maximum_size, disp_rel;

	uint64_t base, offset = gptr.addr_or_offs.offset;
	uint16_t flags = gptr.flags;
	dart_unit_t unitid, target_unitid_rel, target_unitid_abs = gptr.unitid;

	int disp_unit;
	char *baseptr;

	if (flags == 1)
	{
		teamid = gptr.segid;
		result = dart_adapt_teamlist_convert (teamid, &index);
		if (result == -1)
		{
			return DART_ERR_INVAL;
		}
	}
	else 
	{
		index = DART_TEAM_ALL;
	}

	/* Checking whether origin and target are in the same node. 
	 * We use the approach of shared memory accessing only when it passed the above check. */
	
			
//	i = binary_search (dart_unit_mapping[j], gptr.unitid, 0, dart_sharedmem_size[j] - 1);
	/* The value of i will be the target's relative ID in teamid. */
	i = dart_sharedmem_table[index][gptr.unitid];

	if (i >= 0)
	{
		is_sharedmem = 1;
	}
	

	if (is_sharedmem)
	{
		dart_myid (&unitid);
		if (flags == 1)
		{
			if (dart_adapt_transtable_get_win (index, offset, &base, &win) == -1)
			{
				return DART_ERR_INVAL;
			}
		}
		
		if (unitid == target_unitid_abs)
		{/* If orgin and target are identical, then switches to local access. */
			if (flags == 1)
			{
				int flag;
				
				MPI_Win_get_attr (win, MPI_WIN_BASE, &baseptr, &flag);
				baseptr = baseptr + (offset - base);
			}
			else
			{
				baseptr = offset + dart_mempool_localalloc;
			}
		}
		else
		{/* Accesses through shared memory (store). */
			if (flags == 1)
			{
				disp_rel = offset - base;
			}
			else
			{
				disp_rel = offset;
				win = dart_sharedmem_win_local_alloc;
			}
			MPI_Win_shared_query (win, i, &maximum_size, &disp_unit, &baseptr);
			baseptr += disp_rel;
		}
		memcpy (baseptr, ((char*)src), nbytes);
	}
	else
	{/* The traditional remote access method */
		if (flags == 1)
		{	
			win = dart_win_lists[index];
	           	dart_team_unit_g2l (teamid, target_unitid_abs, &target_unitid_rel);
			if (dart_adapt_transtable_get_disp (index, offset, target_unitid_rel, &base, &disp_s) == -1)
			{
				return DART_ERR_INVAL;
			}	
			disp_rel = disp_s + (offset - base);
		}
		else
		{
			win = dart_win_local_alloc;
			disp_rel = offset;
			target_unitid_rel = target_unitid_abs;
		}
	
	//	MPI_Put (src, nbytes, MPI_BYTE, target_unitid, difference, nbytes, MPI_BYTE, win);
		MPI_Rput (src, nbytes, MPI_BYTE, target_unitid_rel, disp_rel, nbytes, MPI_BYTE, win, &mpi_req);
		MPI_Wait (&mpi_req, &mpi_sta);
	}
	
	if (flags == 1)
 	{
        	DEBUG ("PUT_BLOCKING	- %d bytes (allocated with collective allocation) to %d at the offset %d", nbytes, target_unitid_abs, offset);
	}
	else
        {
		DEBUG ("PUT_BLOCKING - %d bytes (allocated with local allocation) to %d at the offset %d", nbytes, target_unitid_abs, offset);
	}

	return DART_OK;
}


/** TODO: Check if MPI_Accumulate (REPLACE) can bring better performance? 
 */
dart_ret_t dart_get_blocking (void *dest, dart_gptr_t gptr, size_t nbytes)
{
	int i, j, is_sharedmem = 0;
	dart_team_t teamid;
	int index, result;
	
	MPI_Win win;
	MPI_Status mpi_sta;
	MPI_Request mpi_req;
	MPI_Aint disp_s, maximum_size, disp_rel;
	
	uint64_t base, offset = gptr.addr_or_offs.offset;
	uint16_t flags = gptr.flags;
	dart_unit_t unitid, target_unitid_rel, target_unitid_abs = gptr.unitid;
	
	int disp_unit;
	char* baseptr;
	
	if (flags == 1)
	{
		teamid = gptr.segid;
		result = dart_adapt_teamlist_convert (teamid, &index);
		if (result == -1)
		{
			return DART_ERR_INVAL;
		}
	}
	else 
	{
		index = 0;
	}
	j = index;

//	i = binary_search (dart_unit_mapping[j], gptr.unitid, 0, dart_sharedmem_size[j] - 1);

	/* Check whether the target is in the same node as the calling unit or not. */
	i = dart_sharedmem_table[j][gptr.unitid];
	if (i >= 0)
	{
		is_sharedmem = 1;
	}

	if (is_sharedmem)
	{
		dart_myid (&unitid);
		if (flags == 1)
		{
			if (dart_adapt_transtable_get_win (index, offset, &base, &win) == -1)
			{
				return DART_ERR_INVAL;
			}
		}

		if (unitid == target_unitid_abs)
		{
			if (flags == 1)
			{
				int flag;
				MPI_Win_get_attr (win, MPI_WIN_BASE, &baseptr, &flag);
		//		dart_adapt_transtable_get_addr (index, offset, &begin, &baseptr);
				baseptr = baseptr + (offset - base);
			}
			else
			{
				baseptr = offset + dart_mempool_localalloc;
			}
		}
		else
		{/* Accesses through shared memory (load)*/
			if (flags == 1)
			{			
				disp_rel = offset - base;
			}
			else
			{
				win = dart_sharedmem_win_local_alloc;
				disp_rel = offset;
			}
			MPI_Win_shared_query (win, i, &maximum_size, &disp_unit, &baseptr);
			baseptr += disp_rel;
		}

		memcpy ((char*)dest, baseptr, nbytes);	
	}
	else
	{
		if (flags == 1)
		{
			win = dart_win_lists[index];
			dart_team_unit_g2l (teamid, target_unitid_abs, &target_unitid_rel);
			if (dart_adapt_transtable_get_disp (index, offset, target_unitid_rel, &base, &disp_s) == -1)
			{
				return DART_ERR_INVAL;
			}
			disp_rel = disp_s + (offset - base);
		}
		else
		{
			win = dart_win_local_alloc;
			disp_rel = offset;
			target_unitid_rel = target_unitid_abs;
		}

	//	MPI_Get (dest, nbytes, MPI_BYTE, target_unitid, offset, nbytes, MPI_BYTE, win);
		MPI_Rget (dest, nbytes, MPI_BYTE, target_unitid_rel, disp_rel, nbytes, MPI_BYTE, win, &mpi_req);
		
		MPI_Wait (&mpi_req, &mpi_sta);
	}
	if (flags == 1)
	{
		DEBUG ("GET_BLOCKING	- %d bytes (allocated with collective allocation) from %d at the offset %d", 
				nbytes, target_unitid_abs, offset);
	}
	else 
	{	
		DEBUG ("GET_BLOCKING - %d bytes (allocated with local allocation) from %d at the offset %d", 
				nbytes, target_unitid_abs, offset);
	}
	return DART_OK;
}


dart_ret_t dart_wait (dart_handle_t handle)
{
	if (handle)
	{
		MPI_Status mpi_sta;
		MPI_Wait (&(handle -> request), &mpi_sta);
	}

	handle = NULL;
	free (handle);
	
	LOG ("WAIT	- finished");
	return DART_OK;
}

dart_ret_t dart_test (dart_handle_t handle, int32_t* is_finished)
{
	if (!handle)
	{
		*is_finished = 1;
		return DART_OK;
	}

	MPI_Status mpi_sta;
	MPI_Test (&(handle -> request), is_finished, &mpi_sta);

	LOG ("TEST	- finished");
	return DART_OK;
}


dart_ret_t dart_waitall (dart_handle_t *handle, size_t n)
{
	int i;
	if (*handle)
	{
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
			handle[i] -> request = mpi_req[i];
		}
		free (mpi_req);
		free (mpi_sta);
	}	
	for (i = 0; i < n; i++)
	{
		handle[i] = NULL;
		free(handle[i]);
	}
	
	LOG ("WAITALL	- finished");
	return DART_OK;
}

/** TODO:	Rectify its return val type or add another new param in which returns the testing result.\n 
 *  FIX:	Adding a flag param here.
 */
dart_ret_t dart_testall (dart_handle_t *handle, size_t n, int32_t* is_finished)
{
	if (!(*handle))
	{
		*is_finished = 1;
		return DART_OK;
	}

	int i;
	MPI_Status *mpi_sta;
	MPI_Request *mpi_req;
	mpi_req = (MPI_Request *)malloc (n * sizeof (MPI_Request));
	mpi_sta = (MPI_Status *)malloc (n * sizeof (MPI_Status));

	for (i = 0; i < n; i++)
	{
		mpi_req [i] = handle[i] -> request;
	}
	MPI_Testall (n, mpi_req, is_finished, mpi_sta);
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
	comm = dart_teams[index];	
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

	comm = dart_teams[index];
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
	comm = dart_teams[index];

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
	comm = dart_teams[index];

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
	comm = dart_teams[index];

	return MPI_Allgather (sendbuf, nbytes, MPI_BYTE, recvbuf, nbytes, MPI_BYTE, comm);
}
