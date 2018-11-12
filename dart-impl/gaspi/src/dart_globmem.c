#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <GASPI.h>


#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_globmem.h>
#include <dash/dart/if/dart_team_group.h>

#include <dash/dart/gaspi/dart_mem.h>
#include <dash/dart/gaspi/dart_translation.h>
#include <dash/dart/gaspi/dart_team_private.h>
#include <dash/dart/gaspi/dart_communication_priv.h>



int16_t dart_memid;

// (size_t nbytes, dart_gptr_t *gptr)
dart_ret_t dart_memalloc(
  size_t            nelem,
  dart_datatype_t   dtype,
  dart_gptr_t     * gptr)
{

    size_t nbytes = dart_gaspi_datatype_sizeof(dtype) * nelem;
    dart_global_unit_t unitid;
    dart_myid(&unitid);
    gptr->unitid = unitid.id;
    gptr->segid = 0; /* For local allocation, the segid is marked as '0'. */
    gptr->flags = 0; /* For local allocation, the flag is marked as '0'. */
    gptr->segid   = DART_TEAM_ALL; /* For local allocation, the segid is marked as '0'. */
    gptr->addr_or_offs.offset = dart_buddy_alloc(dart_localpool, nbytes);
    if (gptr->addr_or_offs.offset == -1)
    {
        fprintf(stderr, "Out of bound: the global memory is exhausted");
        return DART_ERR_OTHER;
    }
    return DART_OK;
}

dart_ret_t dart_memfree (dart_gptr_t gptr)
{
    if (dart_buddy_free (dart_localpool, gptr.addr_or_offs.offset) == -1)
    {
        fprintf(stderr, "Free invalid local global pointer: invalid offset = %lu\n", gptr.addr_or_offs.offset);
        return DART_ERR_INVAL;
    }

    return DART_OK;
}
//(dart_team_t teamid, size_t nbytes, dart_gptr_t *gptr)

dart_ret_t dart_team_memalloc_aligned(
  dart_team_t       teamid,
  size_t            nelem,
  dart_datatype_t   dtype,
  dart_gptr_t      *gptr)
{
    size_t nbytes = dart_gaspi_datatype_sizeof(dtype) * nelem;
    size_t teamsize;
    dart_unit_t gptr_unitid = -1;
    dart_team_unit_t unitid;
    DART_CHECK_ERROR(dart_team_myid(teamid, &unitid));
    DART_CHECK_ERROR(dart_team_size(teamid, &teamsize));

    gaspi_group_t        gaspi_group;
    gaspi_segment_id_t   gaspi_seg_id;
    gaspi_segment_id_t * gaspi_seg_ids = (gaspi_segment_id_t *) malloc(sizeof(gaspi_segment_id_t) * teamsize);
    assert(gaspi_seg_ids);

    uint16_t index;
    int result = dart_adapt_teamlist_convert(teamid, &index);
    if (result == -1)
    {
        goto dart_error_label;
    }

    gaspi_group = dart_teams[index].id;

    dart_unit_t localid = 0;
    if(index == 0)
    {
        gptr_unitid = localid;
    }
    else
    {
        gptr_unitid = dart_teams[index].group->l2g[0];
    }

    /* get a free gaspi segment id */
    DART_CHECK_ERROR_GOTO(dart_error_label, seg_stack_pop(&dart_free_coll_seg_ids, &gaspi_seg_id));

    /* Create the gaspi-segment with memory allocation */
    DART_CHECK_ERROR_GOTO(dart_error_label, gaspi_segment_create(gaspi_seg_id,
                                                                 nbytes,
                                                                 gaspi_group,
                                                                 GASPI_BLOCK,
                                                                 GASPI_MEM_INITIALIZED));
    /**
     * collect the other segment numbers of the team
     * gaspi_segemt_id_t is currently defined as unsigned char.
     * therefore DART_TYPE_BYTE is used as it has the same size
     */
    DART_CHECK_ERROR_GOTO(dart_error_label, dart_allgather(&gaspi_seg_id,
                                                           gaspi_seg_ids,
                                                           1,
                                                           DART_TYPE_BYTE,
                                                           teamid));
    /* -- Updating infos on gptr -- */
    gptr->unitid = gptr_unitid;
    gptr->segid = dart_memid; /* Segid equals to dart_memid (always a positive integer), identifies an unique collective global memory. */
    gptr->flags = index; /* For collective allocation, the flag is marked as 'index' */
    gptr->teamid = teamid;
    gptr->addr_or_offs.offset = 0;

    /* -- Updating the translation table of teamid with the created (segment) infos -- */
    info_t item;
    item.seg_id           = dart_memid;
    item.size             = nbytes;
    item.gaspi_seg_ids    = gaspi_seg_ids;
    item.own_gaspi_seg_id = gaspi_seg_id;
    item.unit_count       = teamsize;

    /* Add this newly generated correspondence relationship record into the translation table. */
    dart_adapt_transtable_add(item);
    DART_CHECK_ERROR_GOTO(dart_error_label, inital_rma_request_entry(item.seg_id));
    dart_memid++;

    return DART_OK;

dart_error_label:
    free(gaspi_seg_ids);
    return DART_ERR_INVAL;

}

