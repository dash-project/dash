
#include <dash/dart/gaspi/dart_gaspi.h>

#include <dash/dart/if/dart_communication.h>
#include <dash/dart/gaspi/dart_team_private.h>
#include <dash/dart/gaspi/dart_translation.h>
#include <dash/dart/gaspi/dart_mem.h>
#include <dash/dart/gaspi/dart_communication_priv.h>
#include <string.h>
/*********************** Notify Value ************************/
gaspi_notification_t dart_notify_value = 42;


/********************** Only for testing *********************/
gaspi_queue_id_t dart_handle_get_queue(dart_handle_t handle)
{
    return handle->queue;
}
/*************************************************************/
/**
 * TODO dart_scatter not implemented yet
 */
dart_ret_t dart_scatter(void *sendbuf, void *recvbuf, size_t nbytes, dart_unit_t root, dart_team_t team)
{
    return DART_ERR_OTHER;
}
/**
 * TODO dart_gather not implemented yet
 */
dart_ret_t dart_gather(void *sendbuf, void *recvbuf, size_t nbytes, dart_unit_t root, dart_team_t team)
{
    return DART_ERR_OTHER;
}

dart_ret_t dart_bcast(void *buf, size_t nbytes, dart_unit_t root, dart_team_t team)
{
    uint16_t        index;
    dart_unit_t     myid;
    gaspi_pointer_t seg_ptr = NULL;

    if(nbytes > DART_GASPI_BUFFER_SIZE)
    {
        fprintf(stderr, "Memory for collective operation is exhausted\n");
        return DART_ERR_OTHER;
    }

    DART_CHECK_ERROR(dart_myid(&myid));
    DART_CHECK_GASPI_ERROR(gaspi_segment_ptr(dart_gaspi_buffer_id, &seg_ptr));

    if(myid == root)
    {
        memcpy(seg_ptr, buf, nbytes);
    }
    int result = dart_adapt_teamlist_convert(team, &index);
    if(result == -1)
    {
        return DART_ERR_INVAL;
    }
    DART_CHECK_GASPI_ERROR(gaspi_bcast(dart_gaspi_buffer_id,
                                       0UL,
                                       nbytes,
                                       root,
                                       dart_teams[index].id));

    if(myid != root)
    {
        memcpy(buf, seg_ptr, nbytes);
    }

    return DART_OK;
}

dart_ret_t dart_allgather(void *sendbuf, void *recvbuf, size_t nbytes, dart_team_t team)
{
    gaspi_pointer_t send_ptr = NULL;
    gaspi_pointer_t recv_ptr = NULL;
    gaspi_offset_t  recv_offset = nbytes;
    size_t          teamsize;
    uint16_t        index;

    DART_CHECK_GASPI_ERROR(dart_team_size(team, &teamsize));

    if(((teamsize * nbytes) + nbytes) > DART_GASPI_BUFFER_SIZE)
    {
        fprintf(stderr, "Memory for collective operation is exhausted\n");
        return DART_ERR_OTHER;
    }

    DART_CHECK_GASPI_ERROR(gaspi_segment_ptr(dart_gaspi_buffer_id, &send_ptr));
    recv_ptr = (gaspi_pointer_t) ((char *) send_ptr + recv_offset);

    memcpy(send_ptr, sendbuf, nbytes);

    int result = dart_adapt_teamlist_convert(team, &index);
    if (result == -1)
    {
        return DART_ERR_INVAL;
    }

    DART_CHECK_GASPI_ERROR(gaspi_allgather(dart_gaspi_buffer_id,
                                           0UL,
                                           dart_gaspi_buffer_id,
                                           recv_offset,
                                           nbytes,
                                           dart_teams[index].id));

    memcpy(recvbuf, recv_ptr, nbytes * teamsize);

    return DART_OK;
}

dart_ret_t dart_barrier (dart_team_t teamid)
{
    uint16_t       index;
    gaspi_group_t  gaspi_group_id;

    int result = dart_adapt_teamlist_convert (teamid, &index);
    if (result == -1)
    {
        return DART_ERR_INVAL;
    }
    gaspi_group_id = dart_teams[index].id;
    DART_CHECK_GASPI_ERROR(gaspi_barrier(gaspi_group_id, GASPI_BLOCK));

    return DART_OK;
}

