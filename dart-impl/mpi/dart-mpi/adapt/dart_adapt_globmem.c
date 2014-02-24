/** @file dart_adapt_globmem.c
 *  @date 20 Nov 2013
 *  @brief Implementation of all the related global pointer operations
 *
 *  All the following functions are implemented with the underlying *MPI-3*
 *  one-sided runtime system.
 */

#ifndef ENABLE_DEBUG
#define ENABLE_DEBUG
#endif
#include <stdio.h>
#include <mpi.h>
#include "dart_types.h"
#include "dart_adapt_mem.h"
#include "dart_adapt_translation.h"
#include "dart_adapt_teamnode.h"
#include "dart_adapt_globmem.h"
#include "dart_adapt_team_group.h"
#include "mpi_adapt_team_private.h"
#include "dart_adapt_communication.h"

/**
 * @note For dart collective allocation/free: offset in the returned gptr represents the displacement
 * relative to the base address of memory region reserved for certain team rather than 
 * the beginning of sub-memory spanned by certain dart collective allocation.
 * For dart local allocation/free: offset in the returned gptr represents the displacement relative to 
 * the base address of memory region reserved for the dart local allocation/free.
 */
dart_ret_t dart_adapt_gptr_getaddr (const dart_gptr_t gptr, void **addr)
{
    	int unique_id = gptr.segid;
	int flag = gptr.flags;
	uint64_t offset;
	offset = gptr.addr_or_offs.offset;
	if (flag == 1)
	{
		*addr = offset + mempool_globalalloc[unique_id];	
	}
	else
	{
		*addr = offset + mempool_localalloc;
	}
}

dart_ret_t dart_adapt_gptr_setaddr (dart_gptr_t* gptr, void* addr)
{
	int unique_id = gptr->segid;
	int flag = gptr->flags;

	/* The modification to addr is reflected in the fact that modifying the offset. */
	if (flag == 1)
	{
		gptr->addr_or_offs.offset = (char *)addr - mempool_globalalloc[unique_id];
	}
	else
	{
		gptr->addr_or_offs.offset = (char *)addr - mempool_localalloc;
	}
}

#if 0
dart_ret_t dart_gptr_setunit (dart_gptr_t* gptr, dart_unit_t unit_id)
{
	gptr->unitid = unit_id;
}
#endif 

dart_ret_t dart_adapt_memalloc (size_t nbytes, dart_gptr_t *gptr)
{
	dart_unit_t unitid;
	dart_myid (&unitid);
	gptr->unitid = unitid;
	gptr->segid = MAX_TEAM_NUMBER;
	gptr->flags = 0; /* For local allocation, the flag is marked as '0'. */
	gptr->addr_or_offs.offset = dart_mempool_alloc (localpool, nbytes);
	debug_print ("%2d: LOCALALLOC	- %d bytes, offset = %d \n", unitid, nbytes, gptr->addr_or_offs.offset);
}

dart_ret_t dart_adapt_memfree (dart_gptr_t gptr)
{	
	dart_mempool_free (localpool, gptr.addr_or_offs.offset);
	debug_print ("%2d: LOCALFREE	- offset = %d \n", gptr.unitid, gptr.addr_or_offs.offset);
}

dart_ret_t dart_adapt_team_memalloc_aligned (dart_team_t teamid, size_t nbytes, dart_gptr_t *gptr)
{
 	dart_unit_t unitid;
	dart_adapt_team_myid(teamid, &unitid);
	int offset;

	/* Only the units belonging to the specified team are eligible to participate
	 * below codes enclosed by 'if (id >= 0)'. */
	if (unitid >= 0)
	{ 
		MPI_Win win;
		MPI_Comm comm;
                int unique_id;
		dart_adapt_team_uniqueid (teamid, &unique_id);
		dart_teamnode_t val;
		dart_adapt_teamnode_query (teamid, &val);
		comm = val -> mpi_comm;
	        if (unitid == 0)
		{
			/* Get the adequate available memory region from the memory region
			 * reserved for unitid 0 in teamid. */
			offset = dart_mempool_alloc (globalpool[unique_id], nbytes); 
		}
		MPI_Bcast (&offset, 1, MPI_INT, 0, comm);
	
		MPI_Win_create (mempool_globalalloc[unique_id] + offset, nbytes, sizeof (char), MPI_INFO_NULL, comm, &win);
		
		/* -- Updating infos on gptr -- */
		gptr->unitid = 0;
		gptr->segid = unique_id;
		gptr->flags = 1; /* For collective allocatioin, the flag is marked as '1'. */
		gptr->addr_or_offs.offset = offset;

		/* -- Updating the translation table of teamid with the created (offset, win) infos -- */
	        info_t item;
		item.offset = offset;
                GMRh handler;
                handler.win = win;
		item.handle = handler;
		
		/* Add this newly generated correspondence relationship record into the translation table. */
		dart_adapt_transtable_add (unique_id, item);
        	debug_print ("%2d: COLLECTIVEALLOC	-  %d bytes, offset = %d across team %d \n",
			       	unitid, nbytes, offset, teamid.team_id);
	        
		/* Start a shared access epoch to all the units of the specified teamid*/
		MPI_Win_lock_all (0, handler.win); 
	}
	else 
	{
		gptr->addr_or_offs.offset = -1;
		gptr->unitid = unitid;
		gptr->segid = -1;
		gptr->flags = -1;
	}
}

dart_ret_t dart_adapt_team_memfree (dart_team_t teamid, dart_gptr_t gptr)
{		
	dart_unit_t unitid;
       	dart_adapt_team_myid (teamid, &unitid);
	int unique_id;
	unique_id = gptr.segid;
	if (unitid >= 0)
	{
		MPI_Win win;
		int begin;
		int offset = gptr.addr_or_offs.offset;
		
		/* Query the win info from related translation table
		 * according to the given offset*/
		dart_adapt_transtable_query (unique_id, offset, &begin, &win);
		
		/* End the shared access epoch to all the units of the specified teamid. */
		MPI_Win_unlock_all (win); 		
	}
     	
	/** TODO: is this barrier needed really?
	 */
	dart_adapt_barrier (teamid);
	if (unitid == 0)
	{
		dart_mempool_free (globalpool[unique_id], gptr.addr_or_offs.offset);
	}
	if (unitid >= 0)
	{
		debug_print ("%2d: COLLECTIVEFREE	- offset = %d across team %d \n", 
				unitid, gptr.addr_or_offs.offset, teamid.team_id);

		/* Remove the related correspondence relation record from the translation table. */
		dart_adapt_transtable_remove (unique_id, gptr.addr_or_offs.offset);
	}
}