dart_ret_t dart_team_memfree(dart_gptr_t gptr)
{
    int16_t            seg_id = gptr.segid;
    gaspi_segment_id_t segs;
    /*
     * TODO May be wait on local completion ??
     */
    DART_CHECK_ERROR(delete_rma_requests(seg_id));
    if(dart_adapt_transtable_get_local_gaspi_seg_id(seg_id, &segs) == -1)
    {
        return DART_ERR_INVAL;
    }

    // Debug purposes
    /*gaspi_rank_t gaspi_myid;
    gaspi_proc_rank(&gaspi_myid);
    bool loop = 1;
    if(0==gaspi_myid)
    {
        while( loop ){}
    }
    gaspi_barrier(GASPI_GROUP_ALL, GASPI_BLOCK);*/
    // ==============
    printf(">>>>> team_memfree for segs:%d and seg_id:%d <<<<<\n", segs, seg_id);
    if(seg_stack_isfull(&dart_free_coll_seg_ids))
    {
        printf(">>>>> ERROR because of full seg_stack in dart_team_memfree\n");
    }

    DART_CHECK_ERROR(gaspi_segment_delete(segs));
    DART_CHECK_ERROR(seg_stack_push(&dart_free_coll_seg_ids, segs));
    
    /* Remove the related correspondence relation record from the related translation table. */
    if(dart_adapt_transtable_remove(seg_id) == -1)
    {
        return DART_ERR_INVAL;
    }

    return DART_OK;
}

/*
 *DASH documantation says, it wont deallocate memory. In fact, gaspi only supports
 *delete with similar deallocation of the segment. Therefore it will be dealocated
 *if invoked in it's current implementation. Basically renamed dart_team_memfree.
 */
dart_ret_t dart_team_memderegister(dart_gptr_t gptr)
{
    int16_t            seg_id = gptr.segid;
    gaspi_segment_id_t segs;
    /*
     * TODO May be wait on local completion ??
     */
    DART_CHECK_ERROR(delete_rma_requests(seg_id));
    if(dart_adapt_transtable_get_local_gaspi_seg_id(seg_id, &segs) == -1)
    {
        return DART_ERR_INVAL;
    }

    DART_CHECK_ERROR(gaspi_segment_delete(segs));

    DART_CHECK_ERROR(seg_stack_push(&dart_free_coll_seg_ids, segs));

    /* Remove the related correspondence relation record from the related translation table. */
    if(dart_adapt_transtable_remove(seg_id) == -1)
    {
        return DART_ERR_INVAL;
    }

    return DART_OK;
}