dart_ret_t dart_handle_notify(dart_handle_t handle, unsigned int tag)
{
    return DART_ERR_NOTFOUND;
}

dart_ret_t dart_notify(dart_gptr_t gptr, unsigned int tag)
{
    dart_unit_t        myid;
    dart_unit_t        my_rel_id;
    gaspi_queue_id_t   queue;
    gaspi_segment_id_t gaspi_seg_id    = dart_mempool_seg_localalloc;
    int16_t            seg_id          = gptr.segid;
    uint16_t           index           = gptr.flags;
    dart_unit_t        target_unit     = gptr.unitid;

    DART_CHECK_ERROR(dart_myid(&myid));

    if(seg_id)
    {
        dart_unit_t rel_target_unit;
        DART_CHECK_ERROR(unit_g2l(index, target_unit, &rel_target_unit));
        if(dart_adapt_transtable_get_gaspi_seg_id(seg_id, rel_target_unit, &gaspi_seg_id) == -1)
        {
            fprintf(stderr, "Can't find given target segment id in dart_notify\n");
            return DART_ERR_NOTFOUND;
        }
    }

    int32_t found = 0;
    DART_CHECK_ERROR(find_rma_request(target_unit, seg_id, &queue, &found));
    if(!found)
    {
        DART_CHECK_ERROR(dart_get_minimal_queue(&queue));
    }

    DART_CHECK_ERROR(unit_g2l(index, myid, &my_rel_id));
    DART_CHECK_GASPI_ERROR(check_queue_size(queue));

    DART_CHECK_GASPI_ERROR(gaspi_notify(gaspi_seg_id, target_unit, my_rel_id, tag, queue, GASPI_BLOCK));

    return DART_OK;
}

dart_ret_t dart_notify_wait(dart_gptr_t gptr, unsigned int * tag)
{
    gaspi_notification_id_t notify_received;
    gaspi_segment_id_t      gaspi_seg_id = dart_mempool_seg_localalloc;
    int16_t                 seg_id       = gptr.segid;
    uint16_t                index        = gptr.flags;
    size_t                  team_size    = dart_teams[index].group.nmem;

    if(seg_id)
    {
        if(dart_adapt_transtable_get_local_gaspi_seg_id(seg_id, &gaspi_seg_id) == -1)
        {
            fprintf(stderr, "Can't find given segment id in dart_notify_wait\n");
        }
    }

    DART_CHECK_GASPI_ERROR(gaspi_notify_waitsome(gaspi_seg_id, 0, team_size, &notify_received, GASPI_BLOCK));
    DART_CHECK_GASPI_ERROR(gaspi_notify_reset(gaspi_seg_id, notify_received, tag));

    return DART_OK;
}

