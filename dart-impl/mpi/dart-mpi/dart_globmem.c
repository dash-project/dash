/** @file dart_adapt_globmem.c
 *  @date 25 Mar 2014
 *  @brief Implementation of all the related global pointer operations
 *
 *  All the following functions are implemented with the underlying *MPI-3*
 *  one-sided runtime system.
 */

/*
#ifndef ENABLE_DEBUG
#define ENABLE_DEBUG
#endif
*/

#include "dart_deb_log.h"
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
	uint16_t flag = gptr.flags;
	uint64_t offset = gptr.addr_or_offs.offset;
	dart_unit_t myid;
	dart_myid (&myid);
  
	if (myid == gptr.unitid)
	{
		if (flag == 1)
		{
			int index, flag;
			uint64_t begin;
			MPI_Win win;
			int result = dart_adapt_teamlist_convert (gptr.segid, &index);
			if (dart_adapt_transtable_query_win (index, offset, &begin, &win) == -1)
			{
				return DART_ERR_INVAL;
			}
			MPI_Win_get_attr (win, MPI_WIN_BASE, addr, &flag);

			*addr = (offset - begin) + (char *)(*addr);
		}
		else
		{
			if (myid == gptr.unitid)
			{
				*addr = offset + dart_mempool_localalloc;
			}
		
		}
	}
	else
	{
		*addr = NULL;
	}
	
	return DART_OK;
}

dart_ret_t dart_gptr_setaddr (dart_gptr_t* gptr, void* addr)
{
	uint16_t flags = gptr->flags;

	/* The modification to addr is reflected in the fact that modifying the offset. */
	if (flags == 1)
	{
		MPI_Win win;
		char* addr_base;
		int index, flag;
		uint64_t begin;
		int result = dart_adapt_teamlist_convert (gptr -> segid, &index);
		if (result == -1)
		{
			return DART_ERR_INVAL;
		}
		if ((dart_adapt_transtable_query_win (index, gptr->addr_or_offs.offset, &begin, &win)) == -1)
		{
			return DART_ERR_INVAL;
		}
    		MPI_Win_get_attr (win, MPI_WIN_BASE, &addr_base, &flag);
		gptr->addr_or_offs.offset = ((char *)addr - addr_base) + begin;
	}
	else
	{
		gptr->addr_or_offs.offset = (char *)addr - dart_mempool_localalloc;
	}

	return DART_OK;
}

dart_ret_t dart_gptr_incaddr (dart_gptr_t* gptr, int offs)
{
	gptr -> addr_or_offs.offset += offs;
	return DART_OK;
}


dart_ret_t dart_gptr_setunit (dart_gptr_t* gptr, dart_unit_t unit_id)
{
	gptr->unitid = unit_id;
	return DART_OK;
}

dart_ret_t dart_memalloc (size_t nbytes, dart_gptr_t *gptr)
{
	dart_unit_t unitid;
	dart_myid (&unitid);
	gptr->unitid = unitid;
	gptr->segid = DART_MAX_TEAM_NUMBER;
	gptr->flags = 0; /* For local allocation, the flag is marked as '0'. */
	gptr->addr_or_offs.offset = dart_mempool_alloc (dart_localpool, nbytes);
	if (gptr->addr_or_offs.offset == -1)
	{
		ERROR ("Out of bound: the global memory is exhausted");
		return DART_ERR_OTHER;
	}
	DEBUG ("%2d: LOCALALLOC	- %d bytes, offset = %d", unitid, nbytes, gptr->addr_or_offs.offset);
	return DART_OK;
}

dart_ret_t dart_memfree (dart_gptr_t gptr)
{	
	if (dart_mempool_free (dart_localpool, gptr.addr_or_offs.offset) == -1)
	{
		ERROR ("Free invalid local global pointer: invalid offset = %llu\n", gptr.addr_or_offs.offset);
		return DART_ERR_INVAL;
	}
	DEBUG ("%2d: LOCALFREE	- offset = %llu", gptr.unitid, gptr.addr_or_offs.offset);
	return DART_OK;
}

