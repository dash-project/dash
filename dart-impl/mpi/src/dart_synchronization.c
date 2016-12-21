/**
 *  \file  dart_synchronization.c
 *
 *  Synchronization operations.
 */

#include <dash/dart/base/logging.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_globmem.h>
#include <dash/dart/if/dart_team_group.h>
#include <dash/dart/if/dart_communication.h>
#include <dash/dart/if/dart_synchronization.h>

#include <dash/dart/mpi/dart_team_private.h>
#include <dash/dart/mpi/dart_mem.h>
#include <dash/dart/mpi/dart_globmem_priv.h>
#include <dash/dart/mpi/dart_synchronization_priv.h>
#include <dash/dart/mpi/dart_segment.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>

dart_ret_t dart_team_lock_init (dart_team_t teamid, dart_lock_t* lock)
{
	dart_gptr_t gptr_tail;
	dart_gptr_t gptr_list;
	dart_team_unit_t unitid;
	dart_global_unit_t myid;
	int32_t *addr;

	uint16_t index;
	int result = dart_adapt_teamlist_convert (teamid, &index);
	if (result == -1) {
		return DART_ERR_INVAL;
	}

	dart_team_myid (teamid, &unitid);
	dart_myid (&myid);
	*lock = (dart_lock_t) malloc (sizeof (struct dart_lock_struct));


	/* Unit 0 is the process holding the gptr_tail by default. */
	if (unitid.id == 0) {
		dart_memalloc(1, DART_TYPE_INT, &gptr_tail);
		dart_gptr_getaddr (gptr_tail, (void*)&addr);

		/* Local store is safe and effective followed by the sync call. */
		*addr = -1;
		MPI_Win_sync (dart_win_local_alloc);
	}

	dart_bcast(&gptr_tail, sizeof(dart_gptr_t), DART_TYPE_BYTE, DART_TEAM_UNIT_ID(0), teamid);

	/* Create a global memory region across the teamid,
	 * and every local memory segment related certain unit
	 * hold the next blocking unit info waiting on the lock. */
	dart_team_memalloc_aligned(teamid,
                             1, DART_TYPE_INT,
                             &gptr_list);

  MPI_Win win;
  win = dart_team_data[index].window; //this window object is used for atomic operations

	dart_gptr_setunit (&gptr_list, myid);
	dart_gptr_getaddr (gptr_list, (void*)&addr);
	*addr = -1;
	MPI_Win_sync (win);

	DART_GPTR_COPY((*lock) -> gptr_tail, gptr_tail);
	DART_GPTR_COPY((*lock) -> gptr_list, gptr_list);
	(*lock) -> teamid = teamid;
	(*lock) -> is_acquired = 0;

	DART_LOG_DEBUG ("%2d: INIT	- done", unitid);

	return DART_OK;
}

dart_ret_t dart_lock_acquire (dart_lock_t lock)
{
	dart_team_unit_t unitid;
	dart_team_myid (lock->teamid, &unitid);

	if (lock -> is_acquired == 1)
	{
		printf ("Warning: LOCK	- %2d has acquired the lock already\n", unitid.id);
		return DART_OK;
	}

	dart_gptr_t gptr_tail;
	dart_gptr_t gptr_list;
	int32_t predecessor[1], result[1];

	MPI_Win win;
	MPI_Status status;

	DART_GPTR_COPY(gptr_tail, lock -> gptr_tail);
	DART_GPTR_COPY(gptr_list, lock -> gptr_list);

  uint64_t offset_tail = gptr_tail.addr_or_offs.offset;
  int16_t seg_id = gptr_list.segid;
  dart_unit_t tail = gptr_tail.unitid;
  MPI_Aint disp_list;

  uint16_t index;
  if (dart_segment_get_teamidx(seg_id, &index) != DART_OK) {
    DART_LOG_ERROR("dart_lock_acquire ! failed: Unknown segment %i!", seg_id);
    return DART_ERR_INVAL;
  }


	/* MPI-3 newly added feature: atomic operation*/
	MPI_Fetch_and_op (&unitid.id, predecessor, MPI_INT32_T, tail, offset_tail, MPI_REPLACE, dart_win_local_alloc);
	MPI_Win_flush (tail, dart_win_local_alloc);

  /* If there was a previous tail (predecessor), update the previous tail's next pointer with unitid
   * and wait for notification from its predecessor. */
  if (*predecessor != -1) {
    if (dart_segment_get_disp(seg_id, DART_TEAM_UNIT_ID(*predecessor), &disp_list) != DART_OK) {
      return DART_ERR_INVAL;
    }
    win = dart_team_data[index].window;

		/* Atomicity: Update its predecessor's next pointer */
		MPI_Fetch_and_op (&unitid.id, result, MPI_INT32_T, *predecessor, disp_list, MPI_REPLACE, win);

		MPI_Win_flush (*predecessor, win);

		/* Waiting for notification from its predecessor*/
		DART_LOG_DEBUG ("%2d: LOCK	- waiting for notification from %d in team %d",
				unitid, *predecessor, (lock -> teamid));

    MPI_Recv(NULL, 0, MPI_INT, *predecessor, 0, dart_team_data[index].comm,
        &status);
  }

	DART_LOG_DEBUG ("%2d: LOCK	- lock required in team %d", unitid, (lock -> teamid));
	lock -> is_acquired = 1;
	return DART_OK;
}