dart_ret_t dart_get_gptr(dart_gptr_t dest, dart_gptr_t src, size_t nbytes)
{
    gaspi_queue_id_t   queue;
    /*
     * local site
     */
    gaspi_segment_id_t dest_gaspi_seg = dart_mempool_seg_localalloc;
    gaspi_offset_t     dest_offset    = dest.addr_or_offs.offset;
    uint16_t           dest_index     = dest.flags;
    int16_t            dest_seg_id    = dest.segid;
    dart_unit_t        my_unit        = dest.unitid;
    /*
     * remote site
     */
    gaspi_segment_id_t src_gaspi_seg  = dart_mempool_seg_localalloc;
    gaspi_offset_t     src_offset     = src.addr_or_offs.offset;
    int16_t            src_seg_id     = src.segid;
    uint16_t           src_index      = src.flags;
    dart_unit_t        target_unit    = src.unitid;
    /*
     * local site
     */
    if(dest_seg_id)
    {
        dart_unit_t rel_my_unit;
        DART_CHECK_ERROR(unit_g2l(dest_index, my_unit, &rel_my_unit));
        if(dart_adapt_transtable_get_gaspi_seg_id(dest_seg_id, rel_my_unit, &dest_gaspi_seg) == -1)
        {
            fprintf(stderr, "Can't find given destination segment id in dart_get_blocking\n");
            return DART_ERR_NOTFOUND;
        }
    }
    /*
     * remote site
     */
    if(src_seg_id)
    {
        dart_unit_t rel_target_unit;
        DART_CHECK_ERROR(unit_g2l(src_index, target_unit, &rel_target_unit));
        if(dart_adapt_transtable_get_gaspi_seg_id(src_seg_id, rel_target_unit, &src_gaspi_seg) == -1)
        {
            fprintf(stderr, "Can't find given source segment id in dart_get_blocking\n");
            return DART_ERR_NOTFOUND;
        }
    }
    int32_t found = 0;
    DART_CHECK_ERROR(find_rma_request(target_unit, src_seg_id, &queue, &found));
    if(!found)
    {
        DART_CHECK_ERROR(dart_get_minimal_queue(&queue));
        DART_CHECK_ERROR(add_rma_request_entry(target_unit, src_seg_id, queue));
    }
    else
    {
        DART_CHECK_GASPI_ERROR(check_queue_size(queue));
    }

    DART_CHECK_GASPI_ERROR(gaspi_read(dest_gaspi_seg,
                                      dest_offset,
                                      target_unit,
                                      src_gaspi_seg,
                                      src_offset,
                                      nbytes,
                                      queue,
                                      GASPI_BLOCK));
    return DART_OK;
}


dart_ret_t dart_put_gptr(dart_gptr_t dest, dart_gptr_t src, size_t nbytes)
{
    gaspi_queue_id_t   queue;
    /*
     * local site
     */
    gaspi_segment_id_t src_gaspi_seg  = dart_mempool_seg_localalloc;
    gaspi_offset_t     src_offset     = src.addr_or_offs.offset;
    int16_t            src_seg_id     = src.segid;
    uint16_t           src_index      = src.flags;
    dart_unit_t        my_unit        = src.unitid;
    /*
     * remote site
     */
    gaspi_segment_id_t dest_gaspi_seg = dart_mempool_seg_localalloc;
    gaspi_offset_t     dest_offset    = dest.addr_or_offs.offset;
    int16_t            dest_seg_id    = dest.segid;
    uint16_t           dest_index     = dest.flags;
    dart_unit_t        target_unit    = dest.unitid;
    /*
     * local site
     */
    if(src_seg_id)
    {
        dart_unit_t rel_my_unit;
        DART_CHECK_ERROR(unit_g2l(src_index, my_unit, &rel_my_unit));
        if(dart_adapt_transtable_get_gaspi_seg_id(src_seg_id, rel_my_unit, &src_gaspi_seg) == -1)
        {
            fprintf(stderr, "Can't find given source segment id in dart_put_gptr\n");
            return DART_ERR_NOTFOUND;
        }
    }
    /*
     * remote site
     */
    if(dest_seg_id)
    {
        dart_unit_t rel_target_unit;
        DART_CHECK_ERROR(unit_g2l(dest_index, target_unit, &rel_target_unit));
        if(dart_adapt_transtable_get_gaspi_seg_id(dest_seg_id, rel_target_unit, &dest_gaspi_seg) == -1)
        {
            fprintf(stderr, "Can't find given destination segment id in dart_put_gptr\n");
            return DART_ERR_NOTFOUND;
        }
    }
    int32_t found = 0;
    DART_CHECK_ERROR(find_rma_request(target_unit, dest_seg_id, &queue, &found));
    if(!found)
    {
        DART_CHECK_ERROR(dart_get_minimal_queue(&queue));
        DART_CHECK_ERROR(add_rma_request_entry(target_unit, dest_seg_id, queue));
    }
    else
    {
        DART_CHECK_GASPI_ERROR(check_queue_size(queue));
    }

    DART_CHECK_GASPI_ERROR(gaspi_write(src_gaspi_seg,
                                       src_offset,
                                       target_unit,
                                       dest_gaspi_seg,
                                       dest_offset,
                                       nbytes,
                                       queue,
                                       GASPI_BLOCK));
    return DART_OK;
}

