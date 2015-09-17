#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <GASPI.h>

#include "dart_types.h"
#include "dart_translation.h"
#include "dart_team_private.h"
#include "dart_globmem.h"
#include "dart_team_group.h"

int16_t dart_memid;

dart_ret_t dart_team_memalloc_aligned(dart_team_t teamid, size_t nbytes, dart_gptr_t *gptr)
{
    size_t teamsize;
    dart_unit_t unitid, gptr_unitid = -1;
    dart_team_myid(teamid, &unitid);
    dart_team_size(teamid, &teamsize);

    gaspi_return_t retval;
    gaspi_group_t gaspi_group;
    gaspi_segment_id_t gaspi_seg_id;
    gaspi_segment_id_t * gaspi_seg_ids = (gaspi_segment_id_t *) malloc(sizeof(gaspi_segment_id_t) * teamsize);
    assert(gaspi_seg_ids);

    uint16_t index;
    int result = dart_adapt_teamlist_convert(teamid, &index);

    if (result == -1)
    {
        return DART_ERR_INVAL;
    }

    gaspi_group = dart_teams[index].id;

    dart_unit_t localid = 0;
    if(index == 0)
    {
        gptr_unitid = localid;
    }
    else
    {
        gptr_unitid = dart_teams[index].group.l2g[0];
    }

    /* get a free gaspi segment id */
    seg_stack_pop(&dart_free_coll_seg_ids, &gaspi_seg_id);

    /* Create the gaspi-segment with memory allocation */
    DART_CHECK_ERROR_RET(retval, gaspi_segment_create(gaspi_seg_id,
                                                      nbytes,
                                                      gaspi_group,
                                                      GASPI_BLOCK,
                                                      GASPI_MEM_UNINITIALIZED));

    *((gaspi_segment_id_t *) dart_gaspi_buffer_ptr) = gaspi_seg_id;
    /**
     * Allgather for gaspi
     *
     * using the same segment in allgather
     *  _____________ _____________
     * | send_buffer | recv_buffer |
     * -----------------------------
     */
    gaspi_offset_t recv_buffer_offset = sizeof(gaspi_segment_id_t);
    /**
     * collect the other segment numbers of the team
     */
    DART_CHECK_ERROR_RET(retval, gaspi_allgather(dart_gaspi_buffer_id,
                                                 0UL,
                                                 dart_gaspi_buffer_id,
                                                 recv_buffer_offset,
                                                 sizeof(gaspi_segment_id_t),
                                                 gaspi_group));

    // Get the pointer to the recv buffer in the segment
    gaspi_pointer_t recv_buffer_ptr = dart_gaspi_buffer_ptr + recv_buffer_offset;
    /**
     * Actual Soultion:
     *          -Copy data to heap memory for the translation table
     * Problem:
     *          -A memcpy must be executed
     *
     *  Other solution:
     *          -save pointer to segment in the translation table
     *  Problem:
     *          -the dart_gaspi_buffer has a static size and can't be resized
     *
     */
    memcpy(gaspi_seg_ids, recv_buffer_ptr, sizeof(gaspi_segment_id_t) * teamsize);

    /* -- Updating infos on gptr -- */
    gptr->unitid = gptr_unitid;
    gptr->segid = dart_memid; /* Segid equals to dart_memid (always a positive integer), identifies an unique collective global memory. */
    gptr->flags = index; /* For collective allocation, the flag is marked as 'index' */
    gptr->addr_or_offs.offset = 0;

    /* -- Updating the translation table of teamid with the created (segment) infos -- */
    info_t item;
    item.seg_id = dart_memid;
    item.size = nbytes;
    item.gaspi_seg_ids = gaspi_seg_ids;
    item.own_gaspi_seg_id = gaspi_seg_id;
    /* Add this newly generated correspondence relationship record into the translation table. */
    dart_adapt_transtable_add (item);
    dart_memid++;

    return retval;
}

dart_ret_t dart_team_memfree(dart_team_t teamid, dart_gptr_t gptr)
{
    gaspi_return_t retval;
    dart_unit_t unitid;
    dart_team_myid (teamid, &unitid);
    int16_t seg_id = gptr.segid;
    gaspi_segment_id_t gaspi_seg_id;

    if(dart_adapt_transtable_get_local_gaspi_seg_id(seg_id, &gaspi_seg_id) == -1)
    {
        return DART_ERR_INVAL;
    }

    DART_CHECK_ERROR_RET(retval, gaspi_segment_delete(gaspi_seg_id));

    DART_CHECK_ERROR_RET(retval, seg_stack_push(&dart_free_coll_seg_ids, gaspi_seg_id));

    /* Remove the related correspondence relation record from the related translation table. */
    if(dart_adapt_transtable_remove(seg_id) == -1)
    {
        return DART_ERR_INVAL;
    }

    return retval;
}

dart_ret_t dart_gptr_getaddr (const dart_gptr_t gptr, void **addr)
{
    gaspi_return_t retval = GASPI_SUCCESS;
    int16_t seg_id = gptr.segid;
    uint64_t offset = gptr.addr_or_offs.offset;
    dart_unit_t myid;
    dart_myid (&myid);

    if (myid == gptr.unitid)
    {
        if (seg_id)
        {
            gaspi_segment_id_t local_seg;
            gaspi_pointer_t seg_ptr;
            if (dart_adapt_transtable_get_local_gaspi_seg_id(seg_id, &local_seg) == -1)
            {
                return DART_ERR_INVAL;
            }
            DART_CHECK_ERROR_RET(retval, gaspi_segment_ptr(local_seg, addr));
            *addr = offset + (char *)(*addr);
        }
        //~ else
        //~ {
            //~ if (myid == gptr.unitid)
            //~ {
                //~ *addr = offset + dart_mempool_localalloc;
            //~ }

        //~ }
    }
    else
    {
        *addr = NULL;
    }

    return retval;
}

dart_ret_t dart_gptr_setaddr (dart_gptr_t* gptr, void* addr)
{
    gaspi_return_t retval = GASPI_SUCCESS;
    int16_t seg_id = gptr->segid;

    /* The modification to addr is reflected in the fact that modifying the offset. */
    if (seg_id)
    {
        gaspi_segment_id_t local_seg;
        gaspi_pointer_t local_seg_addr;
        if(dart_adapt_transtable_get_local_gaspi_seg_id(seg_id, &local_seg) == -1)
        {
            return DART_ERR_INVAL;
        }
        DART_CHECK_ERROR_RET(retval, gaspi_segment_ptr(local_seg, &local_seg_addr));
        gptr->addr_or_offs.offset = (char *)addr - (char *)local_seg_addr;
    }
    //~ else
    //~ {
        //~ gptr->addr_or_offs.offset = (char *)addr - dart_mempool_localalloc;
    //~ }
    return retval;
}

dart_ret_t dart_gptr_incaddr (dart_gptr_t* gptr, int offs)
{
    gptr->addr_or_offs.offset += offs;
    return DART_OK;
}


dart_ret_t dart_gptr_setunit (dart_gptr_t* gptr, dart_unit_t unit_id)
{
    gptr->unitid = unit_id;
    return DART_OK;
}
