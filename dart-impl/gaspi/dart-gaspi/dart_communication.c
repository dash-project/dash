#include "dart_team_private.h"
#include "dart_translation.h"
#include "dart_gaspi.h"
#include "dart_mem.h"
#include "dart_communication.h"
#include "dart_communication_priv.h"
#include <string.h>

/********************** Only for testing *********************/
gaspi_queue_id_t dart_handle_get_queue(dart_handle_t handle)
{
    return handle->queue;
}
/*************************************************************/
/**
 * TODO dart_bcast not implemented yet
 */
dart_ret_t dart_bcast(void *buf, size_t nbytes, dart_unit_t root, dart_team_t team)
{
    return DART_ERR_OTHER;
}
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
/**
 * TODO dart_allgather not implemented yet
 */
dart_ret_t dart_allgather(void *sendbuf, void *recvbuf, size_t nbytes, dart_team_t team)
{
    return DART_ERR_OTHER;
}

dart_ret_t dart_barrier (dart_team_t teamid)
{
    gaspi_group_t gaspi_group_id;
    uint16_t index;
    int result = dart_adapt_teamlist_convert (teamid, &index);
    if (result == -1)
    {
        return DART_ERR_INVAL;
    }
    gaspi_group_id = dart_teams[index].id;
    DART_CHECK_ERROR(gaspi_barrier(gaspi_group_id, GASPI_BLOCK));
    return DART_OK;
}

