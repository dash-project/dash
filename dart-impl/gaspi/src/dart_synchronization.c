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
#include <dash/dart/gaspi/dart_communication_priv.h>

dart_ret_t dart_team_lock_init(dart_team_t teamid, dart_lock_t* lock)
{
    dart_gptr_t gptr_tail;
    dart_gptr_t gptr_list;
    dart_unit_t unitid, myid;
    int32_t *addr;/*  the same type wiht "dart_unit_t" */

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
    }

    DART_CHECK_ERROR(dart_bcast(&gptr_tail, sizeof (dart_gptr_t), 0, teamid));

    /* Create a global memory region across the teamid,
     * and every local memory segment related certain unit
     * hold the next blocking unit info waiting on the lock. */
    DART_CHECK_ERROR(dart_team_memalloc_aligned(teamid, sizeof(int32_t), &gptr_list));

    DART_CHECK_ERROR(dart_gptr_setunit(&gptr_list, myid));
    DART_CHECK_ERROR(dart_gptr_getaddr(gptr_list, (void*)&addr));
    *addr = INT32_MAX;

    DART_GPTR_COPY((*lock)-> gptr_tail, gptr_tail);
    DART_GPTR_COPY((*lock)-> gptr_list, gptr_list);
    (*lock)->teamid = teamid;
    (*lock)->is_acquired = 0;

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

    free (*lock);
    *lock = NULL;

    return DART_OK;
}

dart_ret_t dart_lock_acquire (dart_lock_t lock)
{
    gaspi_segment_id_t gaspi_seg;
    dart_unit_t unitid;
    DART_CHECK_ERROR(dart_team_myid(lock->teamid, &unitid));
    if (lock -> is_acquired == 1)
    {
        printf ("Warning: LOCK  - %2d has acquired the lock already\n", unitid);
        return DART_OK;
    }

    dart_gptr_t gptr_tail;
    dart_gptr_t gptr_list;
    int32_t predecessor[1];

    DART_GPTR_COPY(gptr_tail, lock->gptr_tail);
    DART_GPTR_COPY(gptr_list, lock->gptr_list);

    uint64_t    offset_tail = gptr_tail.addr_or_offs.offset;
    // tail is absolute rank number
    dart_unit_t tail        = gptr_tail.unitid;
    int16_t     seg_id      = gptr_list.segid;
    uint16_t    index       = gptr_list.flags;

    DART_CHECK_GASPI_ERROR(gaspi_atomic_fetch_add(dart_mempool_seg_localalloc,
                                                  offset_tail,
                                                  tail,
                                                  (gaspi_atomic_value_t) unitid,// add the relative rank number
                                                  (gaspi_atomic_value_t *) predecessor,
                                                  GASPI_BLOCK));

    /* If there was a previous tail (predecessor), update the previous tail's next pointer with unitid
     * and wait for notification from its predecessor. */
    if (*predecessor != INT32_MAX)
    {
        gaspi_atomic_value_t old_val;
        if(dart_adapt_transtable_get_gaspi_seg_id(seg_id, *predecessor, &gaspi_seg) == -1)
        {
            return DART_ERR_INVAL;
        }

        dart_unit_t predecessor_abs;
        DART_CHECK_ERROR(unit_l2g(index, &predecessor_abs, *predecessor));

        /* Atomicity: Update its predecessor's next pointer */
        DART_CHECK_GASPI_ERROR(gaspi_atomic_fetch_add(gaspi_seg,
                                                      0UL,
                                                      predecessor_abs,
                                                      (gaspi_atomic_value_t) unitid,// add the relative rank number
                                                      &old_val,
                                                      GASPI_BLOCK));

        if(dart_adapt_transtable_get_local_gaspi_seg_id(seg_id, &gaspi_seg) == -1)
        {
            return DART_ERR_INVAL;
        }
        /* Waiting for notification from its predecessor*/
        gaspi_notification_id_t first_id;
        gaspi_notification_t    notify_val;

        DART_CHECK_GASPI_ERROR(gaspi_notify_waitsome(gaspi_seg, *predecessor, 1, &first_id, GASPI_BLOCK));
        DART_CHECK_GASPI_ERROR(gaspi_notify_reset(gaspi_seg, first_id, &notify_val));

        if(notify_val != 42)
        {
            fprintf(stderr, "Wrong notify value received\n");
            return DART_ERR_OTHER;
        }
    }
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

dart_ret_t dart_lock_release (dart_lock_t lock)
{
    dart_unit_t unitid, myid;
    DART_CHECK_ERROR(dart_team_myid(lock->teamid, &unitid));
    DART_CHECK_ERROR(dart_myid(&myid));

    if (lock -> is_acquired == 0)
    {
        printf("Warning: RELEASE   - %2d has not yet required the lock\n", unitid);
        return DART_OK;
    }

    dart_gptr_t gptr_tail;
    dart_gptr_t gptr_list;
    int32_t     *addr2, next, result[1];
    int32_t     origin[1] = {INT32_MAX};

    DART_GPTR_COPY(gptr_tail, lock->gptr_tail);
    DART_GPTR_COPY(gptr_list, lock->gptr_list);

    uint64_t           offset_tail = gptr_tail.addr_or_offs.offset;
    int16_t            seg_id      = gptr_list.segid;
    uint16_t           index       = gptr_list.flags;
    dart_unit_t        tail        = gptr_tail.unitid;// tail is the absolute rank number
    gaspi_segment_id_t gaspi_seg;

    dart_gptr_getaddr (gptr_list, (void*) &addr2);

    /* Atomicity: Check if we are at the tail of this lock queue, if so, we are done.
     * Otherwise, we still need to send notification. */
    DART_CHECK_GASPI_ERROR(gaspi_atomic_compare_swap(dart_mempool_seg_localalloc,
                                                     offset_tail,
                                                     tail,
                                                     (gaspi_atomic_value_t) unitid,// relative rank number
                                                     (gaspi_atomic_value_t) origin,
                                                     (gaspi_atomic_value_t *) result,
                                                     GASPI_BLOCK));

    /* We are not at the tail of this lock queue. */
    if (*result != unitid)
    {
        // TODO Accessing own segment -> lock list
        if(dart_adapt_transtable_get_gaspi_seg_id(seg_id, unitid, &gaspi_seg) == -1)
        {
            return DART_ERR_INVAL;
        }

        /* Waiting for the update of my next pointer finished. */
        while (1)
        {
            // Atomic local access
            DART_CHECK_GASPI_ERROR(gaspi_atomic_compare_swap(gaspi_seg,
                                                             0UL,
                                                             myid,
                                                             (gaspi_atomic_value_t) INT32_MAX,
                                                             (gaspi_atomic_value_t) INT32_MAX,
                                                             (gaspi_atomic_value_t *) &next,// relative rank number
                                                             GASPI_BLOCK));



            if (next != INT32_MAX) break;
        }
        gaspi_segment_id_t next_segid;
        dart_unit_t        next_abs;
        gaspi_queue_id_t   qid;

        if(dart_adapt_transtable_get_gaspi_seg_id(seg_id, next, &next_segid) == -1)
        {
            return DART_ERR_INVAL;
        }
        DART_CHECK_ERROR(unit_l2g(index, &next_abs, next));
        DART_CHECK_GASPI_ERROR(dart_get_minimal_queue(&qid));
        /* Notifying the next unit waiting on the lock queue. */
        DART_CHECK_GASPI_ERROR(gaspi_notify(next_segid, next_abs, unitid, 42, qid, GASPI_BLOCK));
        *addr2 = INT32_MAX;
    }

    lock -> is_acquired = 0;
    return DART_OK;
}
