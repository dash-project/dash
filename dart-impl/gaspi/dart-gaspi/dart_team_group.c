#include <dart.h>
#include "dart_team_private.h"
#include <assert.h>
#include <string.h>

dart_ret_t dart_team_get_group (dart_team_t teamid, dart_group_t *group)
{
    uint16_t index;
    int result = dart_adapt_teamlist_convert (teamid, &index);
    if (result == -1)
    {
        return DART_ERR_INVAL;
    }

    memcpy(group, &(dart_teams[index].group), sizeof(dart_group_t));

    return DART_OK;
}

/**
 * TODO what happens if rank isn't a part of a group ???
 * TODO use return values of gaspi operations
 */
dart_ret_t dart_team_create (dart_team_t teamid, const dart_group_t* group, dart_team_t *newteam)
{
    gaspi_group_t parent_gaspi_group;
    gaspi_group_t new_gaspi_group;
    int root = -1;
    uint16_t index, unique_id;
    dart_unit_t rank;
    size_t size;
    dart_team_t max_teamid = -1;
    dart_unit_t sub_unit, unit;

    dart_myid(&unit);
    dart_size(&size);

    /*
     * index to dart team
     */
    int result = dart_adapt_teamlist_convert(teamid, &unique_id);
    if (result == -1)
    {
        return DART_ERR_INVAL;
    }

    parent_gaspi_group = dart_teams[unique_id].id;

    DART_CHECK_ERROR(gaspi_group_create(&new_gaspi_group));
    size_t gsize;
    dart_group_size(group, &gsize);
    dart_unit_t * group_members = malloc(sizeof(dart_unit_t) * gsize);
    assert(group_members);

    dart_group_getmembers(group, group_members);

    for(size_t i = 0; i < gsize ; ++i)
    {
        DART_CHECK_ERROR(gaspi_group_add(new_gaspi_group, group_members[i]));
    }

    DART_CHECK_ERROR(gaspi_group_commit(new_gaspi_group, GASPI_BLOCK));

    *newteam = DART_TEAM_NULL;

    /* Get the maximum next_availteamid among all the units belonging to the parent team specified by 'teamid'. */
    DART_CHECK_ERROR(gaspi_allreduce(&dart_next_availteamid,
                                     &max_teamid,
                                     1, GASPI_OP_MAX, GASPI_TYPE_INT, parent_gaspi_group, GASPI_BLOCK));

    dart_next_availteamid = max_teamid + 1;

    result = dart_adapt_teamlist_alloc (max_teamid, &index);
    if (result == -1)
    {
        return DART_ERR_OTHER;
    }
    /* max_teamid is thought to be the new created team ID. */
    *newteam = max_teamid;
    dart_teams[index].id = new_gaspi_group;
    memcpy(&(dart_teams[index].group), group, sizeof(dart_group_t));

    dart_seg_lists[index].seg_id = dart_gaspi_segment_cnt;
    dart_seg_lists[index].state  = DART_GASPI_SEG_NULL;
    dart_gaspi_segment_cnt++;

    free(group_members);
    return DART_OK;
}
/**
 * TODO guarantee that all RMA-Opartions on the segment are finished
 *
 * local completion can realized with queues -> data structure to save used
 *
 * blocking team collective call
 */
dart_ret_t dart_team_destroy (dart_team_t teamid)
{
    gaspi_group_t gaspi_group;
    gaspi_segment_id_t seg_id;
    uint16_t index;

    int result = dart_adapt_teamlist_convert (teamid, &index);

    if (result == -1)
    {
        return DART_ERR_INVAL;
    }
    gaspi_group = dart_teams[index].id;

    if(dart_seg_lists[index].state != DART_GASPI_SEG_NULL)
    {
        /**
        * TODO for segment list: release seg_id
        */
        seg_id = dart_seg_lists[index].seg_id;
        DART_CHECK_ERROR(gaspi_segment_delete(seg_id));
    }
    dart_adapt_teamlist_recycle (index, result);

    DART_CHECK_ERROR(gaspi_group_delete(gaspi_group));

    return DART_OK;
}

dart_ret_t dart_myid(dart_unit_t *myid)
{
    gaspi_rank_t r;
    DART_CHECK_ERROR(gaspi_proc_rank(&r));
    *myid = r;
    return DART_OK;
}


dart_ret_t dart_size(size_t *size)
{
    gaspi_rank_t s;
    DART_CHECK_ERROR(gaspi_proc_num(&s));
    *size = s;
    return DART_OK;
}
