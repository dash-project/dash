
#include <dash/dart/gaspi/dart_gaspi.h>

#include <dash/dart/if/dart_communication.h>
#include <dash/dart/gaspi/dart_team_private.h>
#include <dash/dart/gaspi/dart_translation.h>
#include <dash/dart/gaspi/dart_mem.h>
#include <dash/dart/gaspi/dart_communication_priv.h>
#include <string.h>

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

    DART_CHECK_ERROR(dart_team_size(team, &teamsize));

    if(((teamsize * nbytes) + nbytes) > DART_GASPI_BUFFER_SIZE)
    {
        fprintf(stderr, "Memory for collective operation is exhausted\n");
        return DART_ERR_OTHER;
    }

    DART_CHECK_ERROR(gaspi_segment_ptr(dart_gaspi_buffer_id, &send_ptr));
    recv_ptr = (gaspi_pointer_t) ((char *) send_ptr + recv_offset);

    memcpy(send_ptr, sendbuf, nbytes);

    int result = dart_adapt_teamlist_convert(team, &index);
    if (result == -1)
    {
        return DART_ERR_INVAL;
    }

    DART_CHECK_ERROR(gaspi_allgather(dart_gaspi_buffer_id,
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
    DART_CHECK_ERROR(gaspi_barrier(gaspi_group_id, GASPI_BLOCK));

    return DART_OK;
}

//~ dart_ret_t dart_get(void *dest, dart_gptr_t gptr, size_t nbytes)
//~ {
    //~ gaspi_queue_id_t          queue;
    //~ gaspi_segment_id_t        remote_seg;
    //~ uint64_t                  remote_offset = gptr.addr_or_offs.offset;
    //~ int16_t                   seg_id        = gptr.segid;
    //~ uint16_t                  index         = gptr.flags;
    //~ dart_unit_t               remote_rank   = gptr.unitid;
    //~ struct dart_handle_struct handle;

    //~ handle.local_offset = dart_buddy_alloc(dart_transferpool, nbytes);
    //~ if(handle.local_offset == -1)
    //~ {
        //~ fprintf(stderr, "Out of bound: the global memory is exhausted");
        //~ return DART_ERR_OTHER;
    //~ }

    //~ DART_CHECK_ERROR(dart_get_minimal_queue(&queue));

    //~ handle.local_seg   = dart_transferpool_seg;
    //~ handle.dest_buffer = dest;
    //~ handle.queue       = queue;
    //~ handle.nbytes      = nbytes;

    //~ if(seg_id)
    //~ {
        //~ dart_unit_t rel_remote_rank;
        //~ DART_CHECK_ERROR(unit_g2l(index, remote_rank, &rel_remote_rank));
        //~ if(dart_adapt_transtable_get_gaspi_seg_id(seg_id, rel_remote_rank, &remote_seg) == -1)
        //~ {
            //~ fprintf(stderr, "Can't find given segment id in dart_get\n");
            //~ return DART_ERR_NOTFOUND;
        //~ }
        //~ if(dart_adapt_transtable_add_handle(seg_id, rel_remote_rank, &handle) == -1)
        //~ {
            //~ fprintf(stderr, "Can't save rma handle in dart_put\n");
            //~ return DART_ERR_OTHER;
        //~ }
    //~ }
    //~ else
    //~ {
        //~ remote_seg = dart_mempool_seg_localalloc;
        //~ DART_CHECK_ERROR(enqueue_handle(&(dart_non_collective_rma_request[remote_rank]), &handle));
    //~ }

    //~ DART_CHECK_ERROR(gaspi_read(handle.local_seg,
                                //~ handle.local_offset,
                                //~ remote_rank,
                                //~ remote_seg,
                                //~ remote_offset,
                                //~ nbytes,
                                //~ queue,
                                //~ GASPI_BLOCK));
    //~ return DART_OK;
//~ }
//~ /**
 //~ * TODO dart_put not implemented yet
 //~ */
//~ dart_ret_t dart_put(dart_gptr_t gptr, void *src, size_t nbytes)
//~ {
    //~ gaspi_queue_id_t          queue;
    //~ gaspi_segment_id_t        remote_seg;
    //~ struct dart_handle_struct handle;
    //~ gaspi_pointer_t           local_seg_ptr = NULL;
    //~ uint64_t                  remote_offset = gptr.addr_or_offs.offset;
    //~ int16_t                   seg_id        = gptr.segid;
    //~ uint16_t                  index         = gptr.flags;
    //~ dart_unit_t               remote_rank   = gptr.unitid;

    //~ handle.local_offset = dart_buddy_alloc(dart_transferpool, nbytes);
    //~ if(handle.local_offset == -1)
    //~ {
        //~ fprintf(stderr, "Out of bound: the global memory is exhausted");
        //~ return DART_ERR_OTHER;
    //~ }

    //~ DART_CHECK_ERROR(gaspi_segment_ptr(dart_transferpool_seg, &local_seg_ptr));
    //~ local_seg_ptr = (void *) ((char *) local_seg_ptr + handle.local_offset);

    //~ memcpy(local_seg_ptr, src, nbytes);

    //~ // TODO function must ensure that the returned queue has the size for a later notify
    //~ // to signal the availability of the data for the target rank
    //~ DART_CHECK_ERROR(dart_get_minimal_queue(&queue));

    //~ handle.local_seg = dart_transferpool_seg;
    //~ // to indicate the put operation in wait or test
    //~ handle.dest_buffer = NULL;
    //~ handle.queue       = queue;
    //~ handle.nbytes      = nbytes;

    //~ if(seg_id)
    //~ {
        //~ dart_unit_t rel_remote_rank;
        //~ DART_CHECK_ERROR(unit_g2l(index, remote_rank, &rel_remote_rank));
        //~ if(dart_adapt_transtable_get_gaspi_seg_id(seg_id, rel_remote_rank, &remote_seg) == -1)
        //~ {
            //~ fprintf(stderr, "Can't find given segment id in dart_put\n");
            //~ return DART_ERR_NOTFOUND;
        //~ }
        //~ if(dart_adapt_transtable_add_handle(seg_id, rel_remote_rank, &handle) == -1)
        //~ {
            //~ fprintf(stderr, "Can't save rma handle in dart_put\n");
            //~ return DART_ERR_OTHER;
        //~ }
    //~ }
    //~ else
    //~ {
        //~ remote_seg = dart_mempool_seg_localalloc;
        //~ DART_CHECK_ERROR(enqueue_handle(&(dart_non_collective_rma_request[remote_rank]), &handle));
    //~ }

    //~ DART_CHECK_ERROR(gaspi_write(dart_transferpool_seg,
                                 //~ handle.local_offset,
                                 //~ remote_rank,
                                 //~ remote_seg,
                                 //~ remote_offset,
                                 //~ nbytes,
                                 //~ queue,
                                 //~ GASPI_BLOCK));
    //~ return DART_OK;
//~ }

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

    DART_CHECK_ERROR(gaspi_read(dest_gaspi_seg,
                                dest_offset,
                                target_unit,
                                src_gaspi_seg,
                                src_offset,
                                nbytes,
                                queue,
                                GASPI_BLOCK));

    DART_CHECK_ERROR(gaspi_wait(queue, GASPI_BLOCK));

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

    DART_CHECK_ERROR(gaspi_read(dest_gaspi_seg,
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
/**
 * No guarantee for remote completion !
 * This function works only for get operations and
 * then you have the same semantic as dart_wait_local.
 *
 * Thats why this function is not implemented
 */
dart_ret_t dart_wait (dart_handle_t handle)
{
    return DART_ERR_OTHER;
}

dart_ret_t dart_waitall(dart_handle_t *handle, size_t n)
{
    return DART_ERR_OTHER;
}

dart_ret_t dart_wait_local (dart_handle_t handle)
{
    if (handle == NULL)
    {
        return DART_ERR_INVAL;
    }

    DART_CHECK_ERROR(gaspi_wait(handle->queue, GASPI_BLOCK));
    return DART_OK;
}

//~ dart_ret_t dart_waitall_local (dart_handle_t *handle, size_t n)
//~ {
    //~ if (handle == NULL)
    //~ {
        //~ return DART_ERR_INVAL;
    //~ }
    //~ for(size_t i = 0 ; i < n ; ++i)
    //~ {
        //~ DART_CHECK_ERROR(dart_wait_local(handle[i]));
    //~ }

    //~ return DART_OK;
//~ }

//~ dart_ret_t dart_test_local (dart_handle_t handle, int32_t* is_finished)
//~ {
    //~ if(handle == NULL)
    //~ {
        //~ return DART_ERR_INVAL;
    //~ }

    //~ gaspi_return_t retval = gaspi_wait(handle->queue, GASPI_TEST);
    //~ if(retval == GASPI_SUCCESS)
    //~ {
        //~ /**
        //~ * Indicates a get operation otherwise a put operation
        //~ * TODO: may be better a enum to identify the kind of operation
        //~ */
        //~ if(handle->dest_buffer != NULL)
        //~ {
            //~ gaspi_pointer_t seg_ptr;
            //~ DART_CHECK_ERROR(gaspi_segment_ptr(handle->local_seg, &seg_ptr));
            //~ seg_ptr = (gaspi_pointer_t) ((char *) seg_ptr + handle->local_offset);
            //~ memcpy(handle->dest_buffer, seg_ptr, handle->nbytes);
        //~ }

        //~ if (dart_buddy_free(dart_transferpool, handle->local_offset) == -1)
        //~ {
            //~ fprintf(stderr, "Free invalid local invalid offset = %llu\n", handle->local_offset);
            //~ return DART_ERR_INVAL;
        //~ }

        //~ *is_finished = 1;
        //~ return DART_OK;
    //~ }
    //~ else if(retval == GASPI_ERROR)
    //~ {
        //~ fprintf(stderr, "Error: gaspi_wait failed in dart_test_local\n");
        //~ return DART_ERR_OTHER;
    //~ }

    //~ *is_finished = 0;
    //~ return DART_OK;
//~ }

//~ dart_ret_t dart_testall_local (dart_handle_t *handle, size_t n, int32_t* is_finished)
//~ {
    //~ for(size_t i = 0 ; i < n ; ++i)
    //~ {
        //~ DART_CHECK_ERROR(dart_test_local(handle[i], is_finished));
        //~ if(! *is_finished )
        //~ {
            //~ return DART_OK;
        //~ }
    //~ }
    //~ return DART_OK;
//~ }

dart_ret_t dart_fence(dart_gptr_t gptr)
{
    return DART_ERR_OTHER;
}
/**
 * Guarantees local and remote completion of all outstanding puts and
 * gets on a certain memory allocation / window / segment for the
 * target unit specified in gptr. -> MPI_Win_flush()
 *
 * For GASPI its impossible to guarantee a remote completion for an
 * outstanding put operation without communicating with the target rank.
 *
 */
dart_ret_t dart_fence_all(dart_gptr_t gptr)
{
    return DART_ERR_OTHER;
}

dart_ret_t dart_flush_local(dart_gptr_t gptr)
{
    queue_t   * queue       = NULL;
    int16_t     seg_id      = gptr.segid;
    uint16_t    index       = gptr.flags;
    dart_unit_t remote_rank = gptr.unitid;

    if(seg_id)
    {
        dart_unit_t rel_remote_rank;
        DART_CHECK_ERROR(unit_g2l(index, remote_rank, &rel_remote_rank));
        if(dart_adapt_transtable_get_handle_queue(seg_id, rel_remote_rank, &queue) == -1)
        {
            fprintf(stderr, "Can't find queue for a given segment in dart_flush_local\n");
            return DART_ERR_OTHER;
        }
    }
    else
    {
        queue = &(dart_non_collective_rma_request[remote_rank]);
    }

    struct dart_handle_struct handle;
    size_t queue_size = queue->size;

    for(int i = 0 ; i < queue_size ; ++i)
    {
        DART_CHECK_ERROR(front_handle(queue, &handle));
        DART_CHECK_ERROR(dart_wait_local(&handle));
        DART_CHECK_ERROR(dequeue_handle(queue));
    }

    return DART_OK;
}
/**
 * TODO unefficient method cause of data structure(array)
 */
dart_ret_t dart_flush_local_all(dart_gptr_t gptr)
{
    queue_t * queue  = NULL;
    int16_t   seg_id = gptr.segid;
    uint16_t  index  = gptr.flags;

    if(seg_id)
    {
        size_t teamsize;
        DART_CHECK_ERROR(dart_team_size(index, &teamsize));
        for(dart_unit_t rel_rank = 0; rel_rank < teamsize ; ++rel_rank)
        {
            if(dart_adapt_transtable_get_handle_queue(seg_id, rel_rank, &queue) == -1)
            {
                fprintf(stderr, "Can't find queue for a given segment in dart_flush_local_all\n");
                return DART_ERR_OTHER;
            }

            struct dart_handle_struct handle;
            size_t queue_size = queue->size;

            for(int i = 0 ; i < queue_size ; ++i)
            {
                DART_CHECK_ERROR(front_handle(queue, &handle));
                DART_CHECK_ERROR(dart_wait_local(&handle));
                DART_CHECK_ERROR(dequeue_handle(queue));
            }
        }
    }
    else
    {
        for(int rank = 0 ; rank < dart_gaspi_rank_num ; ++rank)
        {
            struct dart_handle_struct handle;
            queue = &(dart_non_collective_rma_request[rank]);
            size_t queue_size = queue->size;

            for(int i = 0 ; i < queue_size ; ++i)
            {
                DART_CHECK_ERROR(front_handle(queue, &handle));
                DART_CHECK_ERROR(dart_wait_local(&handle));
                DART_CHECK_ERROR(dequeue_handle(queue));
            }
        }
    }

    return DART_OK;
}
