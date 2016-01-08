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

#define DART_UNLOCK_VAL 999999

/**
 * Function for testing dart lock mechanism
 * @param gptr[OUT] returns the global pointer of the lock
 */
dart_ret_t dart_lock_get_gptr(dart_lock_t lock, dart_gptr_t * gptr)
{
    gptr->segid  = lock->gptr.segid;
    gptr->unitid = lock->gptr.unitid;
    gptr->flags  = lock->gptr.flags;
    gptr->addr_or_offs.offset  = lock->gptr.addr_or_offs.offset;

    return DART_OK;
}
/**
 * team collective operation
 */
dart_ret_t dart_team_lock_init(dart_team_t teamid, dart_lock_t* lock)
{
    dart_gptr_t gptr;
    dart_unit_t unitid, myid;

    DART_CHECK_ERROR(dart_team_myid(teamid, &unitid));
    DART_CHECK_ERROR(dart_myid (&myid));

    *lock = (dart_lock_t) malloc (sizeof (struct dart_lock_struct));
    assert(*lock);

    /* Unit 0 is the process holding the gptr by default in the team. */
    if (unitid == 0)
    {
        void * vptr;
        DART_CHECK_ERROR(dart_memalloc(sizeof(gaspi_atomic_value_t), &gptr));
        DART_CHECK_ERROR(dart_gptr_getaddr(gptr, &vptr));

        *((gaspi_atomic_value_t *) vptr) = DART_UNLOCK_VAL;
    }

    DART_CHECK_ERROR(dart_bcast(&gptr, sizeof (dart_gptr_t), 0, teamid));

    DART_GPTR_COPY((*lock)->gptr, gptr);
    (*lock)->teamid = teamid;
    (*lock)->is_acquired = 0;

    return DART_OK;
}
/**
 * team collective operation
 */
dart_ret_t dart_team_lock_free (dart_team_t teamid, dart_lock_t* lock)
{
    dart_gptr_t gptr;
    dart_unit_t unitid;
    DART_GPTR_COPY(gptr, (*lock)->gptr);

    DART_CHECK_ERROR(dart_barrier(teamid));

    DART_CHECK_ERROR(dart_team_myid(teamid, &unitid));
    if(unitid == 0)
    {
        DART_CHECK_ERROR(dart_memfree(gptr));
    }

    free (*lock);
    *lock = NULL;

    return DART_OK;
}

dart_ret_t dart_lock_try_acquire(dart_lock_t lock, int32_t *result)
{
    dart_unit_t          myid;
    gaspi_atomic_value_t old_lock_val;
    dart_unit_t          unitid = lock->gptr.unitid;
    uint64_t             offset = lock->gptr.addr_or_offs.offset;

    if(lock->is_acquired)
    {
        fprintf(stderr, "dart_lock_try_acquire: lock is already acquired\n");
        return DART_ERR_OTHER;
    }

    DART_CHECK_ERROR(dart_myid(&myid));

    DART_CHECK_GASPI_ERROR(gaspi_atomic_compare_swap(dart_mempool_seg_localalloc,
                                                     offset,
                                                     unitid,
                                                     DART_UNLOCK_VAL,
                                                     (gaspi_atomic_value_t) myid,
                                                     &old_lock_val,
                                                     GASPI_BLOCK));
    if(old_lock_val == DART_UNLOCK_VAL)
    {
        *result = 1;
        lock->is_acquired = 1;
    }
    else
    {
        *result = 0;
        lock->is_acquired = 0;
    }

    return DART_OK;
}

dart_ret_t dart_lock_acquire (dart_lock_t lock)
{
    if(lock->is_acquired)
    {
        fprintf(stderr, "dart_lock_acquire: lock is already acquired\n");
        return DART_ERR_OTHER;
    }

    int32_t result = 0;
    do{
        DART_CHECK_ERROR(dart_lock_try_acquire(lock, &result));
    }while(!result);

    return DART_OK;
}