int unit_g2l (uint16_t index, dart_unit_t abs_id, dart_unit_t *rel_id)
{
    if (index == 0)
    {
        *rel_id = abs_id;
    }
    else
    {
        *rel_id = dart_teams[index].group.g2l[abs_id];
    }
    return 0;
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
dart_ret_t dart_get_minimal_queue(gaspi_queue_id_t * qid)
{
    gaspi_return_t retval;
    gaspi_number_t qsize;
    gaspi_number_t queue_num_max;
    gaspi_number_t min_queue_size;
    gaspi_number_t queue_size_max;

    DART_CHECK_ERROR_RET(retval, gaspi_queue_size_max(&queue_size_max));
    DART_CHECK_ERROR_RET(retval, gaspi_queue_num(&queue_num_max));

    min_queue_size = queue_size_max;

    for(gaspi_queue_id_t q = 0 ; q < queue_num_max ; ++q)
    {
        DART_CHECK_ERROR_RET(retval, gaspi_queue_size(q, &qsize));
        if(qsize == 0)
        {
            *qid = q;
            return retval;
        }
        if(min_queue_size > qsize)
        {
            min_queue_size = qsize;
            *qid = q;
        }
    }
    /*
     * If there is no empty queue -> perform wait operation to empty one queue
     */
    if(min_queue_size == queue_size_max)
    {
        DART_CHECK_ERROR_RET(retval, gaspi_wait(*qid, GASPI_BLOCK));
    }

    return retval;
}

dart_ret_t dart_get(void *dest, dart_gptr_t gptr, size_t nbytes)
{
    gaspi_return_t retval = GASPI_SUCCESS;
    gaspi_queue_id_t queue;
    gaspi_segment_id_t remote_seg;
    uint64_t remote_offset = gptr.addr_or_offs.offset;
    int16_t seg_id = gptr.segid;
    uint16_t index  = gptr.flags;
    dart_unit_t remote_rank = gptr.unitid;

    struct dart_handle_struct handle;

    handle.local_offset = dart_buddy_alloc(dart_transferpool, nbytes);
    if(handle.local_offset == -1)
    {
        fprintf(stderr, "Out of bound: the global memory is exhausted");
        return DART_ERR_OTHER;
    }

    DART_CHECK_ERROR_RET(retval, dart_get_minimal_queue(&queue));

    handle.local_seg = dart_transferpool_seg;
    handle.dest_buffer = dest;
    handle.queue = queue;
    handle.nbytes = nbytes;

    if(seg_id)
    {
        dart_unit_t rel_remote_rank;
        unit_g2l(index, remote_rank, &rel_remote_rank);
        dart_adapt_transtable_get_gaspi_seg_id(seg_id, rel_remote_rank, &remote_seg);
        dart_adapt_transtable_add_handle(seg_id, rel_remote_rank, &handle);
    }
    else
    {
        remote_seg = dart_mempool_seg_localalloc;
        DART_CHECK_ERROR_RET(retval, enqueue_handle(&(dart_non_collective_rma_request[remote_rank]), &handle));
    }

    DART_CHECK_ERROR_RET(retval, gaspi_read(handle.local_seg,
                                            handle.local_offset,
                                            remote_rank,
                                            remote_seg,
                                            remote_offset,
                                            nbytes,
                                            queue,
                                            GASPI_BLOCK));

    return retval;
}
/**
 * TODO dart_put not implemented yet
 */
dart_ret_t dart_put(dart_gptr_t gptr, void *src, size_t nbytes)
{
    gaspi_return_t retval = GASPI_SUCCESS;
    gaspi_queue_id_t queue;
    gaspi_pointer_t local_seg_ptr = NULL;
    gaspi_segment_id_t remote_seg;
    uint64_t remote_offset = gptr.addr_or_offs.offset;
    int16_t seg_id = gptr.segid;
    uint16_t index  = gptr.flags;
    dart_unit_t remote_rank = gptr.unitid;
    struct dart_handle_struct handle;

    handle.local_offset = dart_buddy_alloc(dart_transferpool, nbytes);
    if(handle.local_offset == -1)
    {
        fprintf(stderr, "Out of bound: the global memory is exhausted");
        return DART_ERR_OTHER;
    }

    DART_CHECK_ERROR_RET(retval, gaspi_segment_ptr(dart_transferpool_seg, &local_seg_ptr));
    local_seg_ptr = (void *) ((char *) local_seg_ptr + handle.local_offset);

    memcpy(local_seg_ptr, src, nbytes);

    // TODO function must ensure that the returned queue has the size for a later notify
    // to signal the availability of the data for the target rank
    DART_CHECK_ERROR_RET(retval, dart_get_minimal_queue(&queue));

    handle.local_seg = dart_transferpool_seg;
    // to indicate the put operation in wait or test
    handle.dest_buffer = NULL;
    handle.queue = queue;
    handle.nbytes = nbytes;

    if(seg_id)
    {
        dart_unit_t rel_remote_rank;
        unit_g2l(index, remote_rank, &rel_remote_rank);
        dart_adapt_transtable_get_gaspi_seg_id(seg_id, rel_remote_rank, &remote_seg);
        dart_adapt_transtable_add_handle(seg_id, rel_remote_rank, &handle);
    }
    else
    {
        remote_seg = dart_mempool_seg_localalloc;
        DART_CHECK_ERROR_RET(retval, enqueue_handle(&(dart_non_collective_rma_request[remote_rank]), &handle));
    }

    DART_CHECK_ERROR_RET(retval, gaspi_write(dart_transferpool_seg,
                                             handle.local_offset,
                                             remote_rank,
                                             remote_seg,
                                             remote_offset,
                                             nbytes,
                                             queue,
                                             GASPI_BLOCK));
    return retval;
}

/*
 * access local elements ??
 * TODO use dart_transferpool
 */
dart_ret_t dart_get_blocking(void *dest, dart_gptr_t gptr, size_t nbytes)
{
    gaspi_return_t retval = GASPI_SUCCESS;
    gaspi_segment_id_t local_seg = dart_gaspi_buffer_id;
    gaspi_segment_id_t remote_seg;
    gaspi_queue_id_t queue;
    gaspi_pointer_t seg_ptr = NULL;

    uint64_t offset = gptr.addr_or_offs.offset;
    int16_t seg_id = gptr.segid;
    uint16_t index  = gptr.flags;
    dart_unit_t remote_rank = gptr.unitid;

    if(nbytes > DART_GASPI_BUFFER_SIZE)
    {
        DART_CHECK_ERROR_RET(retval, seg_stack_pop(&dart_free_coll_seg_ids, &local_seg));
        DART_CHECK_ERROR_RET(retval, gaspi_segment_alloc(local_seg, nbytes, GASPI_MEM_INITIALIZED));
    }

    DART_CHECK_ERROR_RET(retval, gaspi_segment_ptr(local_seg, &seg_ptr));

    if(seg_id)
    {
        dart_unit_t rel_remote_rank;
        unit_g2l(index, remote_rank, &rel_remote_rank);
        dart_adapt_transtable_get_gaspi_seg_id(seg_id, rel_remote_rank, &remote_seg);
    }
    else
    {
        remote_seg = dart_mempool_seg_localalloc;
    }

    DART_CHECK_ERROR_RET(retval, dart_get_minimal_queue(&queue));

    DART_CHECK_ERROR_RET(retval, gaspi_read(local_seg,
                                            0UL,
                                            remote_rank,
                                            remote_seg,
                                            offset,
                                            nbytes,
                                            queue,
                                            GASPI_BLOCK));

    DART_CHECK_ERROR_RET(retval, gaspi_wait(queue, GASPI_BLOCK));

    memcpy(dest, seg_ptr, nbytes);

    if(nbytes > DART_GASPI_BUFFER_SIZE)
    {
        DART_CHECK_ERROR_RET(retval, gaspi_segment_delete(local_seg));
    }

    return retval;
}

/**
 * its impossible to implement a one-sided blocking put operation
 * with GASPI
 */
dart_ret_t dart_put_blocking(dart_gptr_t ptr, void *src, size_t nbytes)
{
    fprintf(stderr, "dart-gaspi: No support for one-sided blocking put operations\n");
    return DART_ERR_OTHER;
}

dart_ret_t dart_get_handle(void *dest, dart_gptr_t gptr, size_t nbytes, dart_handle_t *handle)
{
    gaspi_return_t retval = GASPI_SUCCESS;
    gaspi_queue_id_t queue;
    gaspi_segment_id_t remote_seg;
    uint64_t remote_offset = gptr.addr_or_offs.offset;
    int16_t seg_id = gptr.segid;
    uint16_t index  = gptr.flags;
    dart_unit_t remote_rank = gptr.unitid;

    *handle = (dart_handle_t) malloc (sizeof (struct dart_handle_struct));
    assert(*handle);

    (*handle)->local_offset = dart_buddy_alloc(dart_transferpool, nbytes);
    if((*handle)->local_offset == -1)
    {
        fprintf(stderr, "Out of bound: the global memory is exhausted");
        return DART_ERR_OTHER;
    }

    DART_CHECK_ERROR_RET(retval, dart_get_minimal_queue(&queue));

    (*handle)->local_seg = dart_transferpool_seg;
    (*handle)->dest_buffer = dest;
    (*handle)->queue = queue;
    (*handle)->nbytes = nbytes;

    if(seg_id)
    {
        dart_unit_t rel_remote_rank;
        unit_g2l(index, remote_rank, &rel_remote_rank);
        dart_adapt_transtable_get_gaspi_seg_id(seg_id, rel_remote_rank, &remote_seg);
    }
    else
    {
        remote_seg = dart_mempool_seg_localalloc;
    }

    DART_CHECK_ERROR_RET(retval, gaspi_read((*handle)->local_seg,
                                            (*handle)->local_offset,
                                            remote_rank,
                                            remote_seg,
                                            remote_offset,
                                            nbytes,
                                            queue,
                                            GASPI_BLOCK));

    return retval;
}

dart_ret_t dart_put_handle(dart_gptr_t gptr, void *src, size_t nbytes, dart_handle_t *handle)
{
    gaspi_return_t retval = GASPI_SUCCESS;
    gaspi_queue_id_t queue;
    gaspi_pointer_t local_seg_ptr = NULL;
    gaspi_segment_id_t remote_seg;
    uint64_t remote_offset = gptr.addr_or_offs.offset;
    int16_t seg_id = gptr.segid;
    uint16_t index  = gptr.flags;
    dart_unit_t remote_rank = gptr.unitid;

    *handle = (dart_handle_t) malloc (sizeof (struct dart_handle_struct));
    assert(*handle);

    (*handle)->local_offset = dart_buddy_alloc(dart_transferpool, nbytes);
    if((*handle)->local_offset == -1)
    {
        fprintf(stderr, "Out of bound: the global memory is exhausted");
        return DART_ERR_OTHER;
    }

    DART_CHECK_ERROR_RET(retval, gaspi_segment_ptr(dart_transferpool_seg, &local_seg_ptr));
    local_seg_ptr = (void *) ((char *) local_seg_ptr + ((*handle)->local_offset));

    memcpy(local_seg_ptr, src, nbytes);

    // TODO function must ensure that the returned queue has the size for a later notify
    // to signal the availability of the data for the target rank
    DART_CHECK_ERROR_RET(retval, dart_get_minimal_queue(&queue));

    (*handle)->local_seg = dart_transferpool_seg;
    // to indicate the put operation in wait or test
    (*handle)->dest_buffer = NULL;
    (*handle)->queue = queue;
    (*handle)->nbytes = nbytes;

    if(seg_id)
    {
        dart_unit_t rel_remote_rank;
        unit_g2l(index, remote_rank, &rel_remote_rank);
        dart_adapt_transtable_get_gaspi_seg_id(seg_id, rel_remote_rank, &remote_seg);
    }
    else
    {
        remote_seg = dart_mempool_seg_localalloc;
    }

    DART_CHECK_ERROR_RET(retval, gaspi_write(dart_transferpool_seg,
                                             (*handle)->local_offset,
                                             remote_rank,
                                             remote_seg,
                                             remote_offset,
                                             nbytes,
                                             queue,
                                             GASPI_BLOCK));
    return retval;
}
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
    gaspi_return_t retval = GASPI_SUCCESS;
    if (handle == NULL)
    {
        return DART_OK;
    }

    DART_CHECK_ERROR_RET(retval, gaspi_wait(handle->queue, GASPI_BLOCK));
    /**
     * Indicates a get operation otherwise a put operation
     * TODO: may be better a enum to identify the kind of operation
     */
    if(handle->dest_buffer != NULL)
    {
        gaspi_pointer_t seg_ptr;
        DART_CHECK_ERROR_RET(retval, gaspi_segment_ptr(handle->local_seg, &seg_ptr));
        seg_ptr = (gaspi_pointer_t) ((char *) seg_ptr + handle->local_offset);
        memcpy(handle->dest_buffer, seg_ptr, handle->nbytes);
    }

    if (dart_buddy_free(dart_transferpool, handle->local_offset) == -1)
    {
        fprintf(stderr, "Free invalid local invalid offset = %llu\n", handle->local_offset);
        return DART_ERR_INVAL;
    }

    return retval;
}