dart_ret_t dart_gptr_getaddr (const dart_gptr_t gptr, void **addr)
{
    dart_global_unit_t myid;
    int16_t     seg_id = gptr.segid;
    uint64_t    offset = gptr.addr_or_offs.offset;

    DART_CHECK_ERROR(dart_myid (&myid));

    if (myid.id == gptr.unitid)
    {
        if (seg_id)
        {
            gaspi_segment_id_t local_seg;
            if (dart_adapt_transtable_get_local_gaspi_seg_id(seg_id, &local_seg) == -1)
            {
                return DART_ERR_INVAL;
            }
            DART_CHECK_ERROR(gaspi_segment_ptr(local_seg, addr));
            *addr = offset + (char *)(*addr);
        }
        else
        {
            if (myid.id == gptr.unitid)
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

dart_ret_t dart_gptr_setaddr(dart_gptr_t* gptr, void* addr)
{
    int16_t seg_id = gptr->segid;

    /* The modification to addr is reflected in the fact that modifying the offset. */
    if (seg_id)
    {
        gaspi_segment_id_t local_seg;
        gaspi_pointer_t    local_seg_addr;
        if(dart_adapt_transtable_get_local_gaspi_seg_id(seg_id, &local_seg) == -1)
        {
            return DART_ERR_INVAL;
        }
        DART_CHECK_ERROR(gaspi_segment_ptr(local_seg, &local_seg_addr));
        gptr->addr_or_offs.offset = (char *)addr - (char *)local_seg_addr;
    }
    else
    {
        gptr->addr_or_offs.offset = (char *)addr - dart_mempool_localalloc;
    }
    return DART_OK;
}
/*
Implementation already defined in dart_globalmem.h
dart_ret_t dart_gptr_incaddr(
  dart_gptr_t * gptr,
  int64_t       offs)
{
    gptr->addr_or_offs.offset += offs;
    return DART_OK;
}


dart_ret_t dart_gptr_setunit(
  dart_gptr_t      * gptr,
  dart_team_unit_t   unit)
{
    gptr->unitid = unit.id;
    return DART_OK;
}
*/
dart_ret_t dart_team_memregister(
   dart_team_t       teamid,
   size_t            nelem,
   dart_datatype_t   dtype,
   void            * addr,
   dart_gptr_t     * gptr)
{
    size_t nbytes = dart_gaspi_datatype_sizeof(dtype) * nelem;
    size_t teamsize;
    dart_unit_t gptr_unitid = -1;
    dart_team_unit_t unitid;
    DART_CHECK_ERROR(dart_team_myid(teamid, &unitid));
    DART_CHECK_ERROR(dart_team_size(teamid, &teamsize));

    gaspi_group_t        gaspi_group;
    gaspi_segment_id_t   gaspi_seg_id;
    gaspi_segment_id_t * gaspi_seg_ids = (gaspi_segment_id_t *) malloc(sizeof(gaspi_segment_id_t) * teamsize);
    assert(gaspi_seg_ids);

    uint16_t index;
    int result = dart_adapt_teamlist_convert(teamid, &index);
    if (result == -1)
    {
        goto dart_error_label;
    }

    gaspi_group = dart_teams[index].id;

    dart_unit_t localid = 0;
    if(index == 0)
    {
        gptr_unitid = localid;
    }
    else
    {
        gptr_unitid = dart_teams[index].group->l2g[0];
    }
    gaspi_memory_description_t segment_descrition = 0;
    /* get a free gaspi segment id */
    DART_CHECK_ERROR_GOTO(dart_error_label, seg_stack_pop(&dart_free_coll_seg_ids, &gaspi_seg_id));

    /* Create the gaspi-segment with memory allocation */
    DART_CHECK_ERROR_GOTO(dart_error_label, gaspi_segment_bind(gaspi_seg_id,
                                                                 addr,
                                                                 nbytes,
                                                                 segment_descrition));
    /**
                                                                  * collect the other segment numbers of the team
                                                                  * gaspi_segemt_id_t is currently defined as unsigned char.
                                                                  * therefore DART_TYPE_BYTE is used as it has the same size
                                                                  */
    DART_CHECK_ERROR_GOTO(dart_error_label, dart_allgather(&gaspi_seg_id,
                                                           gaspi_seg_ids,
                                                           1,
                                                           DART_TYPE_BYTE,
                                                           teamid));
    /* -- Updating infos on gptr -- */
    gptr->unitid = gptr_unitid;
    gptr->segid  = dart_memid; /* Segid equals to dart_memid (always a positive integer), identifies an unique collective global memory. */
    gptr->flags  = index; /* For collective allocation, the flag is marked as 'index' */
    gptr->teamid = teamid;
    gptr->addr_or_offs.offset = 0;

    /* -- Updating the translation table of teamid with the created (segment) infos -- */
    info_t item;
    item.seg_id           = dart_memid;
    item.size             = nbytes;
    item.gaspi_seg_ids    = gaspi_seg_ids;
    item.own_gaspi_seg_id = gaspi_seg_id;
    item.unit_count       = teamsize;

    /* Add this newly generated correspondence relationship record into the translation table. */
    dart_adapt_transtable_add(item);
    DART_CHECK_ERROR_GOTO(dart_error_label, inital_rma_request_entry(item.seg_id));
    dart_memid++;

    return DART_OK;

dart_error_label:
    free(gaspi_seg_ids);
    return DART_ERR_INVAL;

}
