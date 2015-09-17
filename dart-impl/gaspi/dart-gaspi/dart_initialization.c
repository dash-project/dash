#include "dart_team_private.h"
#include "dart_gaspi.h"
#include "dart_translation.h"

gaspi_rank_t dart_gaspi_rank_num;
gaspi_rank_t dart_gaspi_rank;

gaspi_segment_id_t dart_gaspi_buffer_id = 0;
gaspi_pointer_t dart_gaspi_buffer_ptr;

dart_ret_t dart_init(int *argc, char ***argv)
{
    DART_CHECK_ERROR(gaspi_proc_init(GASPI_BLOCK));

    DART_CHECK_ERROR(gaspi_proc_rank(&dart_gaspi_rank));
    DART_CHECK_ERROR(gaspi_proc_num(&dart_gaspi_rank_num));

    /* Initialize the teamlist. */
    dart_adapt_teamlist_init();

    dart_next_availteamid = DART_TEAM_ALL;
    uint16_t index;
    int result = dart_adapt_teamlist_alloc(DART_TEAM_ALL, &index);
    if (result == -1)
    {
        return DART_ERR_OTHER;
    }
    dart_teams[index].id = GASPI_GROUP_ALL;
    dart_group_init(&(dart_teams[index].group));

    for(gaspi_rank_t r = 0; r < dart_gaspi_rank_num ; ++r)
    {
        dart_group_addmember(&(dart_teams[index].group), r);
    }

    dart_next_availteamid++;
    /*
     * private transfer segement per process
     */
    DART_CHECK_ERROR(gaspi_segment_alloc(dart_gaspi_buffer_id, DART_GASPI_BUFFER_SIZE, GASPI_MEM_INITIALIZED));
    DART_CHECK_ERROR(gaspi_segment_ptr(dart_gaspi_buffer_id, &dart_gaspi_buffer_ptr));
    /**
     * TODO use a list to manage free segement ids
     */
    seg_stack_init(&dart_free_coll_seg_ids, dart_coll_seg_count);
    seg_stack_fill(&dart_free_coll_seg_ids, dart_coll_seg_id_begin, dart_coll_seg_count);

    dart_gaspi_segment_cnt = dart_gaspi_buffer_id + 1;

    seg_stack_pop(&dart_free_coll_seg_ids, &(dart_seg_lists[index].seg_id));

    dart_seg_lists[index].state  = DART_GASPI_SEG_NULL;


    dart_gaspi_segment_cnt++;

    return DART_OK;
}

dart_ret_t dart_exit()
{
    DART_CHECK_ERROR(gaspi_segment_delete(dart_gaspi_buffer_id));
    /**
     * TODO use segment lists to free memory
     */
    for(gaspi_segment_id_t i = dart_gaspi_buffer_id + 1; i < dart_gaspi_segment_cnt; ++i)
    {
        if(dart_seg_lists[i].state == DART_GASPI_SEG_ALLOCATED)
        {
            DART_CHECK_ERROR(gaspi_segment_delete(i));
        }
    }

    uint16_t index;
    int result = dart_adapt_teamlist_convert(DART_TEAM_ALL, &index);
    if (result == -1)
    {
        return DART_ERR_INVAL;
    }

    dart_group_fini(&(dart_teams[index].group));

    dart_adapt_teamlist_destroy();
    dart_adapt_transtable_destroy();

    seg_stack_finish(&dart_free_coll_seg_ids);

    DART_CHECK_ERROR(gaspi_proc_term(GASPI_BLOCK));

    return DART_OK;
}