dart_ret_t dart_get_gptr_blocking(dart_gptr_t dest, dart_gptr_t src, size_t nbytes)
{
    gaspi_queue_id_t   queue;
    /*
     * local site
     */
    gaspi_segment_id_t dest_gaspi_seg = dart_mempool_seg_localalloc;
    gaspi_offset_t     dest_offset    = dest.addr_or_offs.offset;
    uint16_t           dest_index     = dest.flags;
    int16_t            dest_seg_id    = dest.segid;
    dart_unit_t        my_unit        = dest.unitid;
    /*
     * remote site
     */
    gaspi_segment_id_t src_gaspi_seg  = dart_mempool_seg_localalloc;
    gaspi_offset_t     src_offset     = src.addr_or_offs.offset;
    int16_t            src_seg_id     = src.segid;
    uint16_t           src_index      = src.flags;
    dart_unit_t        target_unit    = src.unitid;
    /*
     * local site
     */
    if(dest_seg_id)
    {
        dart_unit_t rel_my_unit;
        DART_CHECK_ERROR(unit_g2l(dest_index, my_unit, &rel_my_unit));
        if(dart_adapt_transtable_get_gaspi_seg_id(dest_seg_id, rel_my_unit, &dest_gaspi_seg) == -1)
        {
            fprintf(stderr, "Can't find given destination segment id in dart_get_blocking\n");
            return DART_ERR_NOTFOUND;
        }
    }
    /*
     * remote site
     */
    if(src_seg_id)
    {
        dart_unit_t rel_target_unit;
        DART_CHECK_ERROR(unit_g2l(src_index, target_unit, &rel_target_unit));
        if(dart_adapt_transtable_get_gaspi_seg_id(src_seg_id, rel_target_unit, &src_gaspi_seg) == -1)
        {
            fprintf(stderr, "Can't find given source segment id in dart_get_blocking\n");
            return DART_ERR_NOTFOUND;
        }
    }
    /*
     * target is own unit
     */
    if(my_unit == target_unit)
    {
        gaspi_pointer_t src_seg_ptr = NULL;
        gaspi_pointer_t dest_seg_ptr = NULL;

        DART_CHECK_GASPI_ERROR(gaspi_segment_ptr(src_gaspi_seg, &src_seg_ptr));
        DART_CHECK_GASPI_ERROR(gaspi_segment_ptr(dest_gaspi_seg, &dest_seg_ptr));

        src_seg_ptr  = (void *)((char *) src_seg_ptr  + src_offset);
        dest_seg_ptr = (void *)((char *) dest_seg_ptr + dest_offset);

        memcpy(dest_seg_ptr, src_seg_ptr, nbytes);

        return DART_OK;
    }

    DART_CHECK_ERROR(dart_get_minimal_queue(&queue));

    DART_CHECK_GASPI_ERROR(gaspi_read(dest_gaspi_seg,
                                      dest_offset,
                                      target_unit,
                                      src_gaspi_seg,
                                      src_offset,
                                      nbytes,
                                      queue,
                                      GASPI_BLOCK));

    DART_CHECK_GASPI_ERROR(gaspi_wait(queue, GASPI_BLOCK));

    return DART_OK;
}

dart_ret_t dart_create_handle(dart_handle_t * handle)
{
    *handle = (dart_handle_t) malloc (sizeof (struct dart_handle_struct));
    assert(*handle);
    return DART_OK;
}

dart_ret_t dart_delete_handle(dart_handle_t * handle)
{
    free(*handle);
    return DART_OK;
}

