/** @file dart_adapt_synchronization.c
 *  @date 20 Nov 2013
 *  @brief synchronization operations.
 */

#ifndef ENABLE_DEBUG
#define ENABLE_DEBUG
#endif
#include <stdio.h>
#include <malloc.h>
#include "dart_types.h"
#include "dart_adapt_translation.h"
#include "dart_adapt_mem.h"
#include "dart_adapt_globmem.h"
#include "dart_adapt_teamnode.h"
#include "dart_adapt_team_group.h"
#include "dart_adapt_communication.h"
#include "dart_adapt_synchronization.h"

dart_ret_t dart_adapt_team_lock_init (dart_team_t teamid, dart_lock_t* lock)
{
	dart_gptr_t gptr_tail;
	dart_gptr_t gptr_list;
	dart_unit_t    unitid;
	int *addr, *next;

	dart_teamnode_t p;
	dart_adapt_teamnode_query (teamid, &p);
	MPI_Comm comm = p -> mpi_comm;

	dart_adapt_team_myid (teamid, &unitid);
	*lock = (dart_lock_t) malloc (sizeof (struct dart_lock_struct));
		

	/* Unit 0 is the process holding the gptr_tail by default. */
	if (unitid == 0)
	{
		dart_adapt_memalloc (sizeof (int), &gptr_tail);
		dart_adapt_gptr_getaddr (gptr_tail, &addr);
		
		/* Local store is safe and effective followed by the sync call. */
		*addr = -1;
		MPI_Win_sync (win_local_alloc);
	}

	if (unitid >= 0)
	{
		MPI_Comm_dup (comm, &((*lock) -> comm));
		dart_adapt_bcast (&gptr_tail, sizeof (dart_gptr_t), 0, teamid);
		
		/* Create a global memory region across the teamid, and every local memory segment related certain unit
		 * hold the next blocking unit info waiting on the lock. */
		dart_adapt_team_memalloc_aligned (teamid, sizeof(int), &gptr_list);
		int begin;
		MPI_Win win;
		dart_adapt_transtable_query (gptr_list.segid, gptr_list.addr_or_offs.offset, &begin, &win);
		dart_adapt_gptr_getaddr (gptr_list, &addr);
		*addr = -1;
		MPI_Win_sync (win);
		DART_GPTR_COPY ((*lock) -> gptr_tail, gptr_tail);
		DART_GPTR_COPY ((*lock) -> gptr_list, gptr_list);
		((*lock) -> teamid).parent_id = teamid.parent_id;
		((*lock) -> teamid).team_id = teamid.team_id;
		((*lock) -> teamid).level = teamid.level;
		
		(*lock) -> win = win;
		(*lock) -> acquired = 0;
			
		debug_print ("%2d: INIT	- done\n", unitid);
	}
	return DART_OK;
}

dart_ret_t dart_adapt_lock_acquire (dart_lock_t lock)
{
	dart_unit_t unitid;
	dart_adapt_team_myid (lock -> teamid, &unitid);

	if (lock -> acquired == 1)
	{
		printf ("Warning: LOCK	- %2d has acquired the lock already\n", unitid);
		return -1;
	}

	dart_gptr_t gptr_tail;
	dart_gptr_t gptr_list;
	int predecessor[1], result[1];
	
	MPI_Win win;
	MPI_Status status;
	DART_GPTR_COPY (gptr_tail, lock -> gptr_tail);
	
	DART_GPTR_COPY (gptr_list, lock -> gptr_list);

	int offset = gptr_tail.addr_or_offs.offset;
	int tail = gptr_tail.unitid;
	
	/* MPI-3 newly added feature: atomic operation*/
	MPI_Fetch_and_op (&unitid, predecessor, MPI_INT, tail, offset, MPI_REPLACE, win_local_alloc);
	
	MPI_Win_flush (tail, win_local_alloc);

	
	/* If there was a previous tail (predecessor), update the previous tail's next pointer with unitid
	 * and wait for notification from its predecessor. */
	if (*predecessor != -1)
	{
		win = lock -> win;	
			
		/* Atomicity: Update its predecessor's next pointer */
		MPI_Fetch_and_op (&unitid, result, MPI_INT, *predecessor, DART_LOCK_TAIL_DISP, MPI_REPLACE, win);
		MPI_Win_flush (*predecessor, win);
	

		/* Inform its predecessor after update */
		MPI_Send (NULL, 0, MPI_INT, *predecessor, 0, lock -> comm);
		
		/* Waiting for notification from its predecessor*/
		debug_print ("%2d: LOCK	- waiting for notification from %d in team %d\n", 
				unitid, *predecessor, (lock -> teamid).team_id);
		MPI_Recv (NULL, 0, MPI_INT, *predecessor, MPI_ANY_TAG, lock -> comm, &status);
	}
	
	debug_print ("%2d: LOCK	- lock required in team %d\n", unitid, (lock -> teamid).team_id);
	lock -> acquired = 1;
	return DART_OK;
}

