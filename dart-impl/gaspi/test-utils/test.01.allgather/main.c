#include <GASPI.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <gaspi_utils.h>

int my_cmp_ranks(const void * a, const void * b)
{
    const int c = *((gaspi_rank_t *) a);
    const int d = *((gaspi_rank_t *) b);
    return c - d;
}

gaspi_return_t test_allgather(gaspi_group_t group)
{
    gaspi_segment_id_t segid = 0;
    gaspi_size_t send_size = sizeof(gaspi_rank_t);
    gaspi_pointer_t seg_ptr;
    gaspi_pointer_t seg_recv_ptr;
    gaspi_number_t group_size;
    gaspi_return_t retval;
    gaspi_rank_t rank;

    DART_CHECK_ERROR_RET(retval, gaspi_proc_rank(&rank));
    DART_CHECK_ERROR_RET(retval, gaspi_group_size(group, &group_size));
    DART_CHECK_ERROR_RET(retval, gaspi_segment_create(segid,
                                                      send_size + (send_size * group_size),
                                                      group,
                                                      GASPI_BLOCK,
                                                      GASPI_MEM_INITIALIZED));

    DART_CHECK_ERROR_RET(retval, gaspi_segment_ptr(segid, &seg_ptr));

    *((gaspi_rank_t *) seg_ptr) = rank;

    gaspi_offset_t offset = send_size;

    DART_CHECK_ERROR_RET(retval, gaspi_allgather(segid, 0UL, segid, offset, send_size, group));

    seg_recv_ptr = seg_ptr + offset;
    gaspi_rank_t * recv_ptr = (gaspi_rank_t *) seg_recv_ptr;

    gaspi_rank_t * group_members = (gaspi_rank_t *) malloc(sizeof(gaspi_rank_t) * group_size);
    assert(group_members);

    DART_CHECK_ERROR_RET(retval, gaspi_group_ranks(group, group_members));

    qsort(group_members, group_size, sizeof(gaspi_rank_t), my_cmp_ranks);

    for(unsigned int i = 0 ; i < group_size ; ++i)
    {
        if(recv_ptr[i] != group_members[i])
        {
            gaspi_printf("Error wrong value in recv_buffer[%d]=%d != %d\n",i,recv_ptr[i],group_members[i]);
            free(group_members);
            DART_CHECK_ERROR_RET(retval, gaspi_segment_delete(segid));

            return GASPI_ERROR;
        }
    }

    free(group_members);
    DART_CHECK_ERROR_RET(retval, gaspi_segment_delete(segid));
    return retval;
}

int main (int argc , char ** argv)
{
    gaspi_rank_t rank;
    gaspi_rank_t rank_num;
    gaspi_group_t group;
    gaspi_return_t retval;

    if(gaspi_proc_init( GASPI_BLOCK ) != GASPI_SUCCESS )
    {
        fprintf(stderr,"startup failed\n");
        exit(EXIT_FAILURE);
    }

    DART_CHECK_ERROR_RET(retval, gaspi_proc_num(&rank_num));
    DART_CHECK_ERROR_RET(retval, gaspi_proc_rank(&rank));
    /**
     * test case with odd and even groups
     */
    DART_CHECK_ERROR_RET(retval, gaspi_group_create(&group));
    if(rank % 2 == 0)
    {
        for(gaspi_rank_t r = 0 ; r < rank_num ; ++r)
        {
            if(r % 2 == 0)
            {
                DART_CHECK_ERROR_RET(retval, gaspi_group_add(group, r));
            }
        }
        DART_CHECK_ERROR_RET(retval, gaspi_group_commit(group, GASPI_BLOCK));
    }
    else
    {
        for(gaspi_rank_t r = 0 ; r < rank_num ; ++r)
        {
            if(r % 2 != 0)
            {
                DART_CHECK_ERROR_RET(retval, gaspi_group_add(group, r));
            }
        }
        DART_CHECK_ERROR_RET(retval, gaspi_group_commit(group, GASPI_BLOCK));
    }
    DART_CHECK_ERROR_RET(retval, test_allgather(group));
    DART_CHECK_ERROR_RET(retval, gaspi_group_delete(group));

    DART_CHECK_ERROR_RET(retval, gaspi_barrier(GASPI_GROUP_ALL, GASPI_BLOCK));
    /**
     * test case with GASPI_GROUP_ALL
     */
    DART_CHECK_ERROR_RET(retval, test_allgather(GASPI_GROUP_ALL));

    DART_CHECK_ERROR_RET(retval, gaspi_proc_term(GASPI_BLOCK));

    if(retval == GASPI_SUCCESS && rank == 0)
    {
        gaspi_printf("test allgather: successful\n");
    }

    return retval;
}