dart_ret_t dart_lock_try_acquire (dart_lock_t lock, int32_t *is_acquired)
{
	dart_team_unit_t unitid;
	dart_team_myid(lock->teamid, &unitid);
	if (lock -> is_acquired == 1)
	{
		printf ("Warning: TRYLOCK	- %2d has acquired the lock already\n", unitid.id);
		return DART_OK;
	}
	dart_gptr_t gptr_tail;

	int32_t result[1];
	int32_t compare[1] = {-1};

	DART_GPTR_COPY(gptr_tail, lock -> gptr_tail);
	dart_unit_t tail = gptr_tail.unitid;
	uint64_t offset = gptr_tail.addr_or_offs.offset;

	/* Atomicity: Check if the lock is available and claim it if it is. */
  MPI_Compare_and_swap (&unitid.id, compare, result, MPI_INT32_T, tail, offset, dart_win_local_alloc);
	MPI_Win_flush (tail, dart_win_local_alloc);

	/* If the old predecessor was -1, we will claim the lock, otherwise, do nothing. */
	if (*result == -1)
	{
		lock -> is_acquired = 1;
		*is_acquired = 1;
	}
	else
	{
		*is_acquired = 0;
	}
	DART_LOG_DEBUG("dart_lock_try_acquire: trylock %s in team %d",
                 ((*is_acquired) ? "succeeded" : "failed"),
                 (lock -> teamid));
	return DART_OK;
}

dart_ret_t dart_lock_release (dart_lock_t lock)
{
  dart_team_unit_t unitid;
  dart_team_myid(lock->teamid, &unitid);
  if (lock -> is_acquired == 0) {
    printf("Warning: RELEASE	- %2d has not yet required the lock\n", unitid.id);
    return DART_OK;
  }
  dart_gptr_t gptr_tail;
  dart_gptr_t gptr_list;
  MPI_Win win;
  int32_t * addr2, next, result[1];

  MPI_Aint disp_list;
  int32_t origin[1] = { -1};

  DART_GPTR_COPY(gptr_tail, lock -> gptr_tail);
  DART_GPTR_COPY(gptr_list, lock -> gptr_list);

  uint64_t offset_tail = gptr_tail.addr_or_offs.offset;
  int16_t  seg_id      = gptr_list.segid;
  dart_unit_t tail     = gptr_tail.unitid;
  dart_gptr_getaddr(gptr_list, (void *)&addr2);

  uint16_t index;
  if (dart_segment_get_teamidx(seg_id, &index) != DART_OK) {
    DART_LOG_ERROR("dart_lock_release ! failed: Unknown segment %i!", seg_id);
    return DART_ERR_INVAL;
  }

  win = dart_team_data[index].window;

  /* Atomicity: Check if we are at the tail of this lock queue, if so, we are done.
   * Otherwise, we still need to send notification. */
  MPI_Compare_and_swap(origin, &unitid.id, result, MPI_INT32_T, tail, offset_tail,
                       dart_win_local_alloc);
  MPI_Win_flush(tail, dart_win_local_alloc);

  /* We are not at the tail of this lock queue. */
  if (*result != unitid.id) {
    DART_LOG_DEBUG("%2d: UNLOCK	- waiting for next pointer (tail = %d) in team %d",
                   unitid, *result, (lock -> teamid));

    if (dart_segment_get_disp(seg_id, unitid, &disp_list) != DART_OK) {
      return DART_ERR_INVAL;
    }

    /* Waiting for the update of my next pointer finished. */
    while (1) {
      MPI_Fetch_and_op(NULL, &next, MPI_INT, unitid.id, disp_list, MPI_NO_OP, win);
      MPI_Win_flush(unitid.id, win);

      if (next != -1) {
        break;
      }
    }

    DART_LOG_DEBUG("%2d: UNLOCK	- notifying %d in team %d", unitid, next,
                   (lock -> teamid));

    /* Notifying the next unit waiting on the lock queue. */

    MPI_Send(NULL, 0, MPI_INT, next, 0, dart_team_data[index].comm);
    *addr2 = -1;
    MPI_Win_sync(win);
  }
  lock -> is_acquired = 0;
  DART_LOG_DEBUG("%2d: UNLOCK	- release lock in team %d", unitid,
                 (lock -> teamid));
  return DART_OK;
}

dart_ret_t dart_team_lock_free (dart_team_t teamid, dart_lock_t* lock)
{
	dart_gptr_t gptr_tail;
	dart_gptr_t gptr_list;
	dart_team_unit_t unitid;
	DART_GPTR_COPY(gptr_tail, (*lock) -> gptr_tail);
	DART_GPTR_COPY(gptr_list, (*lock) -> gptr_list);

	dart_team_myid (teamid, &unitid);
	if (unitid.id == 0)
	{
		dart_memfree (gptr_tail);
	}

	dart_team_memfree (teamid, gptr_list);
	DART_LOG_DEBUG ("%2d: Free	- done in team %d", unitid, teamid);
	*lock = NULL;
	free (*lock);
	return DART_OK;
}




