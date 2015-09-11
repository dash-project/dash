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
 * TODO:
 *  what happens if rank isn't a part of a group ???
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
    //~ dart_team_myid (teamid, &sub_unit);

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

    //~ MPI_Comm_create (comm, group -> mpi_group, &subcomm);

    *newteam = DART_TEAM_NULL;

    /* Get the maximum next_availteamid among all the units belonging to the parent team specified by 'teamid'. */
    DART_CHECK_ERROR(gaspi_allreduce(&dart_next_availteamid,
                                     &max_teamid,
                                     1, GASPI_OP_MAX, GASPI_TYPE_INT, parent_gaspi_group, GASPI_BLOCK));

    //~ MPI_Allreduce (&dart_next_availteamid, &max_teamid, 1, MPI_INT32_T, MPI_MAX, comm);
    dart_next_availteamid = max_teamid + 1;
    //~ if (subcomm != MPI_COMM_NULL)
    //~ {
    result = dart_adapt_teamlist_alloc (max_teamid, &index);
    if (result == -1)
    {
        return DART_ERR_OTHER;
    }
    /* max_teamid is thought to be the new created team ID. */
    *newteam = max_teamid;
    dart_teams[index].id = new_gaspi_group;
    memcpy(&(dart_teams[index].group), group, sizeof(dart_group_t));
    //~ group->gaspi_group_id = new_gaspi_group;
        //~ MPI_Win_create_dynamic (MPI_INFO_NULL, subcomm, &win);
        //~ dart_win_lists[index] = win;
    //~ }
    free(group_members);
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
