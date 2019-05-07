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
    dart_datatype_struct_t* dtype_base =
      datatype_base_struct(get_datatype_struct(dtype));
    size_t nbytes = datatype_sizeof(dtype_base) * nelem;

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

dart_ret_t dart_team_mem_impl(
  dart_team_t       teamid,
  size_t            nelem,
  dart_datatype_t   dtype,
  void            * addr,
  dart_gptr_t     * gptr)
{
    size_t nbytes = dart_gaspi_datatype_sizeof(dtype) * nelem;
    size_t teamsize;
    dart_team_unit_t unitid;
    DART_CHECK_ERROR(dart_team_myid(teamid, &unitid));
    DART_CHECK_ERROR(dart_team_size(teamid, &teamsize));

    uint16_t index;
    int result = dart_adapt_teamlist_convert(teamid, &index);
    if (result == -1)
    {
        return DART_ERR_INVAL;
    }

    gaspi_group_t gaspi_group = dart_teams[index].id;

    // GPI2 can't allocate 0 bytes
    if( nbytes == 0 )
    {
        nbytes = 1;
    }

    /* get a free gaspi segment id */
    gaspi_segment_id_t   gaspi_seg_id;
    DART_CHECK_ERROR(seg_stack_pop(&pool_gaspi_seg_ids, &gaspi_seg_id));

    if(addr == NULL)
    {
        /* Create the gaspi-segment with memory allocation */
        DART_CHECK_GASPI_ERROR(gaspi_segment_create(gaspi_seg_id,
                                                    nbytes,
                                                    gaspi_group,
                                                    GASPI_BLOCK,
                                                    GASPI_MEM_INITIALIZED));
    }
    else
    {
        /* Binds and registers given memory to a gaspi-segment */
        DART_CHECK_GASPI_ERROR(gaspi_segment_use(gaspi_seg_id,
                                                 addr,
                                                 nbytes,
                                                 gaspi_group,
                                                 GASPI_BLOCK,
                                                 0));
    }

    /**
     * collect the other segment numbers of the team
     * gaspi_segemt_id_t is currently defined as unsigned char.
     * therefore DART_TYPE_BYTE is used as it has the same size
     */
    gaspi_segment_id_t * gaspi_seg_ids = (gaspi_segment_id_t *) malloc(sizeof(gaspi_segment_id_t) * teamsize);
    assert(gaspi_seg_ids);

    gaspi_return_t ret;
    DART_CHECK_GASPI_ERROR_RET(ret, dart_allgather(&gaspi_seg_id,
                                                    gaspi_seg_ids,
                                                    1,
                                                    DART_TYPE_BYTE,
                                                    teamid));
    if(ret != GASPI_SUCCESS)
    {
        free(gaspi_seg_ids);
        DART_CHECK_ERROR(gaspi_segment_delete(gaspi_seg_id));
        DART_CHECK_ERROR(seg_stack_push(&pool_gaspi_seg_ids, gaspi_seg_id));

        return DART_ERR_INVAL;
    }

    gaspi_segment_id_t dart_seg_id;
    DART_CHECK_ERROR(seg_stack_pop(&pool_dart_seg_ids, &dart_seg_id));

    /* sets gptr properties */
    gptr->unitid = unitid.id;
    gptr->segid = (int16_t) dart_seg_id; /* Segid equals to dart_memid (always a positive integer), identifies an unique collective global memory. */
    gptr->flags = index; /* For collective allocation, the flag is marked as 'index' */
    gptr->teamid = teamid;
    gptr->addr_or_offs.offset = 0;

    /* Creates new segment entry in the translation table */
    info_t item;
    item.seg_id           = (int16_t) dart_seg_id;
    item.size             = nbytes;
    item.gaspi_seg_ids    = gaspi_seg_ids;
    item.own_gaspi_seg_id = gaspi_seg_id;
    item.unit_count       = teamsize;

    /* Add this newly generated correspondence relationship record into the translation table. */
    dart_adapt_transtable_add(item);
    inital_rma_request_entry(item.seg_id);

    dart_barrier(teamid);

    return DART_OK;
}

dart_ret_t dart_team_memalloc_aligned(
  dart_team_t       teamid,
  size_t            nelem,
  dart_datatype_t   dtype,
  dart_gptr_t      *gptr)
{
    return dart_team_mem_impl(teamid, nelem, dtype, NULL, gptr);
}

dart_ret_t dart_team_memfree(dart_gptr_t gptr)
{
    int16_t            seg_id = gptr.segid;
    gaspi_segment_id_t gaspi_seg_id;
    /*
     * TODO May be wait on local completion ??
     */
    DART_CHECK_ERROR(delete_rma_requests(seg_id));
    if(dart_adapt_transtable_get_local_gaspi_seg_id(seg_id, &gaspi_seg_id) == -1)
    {
        return DART_ERR_INVAL;
    }

    if(seg_stack_isfull(&pool_gaspi_seg_ids))
    {
        printf("ERROR because of full seg_stack in dart_team_memfree\n");
    }

    DART_CHECK_ERROR(gaspi_segment_delete(gaspi_seg_id));

    DART_CHECK_ERROR(seg_stack_push(&pool_gaspi_seg_ids, gaspi_seg_id));

    /* Remove the related correspondence relation record from the related translation table. */
    if(dart_adapt_transtable_remove(seg_id) == -1)
    {
        return DART_ERR_INVAL;
    }

    DART_CHECK_ERROR(seg_stack_push(&pool_dart_seg_ids, seg_id));

    dart_barrier(gptr.teamid);

    return DART_OK;
}

dart_ret_t dart_team_memregister(
   dart_team_t       teamid,
   size_t            nelem,
   dart_datatype_t   dtype,
   void            * addr,
   dart_gptr_t     * gptr)
{
    if(addr == NULL)
    {
        DART_LOG_ERROR("Invalid memory address pointer -> NULL!"); \

        return DART_ERR_INVAL;
    }

    return dart_team_mem_impl(teamid, nelem, dtype, addr, gptr);
}

/*
 * gaspi_segment_delete deallocates memory + notify in case gaspi_segment_create was used,
 * but only deallocates the notify part for binded memory.
 * Therefore dart_team_memderegister can simply call dart_team_memfree.
 */
dart_ret_t dart_team_memderegister(dart_gptr_t gptr)
{
    return dart_team_memfree(gptr);
}

dart_ret_t dart_gptr_getaddr (const dart_gptr_t gptr, void **addr)
{
    int16_t     seg_id = gptr.segid;
    uint64_t    offset = gptr.addr_or_offs.offset;

    dart_team_unit_t unitid;
    DART_CHECK_ERROR(dart_team_myid(gptr.teamid, &unitid));

    if (unitid.id == gptr.unitid)
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
            if (unitid.id == gptr.unitid)
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