dart_ret_t dart_team_memalloc_aligned (dart_team_t teamid, size_t nbytes, dart_gptr_t *gptr)
{
	size_t size;
	
 	dart_unit_t unitid, gptr_unitid = -1;
	dart_team_myid(teamid, &unitid);
	dart_team_size (teamid, &size);

	uint64_t offset;
	char* sub_mem;

	/* The units belonging to the specified team are eligible to participate
	 * below codes enclosed. */
	 
	MPI_Win win, numa_win;
	MPI_Comm comm, numa_comm;
	MPI_Aint disp;
	MPI_Aint* disp_set = (MPI_Aint*)malloc (size * sizeof (MPI_Aint));
	
	int index;
	int result = dart_adapt_teamlist_convert (teamid, &index);

	if (result == -1)
	{
		return DART_ERR_INVAL;
	}
	comm = dart_teams[index];
	numa_comm = dart_sharedmem_comm_list[index];

	if (unitid == 0)
	{
	/* Get the adequate offset from the infinite memory pool relating to teamid. */
		offset = dart_mempool_alloc (dart_globalpool[index], nbytes); 
		if (offset == -1)
		{
			ERROR ("Out of bound: the global memory is exhausted");
			return DART_ERR_OTHER;
		}
	}

	MPI_Bcast (&offset, 1, MPI_UINT64_T, 0, comm);
	
	dart_team_unit_l2g (teamid, 0, &gptr_unitid);

	MPI_Info win_info;
	MPI_Info_create (&win_info);
	MPI_Info_set (win_info, "alloc_shared_noncontig", "true");

	/* Allocate shared memory on numa_comm, and create the related numa_win */
	MPI_Win_allocate_shared (nbytes, sizeof (char), win_info, numa_comm, &sub_mem, &numa_win);
		

	win = dart_win_lists[index];
	/* Attach the allocated shared memory to win */
	MPI_Win_attach (win, sub_mem, nbytes);

	MPI_Get_address (sub_mem, &disp);

	/* Collect the disp information from all the ranks in comm */
	MPI_Allgather (&disp, 1, MPI_AINT, disp_set, 1, MPI_LONG, comm);

	/* -- Updating infos on gptr -- */
	gptr->unitid = gptr_unitid;
	gptr->segid = teamid; /* Segid equals to teamid, identifies a team uniquely from certain unit point of view. */
	gptr->flags = 1; /* For collective allocation, the flag is marked as '1'. */
	gptr->addr_or_offs.offset = offset;

	/* -- Updating the translation table of teamid with the created (offset, win) infos -- */
	info_t item;
	item.offset = offset;
	item.size = nbytes;
        item.disp = disp_set;
	GMRh handler;
        handler.win = numa_win;
	item.handle = handler;
		
	/* Add this newly generated correspondence relationship record into the translation table. */
	dart_adapt_transtable_add (index, item);

	MPI_Info_free (&win_info);
        DEBUG ("%2d: COLLECTIVEALLOC	-  %d bytes, offset = %d, gptr_unitid = %d across team %d",
			     unitid, nbytes, offset, gptr_unitid, teamid);
	
	return DART_OK;
}

dart_ret_t dart_team_memfree (dart_team_t teamid, dart_gptr_t gptr)
{		
	dart_unit_t unitid;
       	dart_team_myid (teamid, &unitid);
	int index;
	char* sub_mem;
	int result = dart_adapt_teamlist_convert (teamid, &index);
	if (result == -1)
	{
		return DART_ERR_INVAL;
	}
	
	MPI_Win win, numa_win;
	
	int flag;
	uint64_t begin, offset = gptr.addr_or_offs.offset;	
     	
	win = dart_win_lists[index];
        
	
	if (dart_adapt_transtable_query_win (index, offset, NULL, &numa_win) == -1)
	{
		return DART_ERR_INVAL;
	}

	/* Detach the freed sub-memory from win */
        MPI_Win_get_attr (numa_win, MPI_WIN_BASE, &sub_mem, &flag);
	MPI_Win_detach (win, sub_mem);

	/* Release the shared memory win object related to the freed shared momory */
	MPI_Win_free (&numa_win); 
	
	if (unitid == 0)
	{
		if (dart_mempool_free (dart_globalpool[index], offset) == -1)
		{
			ERROR ("Invalid offset input, can't remove global memory from the memory pool");
			return DART_ERR_INVAL;
		}
	}
	
	DEBUG ("%2d: COLLECTIVEFREE	- offset = %d, gptr_unitid = %d across team %d", 
			unitid, offset, gptr.unitid, teamid);

	/* Remove the related correspondence relation record from the related translation table. */
	if (dart_adapt_transtable_remove (index, offset) == -1)
	{
		return DART_ERR_INVAL;
	}
	
	return DART_OK;
}



