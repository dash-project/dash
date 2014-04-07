/** @file dart_adapt_synchronization.c
 *  @date 25 Mar 2014
 *  @brief synchronization operations.
 */

#include "dart_deb_log.h"
#ifndef ENABLE_DEBUG
#define ENABLE_DEBUG
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include "dart_types.h"
#include "dart_translation.h"
#include "dart_team_private.h"
#include "dart_mem.h"
#include "dart_globmem.h"
#include "dart_team_group.h"
#include "dart_communication.h"
#include "dart_synchronization.h"
#include "dart_synchronization_priv.h"



dart_ret_t dart_team_lock_init (dart_team_t teamid, dart_lock_t* lock)
{
	dart_gptr_t gptr_tail;
	dart_gptr_t gptr_list;
	dart_unit_t    unitid;
	int *addr, *next;

	MPI_Comm comm;
	int index;
	int result = dart_adapt_teamlist_convert (teamid, &index);
	if (result == -1)
	{
		return DART_ERR_INVAL;
	}
	comm = teams[index];

	dart_team_myid (teamid, &unitid);
	*lock = (dart_lock_t) malloc (sizeof (struct dart_lock_struct));
		

	/* Unit 0 is the process holding the gptr_tail by default. */
	if (unitid == 0)
	{
		dart_memalloc (sizeof (int), &gptr_tail);
		dart_gptr_getaddr (gptr_tail, &addr);
		
		/* Local store is safe and effective followed by the sync call. */
		*addr = -1;
		MPI_Win_sync (win_local_alloc);
	}

	
	MPI_Comm_dup (comm, &((*lock) -> comm));
	dart_bcast (&gptr_tail, sizeof (dart_gptr_t), 0, teamid);
		
	/* Create a global memory region across the teamid, and every local memory segment related certain unit
	 * hold the next blocking unit info waiting on the lock. */
	dart_team_memalloc_aligned (teamid, sizeof(int), &gptr_list);
	int begin;
	MPI_Win win;
	dart_adapt_transtable_query (index, gptr_list.addr_or_offs.offset, &begin, &win);
	dart_gptr_getaddr (gptr_list, &addr);
	*addr = -1;
	MPI_Win_sync (win);
	DART_GPTR_COPY ((*lock) -> gptr_tail, gptr_tail);
	DART_GPTR_COPY ((*lock) -> gptr_list, gptr_list);
	(*lock) -> teamid = teamid;		
	(*lock) -> win = win;
	(*lock) -> acquired = 0;
			
	DEBUG ("%2d: INIT	- done", unitid);
	
	return DART_OK;
}

dart_ret_t dart_lock_acquire (dart_lock_t lock)
{
	dart_unit_t unitid;
	dart_team_myid (lock -> teamid, &unitid);

	if (lock -> acquired == 1)
	{
		printf ("Warning: LOCK	- %2d has acquired the lock already\n", unitid);
		goto EXIT;
	}

	dart_gptr_t gptr_tail;
	dart_gptr_t gptr_list;
	int predecessor[1], result[1];
	
	MPI_Win win;
	MPI_Status status;


	DART_GPTR_COPY (gptr_tail, lock -> gptr_tail);
	
	DART_GPTR_COPY (gptr_list, lock -> gptr_list);

	int offset_tail = gptr_tail.addr_or_offs.offset;
	int offset_list = gptr_list.addr_or_offs.offset;
	int tail = gptr_tail.unitid;
	
	/* MPI-3 newly added feature: atomic operation*/
	MPI_Fetch_and_op (&unitid, predecessor, MPI_INT, tail, offset_tail, MPI_REPLACE, win_local_alloc);
	
	MPI_Win_flush (tail, win_local_alloc);

	
	/* If there was a previous tail (predecessor), update the previous tail's next pointer with unitid
	 * and wait for notification from its predecessor. */
	if (*predecessor != -1)
	{
		win = lock -> win;

		/* Atomicity: Update its predecessor's next pointer */
		MPI_Fetch_and_op (&unitid, result, MPI_INT, *predecessor, offset_list, MPI_REPLACE, win);

		MPI_Win_flush (*predecessor, win);
	

		/* Waiting for notification from its predecessor*/
		DEBUG ("%2d: LOCK	- waiting for notification from %d in team %d", 
				unitid, *predecessor, (lock -> teamid));
		MPI_Recv (NULL, 0, MPI_INT, *predecessor, 0, lock -> comm, &status);
	}
	
	DEBUG ("%2d: LOCK	- lock required in team %d", unitid, (lock -> teamid));
	lock -> acquired = 1;
EXIT:
	return DART_OK;
}

