#include <GASPI.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <dart_communication_priv.h>
#include <gaspi_utils.h>

gaspi_return_t test_bcast(gaspi_group_t group, int value, gaspi_rank_t root)
{
    gaspi_segment_id_t segid = 0;
    gaspi_pointer_t seg_ptr;
    gaspi_rank_t rank;

    DART_CHECK_GASPI_ERROR(gaspi_proc_rank(&rank));

    DART_CHECK_GASPI_ERROR(gaspi_segment_create(segid,
                                                1024,
                                                group,
                                                GASPI_BLOCK,
                                                GASPI_MEM_INITIALIZED));

    DART_CHECK_GASPI_ERROR(gaspi_segment_ptr(segid, &seg_ptr));

    if(rank == root)
    {
        *((int *) seg_ptr) = value;
    }

    DART_CHECK_GASPI_ERROR(gaspi_bcast(segid, 0UL, sizeof(int), root, group));

    gaspi_printf("bcast value %d\n", *((int *) seg_ptr));

    DART_CHECK_GASPI_ERROR(gaspi_segment_delete(segid));
    return GASPI_SUCCESS;
}

int main (int argc , char ** argv)
{
    gaspi_rank_t rank;
    gaspi_rank_t root;
    gaspi_rank_t rank_num;
    gaspi_group_t group;
    int value = 0;

    if(gaspi_proc_init( GASPI_BLOCK ) != GASPI_SUCCESS )
    {
        fprintf(stderr,"startup failed\n");
        exit(EXIT_FAILURE);
    }

    DART_CHECK_GASPI_ERROR(gaspi_proc_num(&rank_num));
    DART_CHECK_GASPI_ERROR(gaspi_proc_rank(&rank));
    /**
     * test case with odd and even groups
     */
    DART_CHECK_GASPI_ERROR(gaspi_group_create(&group));
    if(rank % 2 == 0)
    {
        for(gaspi_rank_t r = 0 ; r < rank_num ; ++r)
        {
            if(r % 2 == 0)
            {
                DART_CHECK_GASPI_ERROR(gaspi_group_add(group, r));
            }
        }
        root = 0;
        if(rank == root)
        {
            value = 1337;
        }
        DART_CHECK_GASPI_ERROR(gaspi_group_commit(group, GASPI_BLOCK));
    }
    else
    {
        for(gaspi_rank_t r = 0 ; r < rank_num ; ++r)
        {
            if(r % 2 != 0)
            {
                DART_CHECK_GASPI_ERROR(gaspi_group_add(group, r));
            }
        }
        root = 1;
        if(rank == root)
        {
            value = 42;
        }
        DART_CHECK_GASPI_ERROR(gaspi_group_commit(group, GASPI_BLOCK));
    }
    DART_CHECK_GASPI_ERROR(test_bcast(group, value, root));
    DART_CHECK_GASPI_ERROR(gaspi_group_delete(group));

    DART_CHECK_GASPI_ERROR(gaspi_barrier(GASPI_GROUP_ALL, GASPI_BLOCK));
    /**
     * test case with GASPI_GROUP_ALL
     */
    //~ DART_CHECK_GASPI_ERROR(test_bcast(GASPI_GROUP_ALL));

    DART_CHECK_GASPI_ERROR(gaspi_proc_term(GASPI_BLOCK));

    if(rank == 0)
    {
        gaspi_printf("test bcast: successful\n");
    }

    return 0;
}
