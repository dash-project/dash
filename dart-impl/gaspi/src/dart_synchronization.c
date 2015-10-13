#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <limits.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_globmem.h>
#include <dash/dart/if/dart_communication.h>
#include <dash/dart/if/dart_synchronization.h>
#include <dash/dart/if/dart_team_group.h>

#include <dash/dart/gaspi/dart_translation.h>
#include <dash/dart/gaspi/dart_team_private.h>
#include <dash/dart/gaspi/dart_mem.h>
#include <dash/dart/gaspi/dart_globmem_priv.h>
#include <dash/dart/gaspi/dart_synchronization_priv.h>

dart_ret_t dart_team_lock_init(dart_team_t teamid, dart_lock_t* lock)
{
    dart_gptr_t gptr_tail;
    dart_gptr_t gptr_list;
    dart_unit_t    unitid, myid;
    int32_t *addr, *next;/*  the same type wiht "dart_unit_t" */

    uint16_t index;
    int result = dart_adapt_teamlist_convert(teamid, &index);
    if (result == -1)
    {
        return DART_ERR_INVAL;
    }

    DART_CHECK_ERROR(dart_team_myid (teamid, &unitid));
    DART_CHECK_ERROR(dart_myid (&myid));

    *lock = (dart_lock_t) malloc (sizeof (struct dart_lock_struct));
    assert(*lock);

    /* Unit 0 is the process holding the gptr_tail by default. */
    if (unitid == 0)
    {
        DART_CHECK_ERROR(dart_memalloc(sizeof(int32_t), &gptr_tail));
        DART_CHECK_ERROR(dart_gptr_getaddr (gptr_tail, (void*)&addr));

        /* Local store is safe and effective followed by the sync call. */
        *addr = INT32_MAX;
        //~ MPI_Win_sync (dart_win_local_alloc);
    }

    DART_CHECK_ERROR(dart_bcast(&gptr_tail, sizeof (dart_gptr_t), 0, teamid));

    /* Create a global memory region across the teamid,
     * and every local memory segment related certain unit
     * hold the next blocking unit info waiting on the lock. */
    DART_CHECK_ERROR(dart_team_memalloc_aligned(teamid, sizeof(int32_t), &gptr_list));

    //~ MPI_Win win;
    //~ win = dart_win_lists[index];//this window object is used for atomic operations

    DART_CHECK_ERROR(dart_gptr_setunit (&gptr_list, myid));
    DART_CHECK_ERROR(dart_gptr_getaddr (gptr_list, (void*)&addr));
    *addr = -1;
    //~ MPI_Win_sync (win);

    DART_GPTR_COPY((*lock)-> gptr_tail, gptr_tail);
    DART_GPTR_COPY((*lock)-> gptr_list, gptr_list);
    (*lock)->teamid = teamid;
    (*lock)->is_acquired = 0;

    //~ DEBUG ("%2d: INIT   - done", unitid);

    return DART_OK;
}

dart_ret_t dart_team_lock_free (dart_team_t teamid, dart_lock_t* lock)
{
    dart_gptr_t gptr_tail;
    dart_gptr_t gptr_list;
    dart_unit_t unitid;
    DART_GPTR_COPY(gptr_tail, (*lock)->gptr_tail);
    DART_GPTR_COPY(gptr_list, (*lock)->gptr_list);

    DART_CHECK_ERROR(dart_team_myid(teamid, &unitid));
    if (unitid == 0)
    {
        DART_CHECK_ERROR(dart_memfree(gptr_tail));
    }

    DART_CHECK_ERROR(dart_team_memfree (teamid, gptr_list));
    //~ DEBUG ("%2d: Free   - done in team %d", unitid, teamid);
    // TODO what is that ????
    free (*lock);
    *lock = NULL;
    return DART_OK;
}