dart_ret_t dart_lock_try_acquire (dart_lock_t lock, int *acquired)
{
	dart_unit_t unitid;
	dart_team_myid (lock -> teamid, &unitid);
	if (lock -> acquired == 1)
	{
		printf ("Warning: TRYLOCK	- %2d has acquired the lock already\n", unitid);
		goto EXIT;
	}
	dart_gptr_t gptr_tail;
	dart_gptr_t gptr_list;

	int result[1];
	int compare[1] = {-1};
	MPI_Comm comm;
	MPI_Status status;
	DART_GPTR_COPY (gptr_tail, lock -> gptr_tail);
	int offset = gptr_tail.addr_or_offs.offset;
	int tail = gptr_tail.unitid;
	
	/* Atomicity: Check if the lock is available and claim it if it is. */
 	MPI_Compare_and_swap (&unitid, compare, result, MPI_INT, tail, offset, win_local_alloc);
	MPI_Win_flush (tail, win_local_alloc);

	/* If the old predecessor was -1, we will claim the lock, otherwise, do nothing. */
	if (*result == -1)
	{
		lock -> acquired = 1;
		*acquired = 1;
	}
	else
	{
		*acquired = 0;
	}
	char* string = (*acquired) ? "success" : "Non-success";
	DEBUG ("%2d: TRYLOCK	- %s in team %d", unitid, string, (lock -> teamid));
EXIT:
	return DART_OK;
}

dart_ret_t dart_lock_release (dart_lock_t lock)
{
	dart_unit_t unitid;
	dart_team_myid (lock -> teamid, &unitid);
	if (lock -> acquired == 0)
	{
		printf ("Warning: RELEASE	- %2d has not yet required the lock\n", unitid);
		goto EXIT;
	}
	dart_gptr_t gptr_tail;
	dart_gptr_t gptr_list;
	MPI_Status mpi_status;
	MPI_Win win;
	int successor, *addr2, next, result[1];
	int origin[1] = {-1};

	DART_GPTR_COPY (gptr_tail, lock -> gptr_tail);
	DART_GPTR_COPY (gptr_list, lock -> gptr_list);

	int offset_list = gptr_list.addr_or_offs.offset;
	int offset_tail = gptr_tail.addr_or_offs.offset;
	int tail = gptr_tail.unitid;
	dart_gptr_getaddr (gptr_list, &addr2);

	win = lock -> win;
	
	/* Atomicity: Check if we are at the tail of this lock queue, if so, we are done.
	 * Otherwise, we still need to send notification. */
	MPI_Compare_and_swap (origin, &unitid, result, MPI_INT, tail, offset_tail, win_local_alloc);
	MPI_Win_flush (tail, win_local_alloc);
		
	/* We are not at the tail of this lock queue. */
	if (*result != unitid)
	{
		DEBUG ("%2d: UNLOCK	- waiting for next pointer (tail = %d) in team %d", 
				unitid, *result, (lock -> teamid));
	
		/* Waiting for the update of my next pointer finished. */
		while (1)
		{
			MPI_Fetch_and_op (NULL, &next, MPI_INT, unitid, offset_list, MPI_NO_OP, win);
			MPI_Win_flush (unitid, win);
			
			if (next != -1) break;

		}

		DEBUG ("%2d: UNLOCK	- notifying %d in team %d", unitid, next, (lock -> teamid));
		
		/* Notifying the next unit waiting on the lock queue. */
		MPI_Send (NULL, 0, MPI_INT, next, 0, lock -> comm);

		*addr2 = -1;
		MPI_Win_sync (win);
	}
	lock -> acquired = 0;
	DEBUG ("%2d: UNLOCK	- release lock in team %d", unitid, (lock -> teamid));
EXIT:
	return DART_OK;
}

dart_ret_t dart_team_lock_free (dart_team_t teamid, dart_lock_t* lock)
{
	dart_gptr_t gptr_tail;
	dart_gptr_t gptr_list;
	dart_unit_t unitid;
	DART_GPTR_COPY (gptr_tail, (*lock) -> gptr_tail);
	DART_GPTR_COPY (gptr_list, (*lock) -> gptr_list);
	
	dart_team_myid (teamid, &unitid);
	if (unitid == 0)
	{
		dart_memfree (gptr_tail);
	}
	if (unitid >= 0)
	{
		dart_team_memfree (teamid, gptr_list);
		MPI_Comm_free (&((*lock) -> comm));
		DEBUG ("%2d: Free	- done in team %d", unitid, teamid);
	}

	free (*lock);
	*lock = NULL;
	return DART_OK;	
}