dart_ret_t dart_adapt_lock_try_acquire (dart_lock_t lock, int *success)
{
	dart_unit_t unitid;
	dart_adapt_team_myid (lock -> teamid, &unitid);
	if (lock -> acquired == 1)
	{
		printf ("Warning: TRYLOCK	- %2d has acquired the lock already\n", unitid);
		return -1;
	}
	dart_gptr_t gptr_tail;
	dart_gptr_t gptr_list;

	int result[1];
	int compare[1] = {-1};
	MPI_Comm comm;
	MPI_Status status;
	DART_GPTR_COPY (gptr_tail, lock -> gptr_tail);
	DART_GPTR_COPY (gptr_tail, lock -> gptr_list);
	int offset = gptr_tail.addr_or_offs.offset;
	int tail = gptr_tail.unitid;
	
	/* Atomicity: Check if the lock is available and claim it if it is. */
 	MPI_Compare_and_swap (&unitid, compare, result, MPI_INT, tail, offset, win_local_alloc);
	MPI_Win_flush (tail, win_local_alloc);

	/* If the old predecessor was -1, we will claim the lock, otherwise, do nothing. */
	if (*result == -1)
	{
		lock -> acquired = 1;
		*success = 1;
	}
	else
	{
		*success = 0;
	}
	char* string = (*success) ? "success" : "Non-success";
	debug_print ("%2d: TRYLOCK	- %s in team %d\n", unitid, string, (lock -> teamid).team_id);
	return DART_OK;
}

dart_ret_t dart_adapt_lock_release (dart_lock_t lock)
{
	dart_unit_t unitid;
	dart_adapt_team_myid (lock -> teamid, &unitid);
	if (lock -> acquired == 0)
	{
		printf ("Warning: RELEASE	- %2d has not yet required the lock\n", unitid);
		return -1;
	}
	dart_gptr_t gptr_tail;
	dart_gptr_t gptr_list;
	MPI_Status mpi_status;
	MPI_Win win;
	int successor, *addr2, result[1];
	int origin[1] = {-1};

	DART_GPTR_COPY (gptr_tail, lock -> gptr_tail);
	DART_GPTR_COPY (gptr_list, lock -> gptr_list);

	int offset_list = gptr_list.addr_or_offs.offset;
	int offset_tail = gptr_tail.addr_or_offs.offset;
	int tail = gptr_tail.unitid;
	dart_adapt_gptr_getaddr (gptr_list, &addr2);
	win = lock -> win;
	
	/* Atomicity: Check if we are at the tail of this lock queue, if so, we are done.
	 * Otherwise, we still need to send notification. */
	MPI_Compare_and_swap (origin, &unitid, result, MPI_INT, tail, offset_tail, win_local_alloc);
	MPI_Win_flush (tail, win_local_alloc);
		
	/* We are not at the tail of this lock queue. */
	if (*result != unitid)
	{
		debug_print ("%2d: UNLOCK	- waiting for next pointer (tail = %d) in team \n", 
				unitid, *result, (lock -> teamid).team_id);
		/* Waiting for the update of my next pointer finished. */
		MPI_Recv (NULL, 0, MPI_INT, *result, 0, lock -> comm, &mpi_status);


		debug_print ("%2d: UNLOCK	- notifying %d in team %d \n", unitid, *addr2, (lock -> teamid).team_id);
		
		/* Notifying the next unit waiting on the lock queue. */
		MPI_Send (NULL, 0, MPI_INT, *addr2, 0, lock -> comm);
		*addr2 = -1;
		MPI_Win_sync (win);
	}
	lock -> acquired = 0;
	debug_print ("%2d: UNLOCK	- release lock in team %d\n", unitid, (lock -> teamid).team_id);

	return DART_OK;
}

dart_ret_t dart_adapt_team_lock_free (dart_team_t teamid, dart_lock_t* lock)
{
	dart_gptr_t gptr_tail;
	dart_gptr_t gptr_list;
	dart_unit_t unitid;
	DART_GPTR_COPY (gptr_tail, (*lock) -> gptr_tail);
	DART_GPTR_COPY (gptr_list, (*lock) -> gptr_list);
	
	dart_adapt_team_myid (teamid, &unitid);
	if (unitid == 0)
	{
		dart_adapt_memfree (gptr_tail);
	}
	if (unitid >= 0)
	{
		dart_adapt_team_memfree (teamid, gptr_list);
		MPI_Comm_free (&((*lock) -> comm));
		debug_print ("%2d: Free	- done in team %d\n", unitid, teamid.team_id);
	}

	free (*lock);
	*lock = NULL;
	return DART_OK;	
}




