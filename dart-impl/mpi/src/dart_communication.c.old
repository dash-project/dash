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

int unit_g2l (uint16_t index, dart_unit_t abs_id, dart_unit_t *rel_id)
{
	if (index == 0)
	{
		*rel_id = abs_id;
	}
	else
	{
		MPI_Comm comm;
		MPI_Group group, group_all;
		comm = dart_teams[index];
		MPI_Comm_group (comm, &group);
		MPI_Comm_group (MPI_COMM_WORLD, &group_all);
		MPI_Group_translate_ranks (group_all, 1, &abs_id, group, rel_id);
	}
	return 0;
}

/* -- Non-blocking dart one-sided operations -- */
dart_ret_t dart_get (void *dest, dart_gptr_t gptr, size_t nbytes, dart_handle_t *handle)
{
	MPI_Request mpi_req;
	MPI_Aint disp_s, disp_rel;
	dart_unit_t target_unitid_abs;
	uint64_t offset = gptr.addr_or_offs.offset;
	int16_t seg_id = gptr.segid;
	MPI_Win win;

	*handle = (dart_handle_t) malloc (sizeof (struct dart_handle_struct));
	target_unitid_abs = gptr.unitid;
	
	/* The memory accessed is allocated with collective allocation. */
	if (seg_id)
	{
		uint16_t index = gptr.flags;
	        dart_unit_t target_unitid_rel;
	        
		/*
	        if (dart_adapt_transtable_get_win (index, offset, &begin, &win) == -1)
		{
			ERROR ("Invalid accessing operation");
			return DART_ERR_INVAL;
		}
	        */

		win = dart_win_lists[index];
		
		/* Translate local unitID (relative to teamid) into global unitID 
		 * (relative to DART_TEAM_ALL).
		 *
		 * Note: target_unitid should not be the global unitID but rather the local 
		 * unitID relative to the team associated with the specified win object.
		 */
		unit_g2l (index, target_unitid_abs, &target_unitid_rel);

		if (dart_adapt_transtable_get_disp (seg_id, target_unitid_rel, &disp_s)== -1)
		{
			return DART_ERR_INVAL;
		}

		disp_rel = disp_s + offset;
		 
		/* MPI-3 newly added feature: request version of get call. */

		/** TODO: Check if MPI_Rget_accumulate (NULL, 0, MPI_BYTE, dest, nbytes, MPI_BYTE, 
		 *  target_unitid, disp_rel, nbytes, MPI_BYTE, MPI_NO_OP, win, &mpi_req) 
		 *  could be an better alternative? 
		 */
		MPI_Rget (dest, nbytes, MPI_BYTE, target_unitid_rel, disp_rel, nbytes, MPI_BYTE, win, &mpi_req);

		DEBUG ("GET	- %d bytes (allocated with collective allocation) from %d at the offset %d", 
				nbytes, target_unitid_abs, offset);
	}

	/* The memory accessed is allocated with local allocation. */
	else
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
	MPI_Request mpi_req;
	MPI_Aint disp_s, disp_rel;
	dart_unit_t target_unitid_abs;
	uint64_t offset = gptr.addr_or_offs.offset;
	int16_t seg_id = gptr.segid;
	MPI_Win win;
	
	*handle = (dart_handle_t) malloc (sizeof (struct dart_handle_struct));
	target_unitid_abs = gptr.unitid;

	if (seg_id)
	{
		uint16_t index = gptr.flags;
	        dart_unit_t target_unitid_rel;

		/*
		if (dart_adapt_transtable_get_win (index, offset, &begin, &win) == -1)
		{
			ERROR ("Invalid accessing operation");
			return DART_ERR_INVAL;
		}
		*/

		win = dart_win_lists[index];		
		unit_g2l (index, target_unitid_abs, &target_unitid_rel);
		if (dart_adapt_transtable_get_disp (seg_id, target_unitid_rel, &disp_s) == -1)
		{
			return DART_ERR_INVAL;
		}	

		disp_rel = disp_s + offset;
		/** TODO: Check if MPI_Raccumulate (src, nbytes, MPI_BYTE, target_unitid, disp_rel,
		 *  nbytes, MPI_BYTE,
		 *  REPLACE, win, &mpi_req) could be a better alternative? 
		 */
		MPI_Rput (src, nbytes, MPI_BYTE, target_unitid_rel, disp_rel, nbytes, MPI_BYTE, win, &mpi_req);
		DEBUG ("PUT	-%d bytes (allocated with collective allocation) to %d at the offset %d",
				nbytes, target_unitid_abs, offset);

	}
	else
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
	MPI_Win win;
	MPI_Status mpi_sta;
	MPI_Request mpi_req;
	MPI_Aint disp_s, maximum_size, disp_rel;

	uint64_t offset = gptr.addr_or_offs.offset;
	int16_t seg_id = gptr.segid;
	uint16_t index = gptr.flags;
	dart_unit_t unitid, target_unitid_rel, target_unitid_abs = gptr.unitid;

	int disp_unit;
	char *baseptr;


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
		if (seg_id)
		{
			if (dart_adapt_transtable_get_win (seg_id, &win) == -1)
			{
				return DART_ERR_INVAL;
			}
		}
		else
		{
			win = dart_sharedmem_win_local_alloc;
		}
		
		if (unitid == target_unitid_abs)
		{/* If orgin and target are identical, then switches to local access. */
			if (seg_id)
			{
				int flag;
				
				MPI_Win_get_attr (win, MPI_WIN_BASE, &baseptr, &flag);
				baseptr = baseptr + offset;
			}
			else
			{
				baseptr = offset + dart_mempool_localalloc;
			}
		}
		else
		{/* Accesses through shared memory (store). */
			disp_rel = offset;
			MPI_Win_shared_query (win, i, &maximum_size, &disp_unit, &baseptr);
			baseptr += disp_rel;
		}
		memcpy (baseptr, ((char*)src), nbytes);
	}
	else
	{/* The traditional remote access method */
		if (seg_id)
		{	
			win = dart_win_lists[index];
			unit_g2l (index, target_unitid_abs, &target_unitid_rel);
			if (dart_adapt_transtable_get_disp (seg_id, target_unitid_rel, &disp_s) == -1)
			{
				return DART_ERR_INVAL;
			}	
			disp_rel = disp_s + offset;
		}
		else
		{
			win = dart_win_local_alloc;
			disp_rel = offset;
			target_unitid_rel = target_unitid_abs;
		}
	
//		MPI_Put (src, nbytes, MPI_BYTE, target_unitid, difference, nbytes, MPI_BYTE, win);
		MPI_Rput (src, nbytes, MPI_BYTE, target_unitid_rel, disp_rel, nbytes, MPI_BYTE, win, &mpi_req);
		/* Make sure the access is completed remotedly */
		MPI_Win_flush (target_unitid_rel, win);
		/* MPI_Wait is invoked to release the resource brought by the mpi request handle */
		MPI_Wait (&mpi_req, &mpi_sta);
	}
	
	if (seg_id)
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
	int i, is_sharedmem = 0;
	MPI_Win win;
	MPI_Status mpi_sta;
	MPI_Request mpi_req;
	MPI_Aint disp_s, maximum_size, disp_rel;
	

	uint64_t offset = gptr.addr_or_offs.offset;
	int16_t seg_id = gptr.segid;
	uint16_t index  = gptr.flags;
	dart_unit_t unitid, target_unitid_rel, target_unitid_abs = gptr.unitid;
	
	int disp_unit;
	char* baseptr;

