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
 * team collective operation
 */
dart_ret_t dart_team_lock_init(dart_team_t teamid, dart_lock_t* lock)
{
    dart_gptr_t gptr;
    dart_team_unit_t unitid;
    //not used
    //dart_global_unit_t myid;

    DART_CHECK_ERROR(dart_team_myid(teamid, &unitid));
    //DART_CHECK_ERROR(dart_myid (&myid));

    *lock = (dart_lock_t) malloc (sizeof (struct dart_lock_struct));
    assert(*lock);

    /* Unit 0 is the process holding the gptr by default in the team. */
    if (unitid.id == 0)
    {
        void * vptr;
        DART_CHECK_ERROR(dart_memalloc(1, DART_TYPE_INT, &gptr));
        DART_CHECK_ERROR(dart_gptr_getaddr(gptr, &vptr));

        *((gaspi_atomic_value_t *) vptr) = DART_UNLOCK_VAL;
    }

    DART_CHECK_ERROR(dart_bcast(&gptr, sizeof (dart_gptr_t), DART_TYPE_BYTE, DART_TEAM_UNIT_ID(0), teamid));

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
    dart_team_unit_t unitid;
    DART_GPTR_COPY(gptr, (*lock)->gptr);

    DART_CHECK_ERROR(dart_barrier(teamid));

    DART_CHECK_ERROR(dart_team_myid(teamid, &unitid));
    if(unitid.id == 0)
    {
        DART_CHECK_ERROR(dart_memfree(gptr));
    }

    free (*lock);
    *lock = NULL;

    return DART_OK;
}

dart_ret_t dart_lock_try_acquire(dart_lock_t lock, int32_t *result)
{
    dart_global_unit_t          myid;
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
                                                     (gaspi_atomic_value_t) myid.id,
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
    dart_global_unit_t myid;
    dart_unit_t unitid = lock->gptr.unitid;
    uint64_t offset = lock->gptr.addr_or_offs.offset;

    DART_CHECK_ERROR(dart_myid(&myid));

    DART_CHECK_GASPI_ERROR(gaspi_atomic_compare_swap(dart_mempool_seg_localalloc,
                                                     offset,
                                                     unitid,
                                                     (gaspi_atomic_value_t) myid.id,
                                                     DART_UNLOCK_VAL,
                                                     &old_lock_val,
                                                     GASPI_BLOCK));

    if(((gaspi_atomic_value_t) myid.id) != old_lock_val)
    {
        fprintf(stderr, "Lock release failed\n");
        return DART_ERR_INVAL;
    }

    lock->is_acquired = 0;

    return DART_OK;
}


//copied from the mpi dart_synchronization.c by Rodario
dart_ret_t dart_team_lock_destroy(dart_lock_t* lock)
{
    printf("dart_team_lock_destroy for gaspi not supported!\n");
    return DART_ERR_INVAL;
}

bool dart_lock_initialized(struct dart_lock_struct const * lock)
{
    printf("dart_lock_initialized not yet supportet for gaspi!\n");
    return false;
}