#include "dart_team_private.h"
#include "dart_translation.h"
#include "dart_gaspi.h"

#include <string.h>

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
dart_ret_t get_minimal_queue(gaspi_queue_id_t * qid)
{
    gaspi_return_t retval;
    gaspi_number_t qsize;
    gaspi_number_t queue_num_max;
    gaspi_number_t min_queue_size;

    DART_CHECK_ERROR_RET(retval, gaspi_queue_size_max(&min_queue_size));
    DART_CHECK_ERROR_RET(retval, gaspi_queue_num(&queue_num_max));

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
    return retval;
}

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

    DART_CHECK_ERROR_RET(retval, get_minimal_queue(&queue));

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
