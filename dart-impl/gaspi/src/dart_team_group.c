#include <assert.h>
#include <string.h>
#include <dash/dart/if/dart.h>
#include <dash/dart/gaspi/dart_team_private.h>

#include <dash/dart/base/logging.h>

dart_ret_t dart_team_get_group (dart_team_t teamid, dart_group_t *group)
{
    uint16_t index;
    int result = dart_adapt_teamlist_convert (teamid, &index);
    if (result == -1){
        return DART_ERR_INVAL;
    }

    memcpy(group, &(dart_teams[index].group), sizeof(dart_group_t));

    return DART_OK;
}

/**
 * TODO what happens if rank isn't a part of a group ???
 * TODO use return values of gaspi operations
 */
dart_ret_t dart_team_create (dart_team_t teamid, const dart_group_t group, dart_team_t *newteam)
{
    // SplitAddition
    dart_global_unit_t myid;
    dart_myid(&myid);
    int32_t ismember_tmp; 
    int32_t ismember = 0;


    gaspi_group_t parent_gaspi_group;
    gaspi_group_t new_gaspi_group, temp_group;
    uint16_t      index, unique_id;
    size_t        size;
    dart_team_t   max_teamid = -1;
    DART_CHECK_ERROR(dart_size(&size));
    /*not used
    dart_global_unit_t   unit;
    DART_CHECK_ERROR(dart_myid(&unit));
    */
    /*
     * index to dart team
     */
    int result = dart_adapt_teamlist_convert(teamid, &unique_id);
    if (result == -1)
    {
        return DART_ERR_INVAL;
    }

    parent_gaspi_group = dart_teams[unique_id].id;

    dart_group_ismember(group, myid, &ismember_tmp);
    // gaspi_group_create needs to be called as often as groups are created
    for( size_t i = 0; i < (group->nsplit); ++i )
    {
        DART_CHECK_ERROR(gaspi_group_create(&temp_group));
        
        if(ismember_tmp && group->sibling == i)
        {
            fprintf(stderr, "I [%d] am a member of the group [%d, %d]\n", myid.id, group->l2g[0], group->l2g[1]);
            ismember = 1;
            new_gaspi_group = temp_group;
        }
    }

    /*if(0==ismember)
    {
        fprintf(stderr,"Id: %d exits with member status: %d\n", myid.id, ismember);
        return DART_OK;
    }*/
    /* Get the maximum next_availteamid among all the units belonging to the parent team specified by 'teamid'. */
    DART_CHECK_ERROR(gaspi_allreduce(&dart_next_availteamid,
                                     &max_teamid,
                                     1, GASPI_OP_MAX, GASPI_TYPE_INT, parent_gaspi_group, GASPI_BLOCK));

    dart_next_availteamid = max_teamid + 1;

    *newteam = DART_TEAM_NULL;

    if(ismember)
    {
        size_t gsize;
        dart_group_size(group, &gsize);
        dart_global_unit_t * group_members = malloc(sizeof(dart_unit_t) * gsize);
        assert(group_members);

        DART_CHECK_ERROR(dart_group_getmembers(group, group_members));
        for(size_t i = 0 ; i < gsize ; ++i)
        {
            DART_CHECK_ERROR(gaspi_group_add(new_gaspi_group, group_members[i].id));
        }

        // -> in gaspi a rank needs to be part of the commited group
        DART_CHECK_ERROR(gaspi_group_commit(new_gaspi_group, GASPI_BLOCK));

        result = dart_adapt_teamlist_alloc (max_teamid, &index);
        if (result == -1)
        {
            return DART_ERR_OTHER;
        }
        /* max_teamid is thought to be the new created team ID. */
        *newteam = max_teamid;
        fprintf(stderr,"New team created: %d by me: [%d] ismember: [%d]\n" , max_teamid, myid.id, ismember);
        dart_teams[index].id = new_gaspi_group;
        memcpy(&(dart_teams[index].group), &group, sizeof(dart_group_t));
        //dart_teams[index].group = group;
        free(group_members);
    }

    return DART_OK;
}
/**
 * TODO guarantee that all RMA-Opartions on the segment are finished
 *
 * local completion can realized with queues -> data structure to save used
 *
 * blocking team collective call
 */
dart_ret_t dart_team_destroy (dart_team_t *teamid)
{
    gaspi_group_t gaspi_group;
    uint16_t      index;

    int result = dart_adapt_teamlist_convert ((*teamid), &index);

    if (result == -1)
    {
        return DART_ERR_INVAL;
    }

    gaspi_group = dart_teams[index].id;

    dart_adapt_teamlist_recycle(index, result);

    DART_CHECK_ERROR(gaspi_group_delete(gaspi_group));

    return DART_OK;
}
/**
 * myid returns relative unit id of given team
 */
dart_ret_t dart_team_myid(dart_team_t teamid, dart_team_unit_t *myid)
{
    dart_global_unit_t global_myid;
    DART_CHECK_ERROR(dart_myid(&global_myid));
    DART_CHECK_ERROR(dart_team_unit_g2l(teamid, global_myid, myid));

    return DART_OK;
}
/**
 *  returns the size of a given team
 */
dart_ret_t dart_team_size(dart_team_t teamid, size_t *size)
{
    uint16_t index;

    if(teamid == DART_TEAM_ALL)
    {
        DART_CHECK_ERROR(dart_size(size));
        return DART_OK;
    }
    int result = dart_adapt_teamlist_convert (teamid, &index);

    if (result == -1)
    {
        return DART_ERR_INVAL;
    }

    *size = dart_teams[index].group->nmem;

    return DART_OK;
}

/* convert between local and global unit IDs

   local means the ID with respect to the specified team
   global means the ID with respect to DART_TEAM_ALL

   these calls are *not* collective calls on the specified teams
*/

dart_ret_t dart_team_unit_l2g(
   dart_team_t teamid,
   dart_team_unit_t localid,
   dart_global_unit_t *globalid)
{
    uint16_t index;

    int result = dart_adapt_teamlist_convert(teamid, &index);
    if (result == -1)
    {
        return DART_ERR_INVAL;
    }
    globalid->id = dart_teams[index].group->l2g[localid.id];

    return DART_OK;
}

dart_ret_t dart_team_unit_g2l(
    dart_team_t         teamid,
    dart_global_unit_t  globalid,
    dart_team_unit_t    *localid)
{
    uint16_t index;

    if(teamid == DART_TEAM_ALL)
    {
        localid->id = globalid.id;
        return DART_OK;
    }

    int result = dart_adapt_teamlist_convert(teamid, &index);
    if (result == -1)
    {
        return DART_ERR_INVAL;
    }
    localid->id = dart_teams[index].group->g2l[globalid.id];

    return DART_OK;
}

dart_ret_t dart_myid(dart_global_unit_t *myid)
{
    gaspi_rank_t r;
    DART_CHECK_ERROR(gaspi_proc_rank(&r));
    myid->id = r;
    return DART_OK;
}

dart_ret_t dart_size(size_t *size)
{
    gaspi_rank_t s;
    DART_CHECK_ERROR(gaspi_proc_num(&s));
    *size = s;
    return DART_OK;
}