dart_ret_t dart_waitall_local (dart_handle_t *handle, size_t n)
{
    dart_ret_t retval = DART_OK;

    for(size_t i = 0 ; i < n ; ++i)
    {
        DART_CHECK_ERROR_RET(retval, dart_wait_local(handle[i]));
    }

    return retval;
}

dart_ret_t dart_test_local (dart_handle_t handle, int32_t* is_finished)
{
    gaspi_return_t retval = GASPI_SUCCESS;
    if(handle == NULL)
    {
        DART_ERR_INVAL;
    }

    DART_CHECK_ERROR_RET(retval, gaspi_wait(handle->queue, GASPI_TEST));
    if(retval == GASPI_SUCCESS)
    {
        /**
        * Indicates a get operation otherwise a put operation
        * TODO: may be better a enum to identify the kind of operation
        */
        if(handle->dest_buffer != NULL)
        {
            gaspi_pointer_t seg_ptr;
            DART_CHECK_ERROR_RET(retval, gaspi_segment_ptr(handle->local_seg, &seg_ptr));
            seg_ptr = (gaspi_pointer_t) ((char *) seg_ptr + handle->local_offset);
            memcpy(handle->dest_buffer, seg_ptr, handle->nbytes);
        }

        if (dart_buddy_free(dart_transferpool, handle->local_offset) == -1)
        {
            fprintf(stderr, "Free invalid local invalid offset = %llu\n", handle->local_offset);
            return DART_ERR_INVAL;
        }

        *is_finished = 1;
        return retval;
    }

    *is_finished = 0;
    return DART_OK;
}