dart_ret_t dart_lock_release (dart_lock_t lock)
{
    gaspi_atomic_value_t old_lock_val;
    dart_unit_t myid;
    dart_unit_t unitid = lock->gptr.unitid;
    uint64_t offset = lock->gptr.addr_or_offs.offset;

    DART_CHECK_ERROR(dart_myid(&myid));

    DART_CHECK_GASPI_ERROR(gaspi_atomic_compare_swap(dart_mempool_seg_localalloc,
                                                     offset,
                                                     unitid,
                                                     (gaspi_atomic_value_t) myid,
                                                     DART_UNLOCK_VAL,
                                                     &old_lock_val,
                                                     GASPI_BLOCK));

    if(((gaspi_atomic_value_t) myid) != old_lock_val)
    {
        fprintf(stderr, "Lock release failed\n");
        return DART_ERR_INVAL;
    }

    lock->is_acquired = 0;

    return DART_OK;
}

/*
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

    gaspi_atomic_value_t val_old;
    DART_CHECK_GASPI_ERROR(gaspi_atomic_fetch_add(1,
                                                  0UL,
                                                  0,
                                                  42,//(gaspi_atomic_value_t) unitid,// add the relative rank number
                                                  &val_old,//(gaspi_atomic_value_t *) predecessor,
                                                  GASPI_BLOCK));
    *predecessor = INT32_MAX;
    gaspi_pointer_t ptr;
    DART_CHECK_GASPI_ERROR(gaspi_segment_ptr(1, &ptr));
    gaspi_atomic_value_t * iptr = (gaspi_atomic_value_t *) ptr;
    gaspi_printf("After fetch and add tail %lu\n", *iptr);

    if (*predecessor != INT32_MAX)
    {
        gaspi_atomic_value_t old_val;
        if(dart_adapt_transtable_get_gaspi_seg_id(seg_id, *predecessor, &gaspi_seg) == -1)
        {
            return DART_ERR_INVAL;
        }

        dart_unit_t predecessor_abs;
        DART_CHECK_ERROR(unit_l2g(index, &predecessor_abs, *predecessor));


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
    gaspi_printf("dart_lock_acquire: Lock acquired\n");
    return DART_OK;
}*/

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

/*dart_ret_t dart_lock_release (dart_lock_t lock)
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


    int16_t            seg_id      = gptr_list.segid;
    uint16_t           index       = gptr_list.flags;
    uint64_t           offset_tail = gptr_tail.addr_or_offs.offset;
    dart_unit_t        tail        = gptr_tail.unitid;// tail is the absolute rank number
    gaspi_segment_id_t gaspi_seg;
    void * tmp;
    dart_gptr_getaddr(gptr_list, (void*) &addr2);
    dart_gptr_getaddr(gptr_tail, &tmp);
    gaspi_printf("tail %d\n", *((int32_t *) tmp));

    DART_CHECK_GASPI_ERROR(gaspi_atomic_compare_swap(dart_mempool_seg_localalloc,
                                                     offset_tail,
                                                     tail,
                                                     (gaspi_atomic_value_t) unitid,// relative rank number
                                                     (gaspi_atomic_value_t) origin,
                                                     (gaspi_atomic_value_t *) result,
                                                     GASPI_BLOCK));
    gaspi_printf("Unit %d holds holds the lock\n", *result);

    if (*result != unitid)
    {

        if(dart_adapt_transtable_get_gaspi_seg_id(seg_id, unitid, &gaspi_seg) == -1)
        {
            return DART_ERR_INVAL;
        }


        while (1)
        {

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

        DART_CHECK_GASPI_ERROR(gaspi_notify(next_segid, next_abs, unitid, 42, qid, GASPI_BLOCK));
        *addr2 = INT32_MAX;
    }

    lock -> is_acquired = 0;
    return DART_OK;
}
*/