dart_ret_t dart_get_gptr_handle(dart_gptr_t dest, dart_gptr_t src, size_t nbytes, dart_handle_t handle)
{
    gaspi_queue_id_t   queue;
    /*
     * local site
     */
    gaspi_segment_id_t dest_gaspi_seg = dart_mempool_seg_localalloc;
    gaspi_offset_t     dest_offset    = dest.addr_or_offs.offset;
    uint16_t           dest_index     = dest.flags;
    int16_t            dest_seg_id    = dest.segid;
    dart_unit_t        my_unit        = dest.unitid;
    /*
     * remote site
     */
    gaspi_segment_id_t src_gaspi_seg  = dart_mempool_seg_localalloc;
    gaspi_offset_t     src_offset     = src.addr_or_offs.offset;
    int16_t            src_seg_id     = src.segid;
    uint16_t           src_index      = src.flags;
    dart_unit_t        target_unit    = src.unitid;

    if(handle == NULL)
    {
        fprintf(stderr, "dart_get_gptr_handle : Handle structure not initialized\n");
        return DART_ERR_OTHER;
    }
    /*
     * local site
     */
    if(dest_seg_id)
    {

        dart_unit_t rel_my_unit;
        DART_CHECK_ERROR(unit_g2l(dest_index, my_unit, &rel_my_unit));
        if(dart_adapt_transtable_get_gaspi_seg_id(dest_seg_id, rel_my_unit, &dest_gaspi_seg) == -1)
        {
            fprintf(stderr, "Can't find given destination segment id in dart_get_blocking\n");
            return DART_ERR_NOTFOUND;
        }
    }
    /*
     * remote site
     */
    if(src_seg_id)
    {
        dart_unit_t rel_target_unit;
        DART_CHECK_ERROR(unit_g2l(src_index, target_unit, &rel_target_unit));
        if(dart_adapt_transtable_get_gaspi_seg_id(src_seg_id, rel_target_unit, &src_gaspi_seg) == -1)
        {
            fprintf(stderr, "Can't find given source segment id in dart_get_blocking\n");
            return DART_ERR_NOTFOUND;
        }
    }

    DART_CHECK_ERROR(dart_get_minimal_queue(&queue));

    handle->queue      = queue;
    handle->local_seg  = src_gaspi_seg;
    handle->remote_seg = dest_gaspi_seg;

    DART_CHECK_GASPI_ERROR(gaspi_read(dest_gaspi_seg,
                                      dest_offset,
                                      target_unit,
                                      src_gaspi_seg,
                                      src_offset,
                                      nbytes,
                                      queue,
                                      GASPI_BLOCK));

    return DART_OK;
}
//~ dart_ret_t dart_put_handle(dart_gptr_t gptr, void *src, size_t nbytes, dart_handle_t *handle)
//~ {
    //~ gaspi_queue_id_t   queue;
    //~ gaspi_segment_id_t remote_seg;
    //~ gaspi_pointer_t    local_seg_ptr = NULL;
    //~ uint64_t           remote_offset = gptr.addr_or_offs.offset;
    //~ int16_t            seg_id        = gptr.segid;
    //~ uint16_t           index         = gptr.flags;
    //~ dart_unit_t        remote_rank   = gptr.unitid;

    //~ *handle = (dart_handle_t) malloc (sizeof (struct dart_handle_struct));
    //~ assert(*handle);

    //~ (*handle)->local_offset = dart_buddy_alloc(dart_transferpool, nbytes);
    //~ if((*handle)->local_offset == -1)
    //~ {
        //~ fprintf(stderr, "Out of bound: the global memory is exhausted");
        //~ return DART_ERR_OTHER;
    //~ }

    //~ DART_CHECK_ERROR(gaspi_segment_ptr(dart_transferpool_seg, &local_seg_ptr));
    //~ local_seg_ptr = (void *) ((char *) local_seg_ptr + ((*handle)->local_offset));

    //~ memcpy(local_seg_ptr, src, nbytes);

    //~ // TODO function must ensure that the returned queue has the size for a later notify
    //~ // to signal the availability of the data for the target rank
    //~ DART_CHECK_ERROR(dart_get_minimal_queue(&queue));

    //~ (*handle)->local_seg   = dart_transferpool_seg;
    //~ (*handle)->dest_buffer = NULL; // to indicate the put operation in wait or test
    //~ (*handle)->queue       = queue;
    //~ (*handle)->nbytes      = nbytes;

    //~ if(seg_id)
    //~ {
        //~ dart_unit_t rel_remote_rank;
        //~ DART_CHECK_ERROR(unit_g2l(index, remote_rank, &rel_remote_rank));
        //~ if(dart_adapt_transtable_get_gaspi_seg_id(seg_id, rel_remote_rank, &remote_seg) == -1)
        //~ {
            //~ fprintf(stderr, "Can't find given segment id in dart_put_handle\n");
            //~ return DART_ERR_NOTFOUND;
        //~ }
    //~ }
    //~ else
    //~ {
        //~ remote_seg = dart_mempool_seg_localalloc;
    //~ }

    //~ DART_CHECK_ERROR(gaspi_write(dart_transferpool_seg,
                                 //~ (*handle)->local_offset,
                                 //~ remote_rank,
                                 //~ remote_seg,
                                 //~ remote_offset,
                                 //~ nbytes,
                                 //~ queue,
                                 //~ GASPI_BLOCK));
    //~ return DART_OK;