dart_ret_t dart_testall_local (dart_handle_t *handle, size_t n, int32_t* is_finished)
{
    dart_ret_t retval = DART_OK;

    for(size_t i = 0 ; i < n ; ++i)
    {
        DART_CHECK_ERROR_RET(retval, dart_test_local(handle[i], is_finished));
        if(! *is_finished )
        {
            return retval;
        }
    }

    return retval;
}

dart_ret_t dart_fence(dart_gptr_t gptr)
{
    return DART_ERR_OTHER;
}

dart_ret_t dart_fence_all(dart_gptr_t gptr)
{
    return DART_ERR_OTHER;
}

dart_ret_t dart_flush_local(dart_gptr_t gptr)
{
    dart_ret_t retval = DART_OK;
    int16_t seg_id = gptr.segid;
    uint16_t index  = gptr.flags;
    dart_unit_t remote_rank = gptr.unitid;
    queue_t * queue;
    if(seg_id)
    {
        dart_unit_t rel_remote_rank;
        unit_g2l(index, remote_rank, &rel_remote_rank);
        dart_adapt_transtable_get_handle_queue(seg_id, rel_remote_rank, &queue);
    }
    else
    {
        queue = &(dart_non_collective_rma_request[remote_rank]);
    }

    struct dart_handle_struct handle;
    size_t queue_size = queue->size;

    for(int i = 0 ; i < queue_size ; ++i)
    {
        DART_CHECK_ERROR_RET(retval, front_handle(queue, &handle));
        DART_CHECK_ERROR_RET(retval, dart_wait_local(&handle));
        DART_CHECK_ERROR_RET(retval, dequeue_handle(queue));
    }


    return retval;
}
