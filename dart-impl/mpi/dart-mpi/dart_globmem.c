/** @file dart_adapt_globmem.c
 *  @date 25 Mar 2014
 *  @brief Implementation of all the related global pointer operations
 *
 *  All the following functions are implemented with the underlying *MPI-3*
 *  one-sided runtime system.
 */

#include "dart_deb_log.h"
#ifndef ENABLE_DEBUG
#define ENABLE_DEBUG
#endif
#include <stdio.h>
#include <mpi.h>
#include "dart_types.h"
#include "dart_mem.h"
#include "dart_translation.h"
#include "dart_team_private.h"
#include "dart_globmem.h"
#include "dart_team_group.h"
#include "dart_communication.h"

/**
 * @note For dart collective allocation/free: offset in the returned gptr represents the displacement
 * relative to the base address of memory region reserved for certain team rather than 
 * the beginning of sub-memory spanned by certain dart collective allocation.
 * For dart local allocation/free: offset in the returned gptr represents the displacement relative to 
 * the base address of memory region reserved for the dart local allocation/free.
 */
dart_ret_t dart_gptr_getaddr (const dart_gptr_t gptr, void **addr)
{
	int flag = gptr.flags;
	uint64_t offset;
	offset = gptr.addr_or_offs.offset;

	if (flag == 1)
	{
		int index;
		int result = dart_adapt_teamlist_convert (gptr.segid, &index);
		*addr = offset + mempool_globalalloc[index];	
	}
	else
	{
		*addr = offset + mempool_localalloc;
	}

	return DART_OK;
}

dart_ret_t dart_gptr_setaddr (dart_gptr_t* gptr, void* addr)
{
	int flag = gptr->flags;

	/* The modification to addr is reflected in the fact that modifying the offset. */
	if (flag == 1)
	{
		int index;
		int result = dart_adapt_teamlist_convert (gptr -> segid, &index);
		gptr->addr_or_offs.offset = (char *)addr - mempool_globalalloc[index];
	}
	else
	{
		gptr->addr_or_offs.offset = (char *)addr - mempool_localalloc;
	}

	return DART_OK;
}

dart_ret_t dart_gptr_incaddr (dart_gptr_t* gptr, int offs)
{
	gptr -> addr_or_offs.offset += offs;
	return DART_OK;
}

#if 0
dart_ret_t dart_gptr_setunit (dart_gptr_t* gptr, dart_unit_t unit_id)
{
	gptr->unitid = unit_id;
}
#endif 

dart_ret_t dart_memalloc (size_t nbytes, dart_gptr_t *gptr)
{
	dart_unit_t unitid;
	dart_myid (&unitid);
	gptr->unitid = unitid;
	gptr->segid = MAX_TEAM_NUMBER;
	gptr->flags = 0; /* For local allocation, the flag is marked as '0'. */
	gptr->addr_or_offs.offset = dart_mempool_alloc (localpool, nbytes);
	DEBUG ("%2d: LOCALALLOC	- %d bytes, offset = %d", unitid, nbytes, gptr->addr_or_offs.offset);
	return DART_OK;
}

dart_ret_t dart_memfree (dart_gptr_t gptr)
{	
	if (dart_mempool_free (localpool, gptr.addr_or_offs.offset) == -1)
	{
		ERROR ("Out of bound: the global memory is exhausted");
		return DART_ERR_OTHER;
	}
	DEBUG ("%2d: LOCALFREE	- offset = %d", gptr.unitid, gptr.addr_or_offs.offset);
	return DART_OK;
}

dart_ret_t dart_team_memalloc_aligned (dart_team_t teamid, size_t nbytes, dart_gptr_t *gptr)
{
 	dart_unit_t unitid, gptr_unitid = -1;
	dart_team_myid(teamid, &unitid);
		
	int offset;

	/* The units belonging to the specified team are eligible to participate
	 * below codes enclosed. */
	 
	MPI_Win win;
	MPI_Comm comm;
        int index;
	int result = dart_adapt_teamlist_convert (teamid, &index);
	if (result == -1)
	{
		return DART_ERR_INVAL;
	}
	comm = teams[index];
	if (unitid == 0)
	{
	/* Get the adequate available memory region from the memory region
	 * reserved for unitid 0 in teamid. */
		offset = dart_mempool_alloc (globalpool[index], nbytes); 
		if (offset == -1)
		{
			ERROR ("Out of bound: the global memory is exhausted");
			return DART_ERR_OTHER;
		}
	}
	MPI_Bcast (&offset, 1, MPI_INT, 0, comm);
	
	dart_team_unit_l2g (teamid, 0, &gptr_unitid);
	
	MPI_Win_create (mempool_globalalloc[index] + offset, nbytes, sizeof (char), MPI_INFO_NULL, comm, &win);
		
	/* -- Updating infos on gptr -- */
	gptr->unitid = gptr_unitid;
	gptr->segid = teamid; /* Segid equals to teamid, identifies a team uniquely from certain unit point of view. */
	gptr->flags = 1; /* For collective allocation, the flag is marked as '1'. */
	gptr->addr_or_offs.offset = offset;

	/* -- Updating the translation table of teamid with the created (offset, win) infos -- */
	info_t item;
	item.offset = offset;
	item.size = nbytes;
        GMRh handler;
        handler.win = win;
	item.handle = handler;
		
	/* Add this newly generated correspondence relationship record into the translation table. */
	dart_adapt_transtable_add (index, item);
        DEBUG ("%2d: COLLECTIVEALLOC	-  %d bytes, offset = %d, gptr_unitid = %d across team %d",
			     unitid, nbytes, offset, gptr_unitid, teamid);
	        
	/* Start a shared access epoch to all the units of the specified teamid*/
	MPI_Win_lock_all (0, handler.win); 
	
	return DART_OK;
}

dart_ret_t dart_team_memfree (dart_team_t teamid, dart_gptr_t gptr)
{		
	dart_unit_t unitid;
       	dart_team_myid (teamid, &unitid);
	int index;
	int result = dart_adapt_teamlist_convert (teamid, &index);
	if (result == -1)
	{
		return DART_ERR_INVAL;
	}
	
	MPI_Win win;
	int begin;
	int offset = gptr.addr_or_offs.offset;
		
	/* Query the win info from related translation table
	 * according to the given offset. */
	dart_adapt_transtable_query (index, offset, &begin, &win);
		
	/* End the shared access epoch to all the units of the specified teamid. */
	MPI_Win_unlock_all (win); 		
	
     	
	/** TODO: is this barrier needed really?
	 */
	dart_barrier (teamid);
	if (unitid == 0)
	{
		if (dart_mempool_free (globalpool[index], gptr.addr_or_offs.offset) == -1)
		{
			ERROR ("Invalid offset input, can't remove global memory from the memory pool");
			return DART_ERR_INVAL;
		}
	}
	
	DEBUG ("%2d: COLLECTIVEFREE	- offset = %d, gptr_unitid = %d across team %d", 
			unitid, gptr.addr_or_offs.offset, gptr.unitid, teamid);

	/* Remove the related correspondence relation record from the related translation table. */
	if (dart_adapt_transtable_remove (index, gptr.addr_or_offs.offset) == -1)
	{
		ERROR ("Invalid offset input, can't remove record from translation table");
		return DART_ERR_INVAL;
	}
	
	return DART_OK;
}