//~ }

dart_ret_t dart_wait_local (dart_handle_t handle)
{
    if (handle == NULL)
    {
        return DART_ERR_INVAL;
    }

    DART_CHECK_GASPI_ERROR(gaspi_wait(handle->queue, GASPI_BLOCK));
    return DART_OK;
}

dart_ret_t dart_waitall_local(dart_handle_t * handle, size_t n)
{
    if (handle == NULL)
    {
        return DART_ERR_INVAL;
    }
    for(size_t i = 0 ; i < n ; ++i)
    {
        DART_CHECK_ERROR(dart_wait_local(handle[i]));
    }

    return DART_OK;
}

dart_ret_t dart_test_local (dart_handle_t handle, int32_t* is_finished)
{
    if(handle == NULL)
    {
        return DART_ERR_INVAL;
    }

    gaspi_return_t retval = gaspi_wait(handle->queue, GASPI_TEST);

    if(retval == GASPI_SUCCESS)
    {
        *is_finished = 1;
        return DART_OK;
    }
    else if(retval == GASPI_ERROR)
    {
        fprintf(stderr, "Error: gaspi_wait failed in dart_test_local\n");
        return DART_ERR_OTHER;
    }

    *is_finished = 0;
    return DART_OK;
}

dart_ret_t dart_testall_local (dart_handle_t *handle, size_t n, int32_t* is_finished)
{
    for(size_t i = 0 ; i < n ; ++i)
    {
        DART_CHECK_ERROR(dart_test_local(handle[i], is_finished));
        if(! *is_finished )
        {
            return DART_OK;
        }
    }
    return DART_OK;
}

dart_ret_t _dart_flush_local(dart_gptr_t gptr, gaspi_timeout_t timeout, gaspi_return_t * ret)
{
    int16_t          seg_id      = gptr.segid;
    dart_unit_t      remote_rank = gptr.unitid;
    int32_t          found       = 0;
    gaspi_queue_id_t qid;


    DART_CHECK_ERROR(find_rma_request(remote_rank, seg_id, &qid, &found));
    if(found)
    {
        *ret = gaspi_wait(qid, timeout);
    }
    else
    {
        gaspi_printf("No queue found\n");
    }

    return DART_OK;
}

dart_ret_t dart_flush_local(dart_gptr_t gptr)
{
    gaspi_return_t ret = GASPI_SUCCESS;
    DART_CHECK_ERROR(_dart_flush_local(gptr, GASPI_BLOCK, &ret));
    if(ret != GASPI_SUCCESS)
    {
        gaspi_printf("ERROR in %s at line %d in file %s\n","_dart_flush_local", __LINE__, __FILE__);
        return DART_ERR_OTHER;
    }
    return DART_OK;
}

dart_ret_t dart_flush_local_all(dart_gptr_t gptr)
{
    int16_t seg_id      = gptr.segid;

    request_iterator_t iter = new_request_iter(seg_id);

    while(request_iter_is_vaild(iter))
    {
        gaspi_queue_id_t qid;
        DART_CHECK_ERROR(request_iter_get_queue(iter, &qid));

        DART_CHECK_GASPI_ERROR(gaspi_wait(qid, GASPI_BLOCK));

        DART_CHECK_ERROR(request_iter_next(iter));
    }

    DART_CHECK_ERROR(destroy_request_iter(iter));
    return DART_OK;
}