dart_ret_t dart_lock_acquire (dart_lock_t lock)
{
    gaspi_segment_id_t gaspi_seg;
    dart_unit_t unitid;
    dart_team_myid(lock->teamid, &unitid);

    if (lock -> is_acquired == 1)
    {
        printf ("Warning: LOCK  - %2d has acquired the lock already\n", unitid);
        return DART_OK;
    }

    dart_gptr_t gptr_tail;
    dart_gptr_t gptr_list;
    int32_t predecessor[1], result[1];
    //~ MPI_Win win;
    //~ MPI_Status status;
    DART_GPTR_COPY(gptr_tail, lock->gptr_tail);
    DART_GPTR_COPY(gptr_list, lock->gptr_list);

    uint64_t offset_tail = gptr_tail.addr_or_offs.offset;
    uint64_t offset_list = gptr_list.addr_or_offs.offset;
    int16_t seg_id = gptr_list.segid;
    dart_unit_t tail = gptr_tail.unitid;
    uint16_t index = gptr_list.flags;
    //~ MPI_Aint disp_list;

    DART_CHECK_GASPI_ERROR(gaspi_atomic_fetch_add(dart_mempool_seg_localalloc, offset_tail, tail,(gaspi_atomic_value_t) unitid,(gaspi_atomic_value_t *) predecessor, GASPI_BLOCK));
    //~ MPI_Fetch_and_op(&unitid, predecessor, MPI_INT32_T, tail, offset_tail, MPI_REPLACE, dart_win_local_alloc);
    //~ MPI_Win_flush(tail, dart_win_local_alloc);

    /* If there was a previous tail (predecessor), update the previous tail's next pointer with unitid
     * and wait for notification from its predecessor. */
    if (*predecessor != INT32_MAX)
    {
        gaspi_atomic_value_t old_val;
        //~ if (dart_adapt_transtable_get_disp (seg_id, *predecessor, &disp_list) == -1)
        if(dart_adapt_transtable_get_gaspi_seg_id(seg_id, *predecessor,&gaspi_seg) == -1)
        {
            return DART_ERR_INVAL;
        }
        //~ win = dart_win_lists[index];

        /* Atomicity: Update its predecessor's next pointer */
        DART_CHECK_GASPI_ERROR(gaspi_atomic_fetch_add(gaspi_seg, 0UL, *predecessor, (gaspi_atomic_value_t) unitid, &old_val, GASPI_BLOCK));
        //~ MPI_Fetch_and_op(&unitid, result, MPI_INT32_T, *predecessor, disp_list, MPI_REPLACE, win);

        //~ MPI_Win_flush(*predecessor, win);

        /* Waiting for notification from its predecessor*/
        //~ DEBUG ("%2d: LOCK   - waiting for notification from %d in team %d",
               //~ unitid, *predecessor, (lock -> teamid));
        gaspi_notification_id_t first_id;
        gaspi_notification_t    notify_val;
        DART_CHECK_GASPI_ERROR(gaspi_notify_waitsome(gaspi_seg, 1, 1, &first_id, GASPI_BLOCK));
        DART_CHECK_GASPI_ERROR(gaspi_notify_reset(gaspi_seg, first_id, &notify_val));

        if(notify_val != 42)
        {
            fprintf(stderr, "Wrong notify value received\n");
            return DART_ERR_OTHER;
        }

        //~ MPI_Recv(NULL, 0, MPI_INT, *predecessor, 0, dart_teams[index], &status);
    }
    //~ DEBUG ("%2d: LOCK   - lock required in team %d", unitid, (lock -> teamid));
    lock->is_acquired = 1;
    return DART_OK;
}

//~ dart_ret_t dart_lock_try_acquire (dart_lock_t lock, int32_t *is_acquired)
//~ {
    //~ dart_unit_t unitid;
    //~ dart_team_myid (lock -> teamid, &unitid);
    //~ if (lock -> is_acquired == 1)
    //~ {
        //~ printf ("Warning: TRYLOCK   - %2d has acquired the lock already\n", unitid);
        //~ return DART_OK;
    //~ }
    //~ dart_gptr_t gptr_tail;
    //~ dart_gptr_t gptr_list;

    //~ int32_t result[1];
    //~ int32_t compare[1] = {-1};

    //~ MPI_Status status;
    //~ DART_GPTR_COPY (gptr_tail, lock -> gptr_tail);
    //~ dart_unit_t tail = gptr_tail.unitid;
    //~ uint64_t offset = gptr_tail.addr_or_offs.offset;

    //~ /* Atomicity: Check if the lock is available and claim it if it is. */
    //~ MPI_Compare_and_swap (&unitid, compare, result, MPI_INT32_T, tail, offset, dart_win_local_alloc);
    //~ MPI_Win_flush (tail, dart_win_local_alloc);

    //~ /* If the old predecessor was -1, we will claim the lock, otherwise, do nothing. */
    //~ if (*result == -1)
    //~ {
        //~ lock -> is_acquired = 1;
        //~ *is_acquired = 1;
    //~ }
    //~ else
    //~ {
        //~ *is_acquired = 0;
    //~ }
    //~ char* string = (*is_acquired) ? "success" : "Non-success";
    //~ DEBUG ("%2d: TRYLOCK    - %s in team %d", unitid, string, (lock -> teamid));
    //~ return DART_OK;