//	i = binary_search (dart_unit_mapping[j], gptr.unitid, 0, dart_sharedmem_size[j] - 1);

	/* Check whether the target is in the same node as the calling unit or not. */
	i = dart_sharedmem_table[index][gptr.unitid];
	if (i >= 0)
	{
		is_sharedmem = 1;
	}

	if (is_sharedmem)
	{
		dart_myid (&unitid);
		if (seg_id)
		{
			if (dart_adapt_transtable_get_win (seg_id, &win) == -1)
			{
				return DART_ERR_INVAL;
			}
		}
		else
		{
			win = dart_sharedmem_win_local_alloc;
		}

		if (unitid == target_unitid_abs)
		{
			if (seg_id)
			{
				int flag;
				MPI_Win_get_attr (win, MPI_WIN_BASE, &baseptr, &flag);
				baseptr = baseptr + offset;
			}
			else
			{
				baseptr = offset + dart_mempool_localalloc;
			}
		}
		else
		{/* Accesses through shared memory (load)*/
			disp_rel = offset;
			MPI_Win_shared_query (win, i, &maximum_size, &disp_unit, &baseptr);
			baseptr += disp_rel;
		}

		memcpy ((char*)dest, baseptr, nbytes);	
	}
	else
	{
		if (seg_id)
		{
			win = dart_win_lists[index];
			unit_g2l (index, target_unitid_abs, &target_unitid_rel);
			if (dart_adapt_transtable_get_disp (seg_id, target_unitid_rel, &disp_s) == -1)
			{
				return DART_ERR_INVAL;
			}
			disp_rel = disp_s + offset;
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
	if (seg_id)
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
		handle = NULL;
		free (handle);
	}

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

	if (*is_finished)
	{
		handle = NULL;
		free (handle);
	}

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
		
		for (i = 0; i < n; i++)
		{
			handle[i] = NULL;
			free(handle[i]);
		}
	}
	
	LOG ("WAITALL	- finished");
	return DART_OK;
}

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
	/* If the tested operations are finished, then release the handler resource */
	if (*is_finished)
	{
		for (i = 0; i < n; i++)
		{
			handle[i] = NULL;
			free (handle[i]);
		}

	}
	LOG ("TESTALL	- finished");
	return DART_OK;
}

/* -- Dart collective operations -- */

dart_ret_t dart_barrier (dart_team_t teamid)
{
	MPI_Comm comm;	
	uint16_t index;
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
	uint16_t index;
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
	uint16_t index;
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
	uint16_t index;
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
	uint16_t index;
	int result = dart_adapt_teamlist_convert (teamid, &index);
	if (result == -1)
	{
		return DART_ERR_INVAL;
	}
	comm = dart_teams[index];

	return MPI_Allgather (sendbuf, nbytes, MPI_BYTE, recvbuf, nbytes, MPI_BYTE, comm);
}

dart_ret_t dart_reduce (double *sendbuf, double *recvbuf, dart_team_t teamid)
{
	MPI_Comm comm;
	uint16_t index;
	int result = dart_adapt_teamlist_convert (teamid, &index);
	if (result == -1)
	{
		return DART_ERR_INVAL;
	}
	comm = dart_teams[index];

	return MPI_Reduce (sendbuf, recvbuf, 1, MPI_DOUBLE, MPI_MAX, 0, comm);
}