//~ }

//~ dart_ret_t dart_lock_release (dart_lock_t lock)
//~ {
    //~ dart_unit_t unitid;
    //~ dart_team_myid(lock->teamid, &unitid);
    //~ if (lock -> is_acquired == 0)
    //~ {
        //~ printf ("Warning: RELEASE   - %2d has not yet required the lock\n", unitid);
        //~ return DART_OK;
    //~ }
    //~ dart_gptr_t gptr_tail;
    //~ dart_gptr_t gptr_list;
    //~ MPI_Status mpi_status;
    //~ MPI_Win win;
    //~ int32_t successor, *addr2, next, result[1];

    //~ MPI_Aint disp_list;
    //~ int32_t origin[1] = {INT32_MAX};

    //~ DART_GPTR_COPY(gptr_tail, lock->gptr_tail);
    //~ DART_GPTR_COPY(gptr_list, lock->gptr_list);

    //~ uint64_t offset_tail = gptr_tail.addr_or_offs.offset;
    //~ uint64_t offset_list = gptr_list.addr_or_offs.offset;
    //~ int16_t seg_id = gptr_list.segid;
    //~ dart_unit_t tail = gptr_tail.unitid;
    //~ uint16_t index = gptr_list.flags;
    //~ dart_gptr_getaddr (gptr_list, (void*)&addr2);

    //~ win = dart_win_lists[index];
    //~ gaspi_segment_id_t gaspi_seg;
    //~ if(dart_adapt_transtable_get_local_gaspi_seg_id(seg_id, &gaspi_seg) == -1)
    //~ {
        //~ return DART_ERR_OTHER;
    //~ }

    //~ /* Atomicity: Check if we are at the tail of this lock queue, if so, we are done.
     //~ * Otherwise, we still need to send notification. */
    //~ DART_CHECK_GASPI_ERROR(gaspi_atomic_compare_swap(gaspi_seg,
                                                     //~ 0UL,
                                                     //~ tail,
                                                     //~ (gaspi_atomic_value_t) unitid,
                                                     //~ (gaspi_atomic_value_t) origin,
                                                     //~ (gaspi_atomic_value_t *) result,
                                                     //~ GASPI_BLOCK));
    //~ MPI_Compare_and_swap (origin, &unitid, result, MPI_INT32_T, tail, offset_tail, dart_win_local_alloc);
    //~ MPI_Win_flush (tail, dart_win_local_alloc);

    //~ /* We are not at the tail of this lock queue. */
    //~ if (*result != unitid)
    //~ {
        //~ DEBUG ("%2d: UNLOCK - waiting for next pointer (tail = %d) in team %d",
               //~ unitid, *result, (lock->teamid));

        //~ if (dart_adapt_transtable_get_disp (seg_id, unitid, &disp_list) == -1)
        //~ if(dart_adapt_transtable_get_gaspi_seg_id(seg_id, unitid, &gaspi_seg) == -1)
        //~ {
            //~ return DART_ERR_INVAL;
        //~ }

        //~ /* Waiting for the update of my next pointer finished. */
        //~ while (1)
        //~ {
            //~ // May be gaspi_read ??
            //~ MPI_Fetch_and_op (NULL, &next, MPI_INT, unitid, disp_list, MPI_NO_OP, win);
            //~ MPI_Win_flush (unitid, win);

            //~ if (next != INT32_MAX) break;
        //~ }

        //~ DEBUG ("%2d: UNLOCK - notifying %d in team %d", unitid, next, (lock -> teamid));

        //~ /* Notifying the next unit waiting on the lock queue. */

        //~ MPI_Send (NULL, 0, MPI_INT, next, 0, dart_teams[index]);
        //~ *addr2 = -1;
        //~ MPI_Win_sync (win);
    //~ }
    //~ lock -> is_acquired = 0;
    //~ DEBUG ("%2d: UNLOCK - release lock in team %d", unitid, (lock -> teamid));
    //~ return DART_OK;
//~ }
