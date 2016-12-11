/**
 *  \file dart_team_group.c
 *
 *  Implementation of dart operations on team_group.
 */

#include <mpi.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_team_group.h>
#include <dash/dart/if/dart_initialization.h>
#include <dash/dart/if/dart_locality.h>

#include <dash/dart/base/logging.h>
#include <dash/dart/base/macro.h>
#include <dash/dart/base/assert.h>
#include <dash/dart/base/locality.h>

#include <dash/dart/mpi/dart_team_private.h>
#include <dash/dart/mpi/dart_group_priv.h>

#include <limits.h>

/* ======================================================================= *
 * Private Functions                                                        *
 * ======================================================================= */

static struct dart_group_struct* allocate_group()
{
  struct dart_group_struct* group = malloc(sizeof(struct dart_group_struct));
  return group;
}

dart_ret_t dart_group_create(
  dart_group_t *group)
{
  struct dart_group_struct* res = allocate_group();
  // Initialize the group as empty but not directly assign MPI_GROUP_EMPTY as it might lead to invalid free later
  MPI_Group g;
  MPI_Comm_group(MPI_COMM_WORLD, &g);
  MPI_Group_incl(g, 0, NULL, &res->mpi_group);
  *group = res;
  return DART_OK;
}

dart_ret_t dart_group_destroy(
  dart_group_t *group)
{
  struct dart_group_struct** g = group;
  if ((*g)->mpi_group != MPI_GROUP_NULL) {
    MPI_Group_free(&(*g)->mpi_group);
    (*g)->mpi_group = MPI_GROUP_NULL;
  }

  free(*g);
  *g = NULL;

  return DART_OK;
}

dart_ret_t dart_group_clone(
  const dart_group_t   gin,
  dart_group_t       * gout)
{
  //gout->mpi_group = gin->mpi_group;
  struct dart_group_struct* res = allocate_group();
  MPI_Group_excl(gin->mpi_group, 0, NULL, &res->mpi_group);
  *gout = res;
  return DART_OK;
}

#if 0
dart_ret_t dart_adapt_group_union(
  const dart_group_t *g1,
  const dart_group_t *g2,
  dart_group_t *gout)
{
  return MPI_Group_union(
    g1 -> mpi_group,
    g2 -> mpi_group,
    &(gout -> mpi_group));
}
#endif

dart_ret_t dart_group_union(
  const dart_group_t   g1,
  const dart_group_t   g2,
  dart_group_t       * gout)
{
  *gout = NULL;
  /* g1 and g2 are both ordered groups. */
  struct dart_group_struct* res = allocate_group();
  if (MPI_Group_union(
              g1->mpi_group,
              g2->mpi_group,
              &res->mpi_group) == MPI_SUCCESS)
  {
    int i, j, k, size_in, size_out;
    dart_unit_t *pre_unitidsout, *post_unitidsout;;

    MPI_Group group_all;
    MPI_Comm_group(MPI_COMM_WORLD, &group_all);
    MPI_Group_size(res->mpi_group, &size_out);
    if (size_out > 1) {
      MPI_Group_size(g1->mpi_group, &size_in);
      pre_unitidsout  = (dart_unit_t *)malloc(
                          size_out * sizeof (dart_unit_t));
      post_unitidsout = (dart_unit_t *)malloc(
                          size_out * sizeof (dart_unit_t));
      dart_group_getmembers (res, pre_unitidsout);

      /* Sort gout by the method of 'merge sort'. */
      i = k = 0;
      j = size_in;

      while ((i <= size_in - 1) && (j <= size_out - 1)) {
        post_unitidsout[k++] =
          (pre_unitidsout[i] <= pre_unitidsout[j])
          ? pre_unitidsout[i++]
          : pre_unitidsout[j++];
      }
      while (i <= size_in -1) {
        post_unitidsout[k++] = pre_unitidsout[i++];
      }
      while (j <= size_out -1) {
        post_unitidsout[k++] = pre_unitidsout[j++];
      }
      MPI_Group_free(&res->mpi_group);
      MPI_Group_incl(
        group_all,
        size_out,
        post_unitidsout,
        &(res->mpi_group));
      free (pre_unitidsout);
      free (post_unitidsout);
    }
    *gout = res;
    return DART_OK;
  }
  return DART_ERR_INVAL;
}

dart_ret_t dart_group_intersect(
  const dart_group_t   g1,
  const dart_group_t   g2,
  dart_group_t       * gout)
{
  struct dart_group_struct* res = allocate_group();
  *gout = NULL;
  if (MPI_Group_intersection(
           g1 -> mpi_group,
           g2 -> mpi_group,
           &(res->mpi_group)) != MPI_SUCCESS) {
    free(res);
    return DART_ERR_INVAL;
  }
  *gout = res;
  return DART_OK;
}

/**
 * <fuchst>   Does this function expect global or local unit ids (relative
 *            to a team)?
 */
dart_ret_t dart_group_addmember(
  dart_group_t g,
  dart_unit_t  unitid)
{
  int array[1];
  struct dart_group_struct group;
  dart_group_t  res;
  MPI_Group     group_all;
  /* Group_all comprises all the running units. */
  MPI_Comm_group(MPI_COMM_WORLD, &group_all);
  array[0]   = unitid;
  MPI_Group_incl(group_all, 1, array, &group.mpi_group);
  /* Make the new group being an ordered group. */
  if (dart_group_union(g, &group, &res) != DART_OK) {
    return DART_ERR_INVAL;
  }
  MPI_Group_free(&group.mpi_group);
  // swap the group from res to g and properly deallocate
  MPI_Group tmp  = g->mpi_group;
  g->mpi_group   = res->mpi_group;
  res->mpi_group = tmp;
  dart_group_destroy(&res);
  return DART_OK;
}

dart_ret_t dart_group_delmember(
  dart_group_t g,
  dart_unit_t  unitid)
{
  int array[1];
  MPI_Group newgroup, group_all, resgroup;
  MPI_Comm_group(MPI_COMM_WORLD, &group_all);
  array[0] = unitid;
  MPI_Group_incl(
    group_all,
    1,
    array,
    &newgroup);
  MPI_Group_difference(
    g->mpi_group,
    newgroup,
    &resgroup);
  MPI_Group_free(&newgroup);
  MPI_Group_free(&g->mpi_group);
  g->mpi_group = resgroup;
  return DART_OK;
}

dart_ret_t dart_group_size(
  const dart_group_t   g,
  size_t             * size)
{
  int s;
  MPI_Group_size(g->mpi_group, &s);
  *size = s;
  return DART_OK;
}

dart_ret_t dart_group_getmembers(
  const dart_group_t   g,
  dart_unit_t        * unitids)
{
  int size;
  int *array;
  MPI_Group group_all;
  MPI_Group_size(g->mpi_group, &size);
  MPI_Comm_group(MPI_COMM_WORLD, &group_all);
  array = (int*) malloc(sizeof (int) * size);
  for (int i = 0; i < size; i++) {
    array[i] = i;
  }
  MPI_Group_translate_ranks(
    g->mpi_group,
    size,
    array,
    group_all,
    unitids);
  free (array);
  return DART_OK;
}

dart_ret_t dart_group_split(
  const dart_group_t    g,
  size_t                n,
  size_t              * nout,
  dart_group_t        * gout)
{
  int size, length, ranges[1][3];

  MPI_Group_size(g->mpi_group, &size);

  if (n > INT_MAX) {
    DART_LOG_ERROR("dart_group_split: n > INT_MAX", n, *nout);
    return DART_ERR_INVAL;
  }

  *nout = size;
  if (size < (int)n) {
    DART_LOG_DEBUG("dart_group_split: requested:%d split:%d", n, *nout);
  }

  /* Ceiling division. */
  length = (size + (int)(n-1)) / ((int)(n));

  /* Note: split the group into chunks of subgroups. */
  for (int i = 0; i < (int)n; i++) {
    gout[i] = allocate_group();
    if (i * length < size) {
      ranges[0][0] = i * length;

      if (i * length + length <= size) {
        ranges[0][1] = i * length + length -1;
      } else {
        ranges[0][1] = size - 1;
      }
      ranges[0][2] = 1;

      MPI_Group_range_incl(
        g->mpi_group,
        1,
        ranges,
        &(gout[i]->mpi_group));
    } else {
      gout[i]->mpi_group = MPI_GROUP_NULL;
    }
  }
  return DART_OK;
}

dart_ret_t dart_group_locality_split(
  const dart_group_t        group,
  dart_domain_locality_t  * domain,
  dart_locality_scope_t     scope,
  size_t                    num_groups,
  size_t                  * nout,
  dart_group_t            * gout)
{
  DART_LOG_TRACE("dart_group_locality_split: split at scope %d", scope);

  dart_team_t team = domain->team;

  /* query domain tags of all domains in specified scope: */
  int     num_domains;
  char ** domain_tags;
  DART_ASSERT_RETURNS(
    dart_domain_scope_tags(
      domain,
      scope,
      &num_domains,
      &domain_tags),
    DART_OK);

  DART_LOG_TRACE("dart_group_locality_split: %d domains at scope %d",
                 num_domains, scope);

  /* create a group for every domain in the specified scope: */

  int total_domains_units           = 0;
  dart_domain_locality_t ** domains = malloc(
                                        num_domains *
                                        sizeof(dart_domain_locality_t *));
  for (int d = 0; d < num_domains; ++d) {
    DART_ASSERT_RETURNS(
      dart_domain_team_locality(team, domain_tags[d], &domains[d]),
      DART_OK);
    total_domains_units += domains[d]->num_units;

    DART_LOG_TRACE("dart_group_locality_split: domains[%d]: %s",
                   d, domain_tags[d]);
    DART_LOG_TRACE("dart_group_locality_split: - number of units: %d",
                   domains[d]->num_units);
  }
  DART_LOG_TRACE("dart_group_locality_split: total number of units: %d",
                 total_domains_units);

  /* Splitting into more groups than domains not supported: */
  if (num_groups > (size_t)num_domains) {
    num_groups = num_domains;
    *nout      = num_groups;
  }

  if (num_groups == (size_t)num_domains) {
    /* one domain per group: */
    for (size_t g = 0; g < num_groups; ++g) {
      int   group_num_units       = domains[g]->num_units;
      int * group_team_unit_ids   = domains[g]->unit_ids;

      /* convert relative unit ids from domain to global unit ids: */
      int * group_global_unit_ids = malloc(group_num_units * sizeof(int));
      for (int u = 0; u < group_num_units; ++u) {
        dart_team_unit_l2g(team,
                           group_team_unit_ids[u],
                           &group_global_unit_ids[u]);
        DART_LOG_TRACE("dart_group_locality_split: group[%d].units[%d] "
                       "global unit id: %d",
                       g, u, group_global_unit_ids[u]);
      }

      gout[g] = allocate_group();
      MPI_Group_incl(
        group->mpi_group,
        group_num_units,
        group_global_unit_ids,
        &(gout[g]->mpi_group));

      free(group_global_unit_ids);
    }
  } else if (num_groups < (size_t)num_domains) {
    /* Multiple domains per group. */
#if 0
    /* TODO */
    /* Combine domains into groups such that the number of units in the
     * groups is balanced, that is: the difference of the number of units of
     * the groups is minimal.
     *
     * Note that this corresponds to the NP-complete partition problem
     * (see https://en.wikipedia.org/wiki/Partition_problem), more specific
     * the multi-way partition- or multiprocessor schedulig problem
     * (see https://en.wikipedia.org/wiki/Multiprocessor_scheduling).
     *
     * Using the "Longest Processing Time" (LPT) algorithm here.
     */

    /* number of units assigned to single groups: */
    int * units_in_group = calloc(num_groups * sizeof(int), sizeof(int));

    /* sort domains by their number of units: */
    typedef struct {
      int domain_idx;
      int num_units;
    } domain_units_t_;

    domain_units_t_ * sorted_domains = malloc(num_domains *
                                              sizeof(domain_units_t_));

    /* minimum number of units in any group: */
    int min_group_units    = INT_MAX;
    /* index of group with fewest units: */
    int smallest_group_idx = 0;
    for (int d = 0; d < num_domains; ++d) {
      units_in_group[smallest_group_idx] = sorted_domains[d].num_units;
      min_group_units = INT_MAX;
      for (size_t g = 0; g < num_groups; ++g) {
        if (units_in_group[g] < min_group_units) {
          smallest_group_idx = g;
          min_group_units    = units_in_group[g];
        }
      }
    }
#else
    /* Preliminary implementation */
    int max_group_domains = (num_domains + (num_groups-1)) / num_groups;
    DART_LOG_TRACE("dart_group_locality_split: max. domains per group: %d",
                   max_group_domains);
    for (size_t g = 0; g < num_groups; ++g) {
      int   group_num_units     = 0;
      int * group_team_unit_ids = NULL;
      int   num_group_domains   = max_group_domains;
      if ((g+1) * max_group_domains > (size_t)num_domains) {
        num_group_domains = (g * max_group_domains) - num_domains;
      }
      DART_LOG_TRACE("dart_group_locality_split: domains in group %d: %d",
                     g, num_group_domains);
      int group_first_dom_idx = g * num_group_domains;
      int group_last_dom_idx  = group_first_dom_idx + num_group_domains;
      for (int d = group_first_dom_idx; d < group_last_dom_idx; ++d) {
        group_num_units += domains[d]->num_units;
      }
      group_team_unit_ids = malloc(sizeof(int) * group_num_units);
      int group_unit_idx = 0;
      for (int d = group_first_dom_idx; d < group_last_dom_idx; ++d) {
        for (int u = 0; u < domains[d]->num_units; ++u) {
          group_team_unit_ids[group_unit_idx + u] = domains[d]->unit_ids[u];
        }
        group_unit_idx += domains[d]->num_units;
      }

      /* convert relative unit ids from domain to global unit ids: */
      int * group_global_unit_ids = malloc(group_num_units * sizeof(int));
      for (int u = 0; u < group_num_units; ++u) {
        dart_team_unit_l2g(team,
                           group_team_unit_ids[u],
                           &group_global_unit_ids[u]);
        DART_LOG_TRACE("dart_group_locality_split: group[%d].units[%d] "
                       "global unit id: %d",
                       g, u, group_global_unit_ids[u]);
      }

      gout[g] = allocate_group();
      MPI_Group_incl(
        group->mpi_group,
        group_num_units,
        group_global_unit_ids,
        &(gout[g]->mpi_group));

      free(group_team_unit_ids);
      free(group_global_unit_ids);
    }
#endif
  }

  free(domains);
  free(domain_tags);

  DART_LOG_TRACE("dart_group_locality_split >");
  return DART_OK;
}


dart_ret_t dart_group_ismember(
  const dart_group_t   g,
  dart_unit_t          unitid,
  int32_t            * ismember)
{
  dart_unit_t id;
  dart_myid (&id);

  int           i, size;
  dart_unit_t*  ranks;

  MPI_Group_size(g->mpi_group, &size);
  ranks = (dart_unit_t *)malloc(size * sizeof(dart_unit_t));
  dart_group_getmembers (g, ranks);
  for (i = 0; i < size; i++) {
    if (ranks[i] == unitid) {
      break;
    }
  }
  *ismember = (i!=size);
  DART_LOG_DEBUG("dart_group_ismember : unit %2d: %s",
                 unitid, (*ismember) ? "yes" : "no");
  return DART_OK;
}

dart_ret_t dart_team_get_group(
  dart_team_t    teamid,
  dart_group_t * group)
{
  MPI_Comm comm;
  uint16_t index;
  *group = NULL;

  struct dart_group_struct* res = allocate_group();
  int result = dart_adapt_teamlist_convert(teamid, &index);
  if (result == -1) {
    free(res);
    return DART_ERR_INVAL;
  }
  comm = dart_team_data[index].comm;
  MPI_Comm_group(comm, &(res->mpi_group));

  *group = res;
  return DART_OK;
}

/**
 * Create a team as child of the specified team with units in
 * given group.
 *
 */
dart_ret_t dart_team_create(
  dart_team_t          teamid,
  const dart_group_t   group,
  dart_team_t        * newteam)
{
  MPI_Comm    comm;
  MPI_Comm    subcomm;
  MPI_Win     win;
  uint16_t    index,
              unique_id;
  size_t      size;
  dart_team_t max_teamid = -1;
  dart_unit_t sub_unit,
              unit;

  dart_team_data_t *team_data;

  dart_myid(&unit);
  dart_size(&size);
  dart_team_myid(teamid, &sub_unit);

  int result = dart_adapt_teamlist_convert(teamid, &unique_id);
  if (result == -1) {
    return DART_ERR_INVAL;
  }
  comm = dart_team_data[unique_id].comm;
  subcomm = MPI_COMM_NULL;

  MPI_Comm_create(comm, group->mpi_group, &subcomm);

  *newteam = DART_TEAM_NULL;

  /* Get the maximum next_availteamid among all the units belonging to
   * the parent team specified by 'teamid'. */
  MPI_Allreduce(
    &dart_next_availteamid,
    &max_teamid,
    1,
    MPI_INT32_T,
    MPI_MAX,
    comm);
  dart_next_availteamid = max_teamid + 1;

  if (subcomm != MPI_COMM_NULL) {
    int result = dart_adapt_teamlist_alloc(max_teamid, &index);
    if (result == -1) {
      return DART_ERR_OTHER;
    }
    /* max_teamid is thought to be the new created team ID. */
    *newteam = max_teamid;
    team_data = &dart_team_data[index];
    team_data->comm = subcomm;
    MPI_Win_create_dynamic(MPI_INFO_NULL, subcomm, &win);
    team_data->window = win;
  }

#if 0
  /* Another way of generating the available teamID for the newly crated team. */
  if (subcomm != MPI_COMM_NULL)
  {
    /* Get the maximum next_availteamid among all the units belonging to the
     * created sub-communicator. */
    MPI_Allreduce (&next_availteamid, &max_teamid, 1, MPI_INT, MPI_MAX, subcomm);
    int result = dart_adapt_teamlist_alloc (max_teamid, &index);

    if (result == -1)
    {
      return DART_ERR_OTHER;
    }

    *newteam = max_teamid;
    teams[index] = subcomm;
    MPI_Comm_rank (subcomm, &rank);

    if (rank == 0)
    {
      root = sub_unit;
      if (sub_unit != 0)
      {
        MPI_Send (&root, 1, MPI_INT, 0, 0, comm);
      }
    }

    next_availteamid = max_teamid + 1;
  }

  if (sub_unit == 0)
  {
    if (root == -1)
    {
      MPI_Recv (&root, 1, MPI_INT, MPI_ANY_SOURCE, 0, comm, MPI_STATUS_IGNORE);
    }
  }

  MPI_Bcast (&root, 1, MPI_INT, 0, comm);

  /* Broadcast the calculated max_teamid to all the units not belonging to the
   * sub-communicator. */
  MPI_Bcast (&max_teamid, 1, MPI_INT, root, comm);
  if (subcomm == MPI_COMM_NULL)
  {
    /* 'Next_availteamid' is changed iff it is smaller than 'max_teamid + 1' */
    if (max_teamid + 1 > next_availteamid)
    {
      next_availteamid = max_teamid + 1;
    }
  }
#endif

  if (subcomm != MPI_COMM_NULL) {
#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
    int    i;
    size_t n;

    MPI_Comm sharedmem_comm;
    MPI_Group sharedmem_group, group_all;
    MPI_Comm_split_type(
      subcomm,
      MPI_COMM_TYPE_SHARED,
      1,
      MPI_INFO_NULL,
      &sharedmem_comm);
    team_data->sharedmem_comm = sharedmem_comm;

    if (sharedmem_comm != MPI_COMM_NULL) {
      MPI_Comm_size(
        sharedmem_comm,
          &(team_data->sharedmem_nodesize));

    // dart_unit_mapping[index] = (int*)malloc (
    // dart_sharedmem_size[index] * sizeof (int));

      MPI_Comm_group(sharedmem_comm, &sharedmem_group);
      MPI_Comm_group(MPI_COMM_WORLD, &group_all);

      int * dart_unit_mapping = malloc(
          team_data->sharedmem_nodesize * sizeof(int));
      int * sharedmem_ranks = malloc(
          team_data->sharedmem_nodesize * sizeof(int));
      team_data->sharedmem_tab = malloc(size * sizeof(int));

      for (i = 0; i < team_data->sharedmem_nodesize; i++) {
        sharedmem_ranks[i] = i;
      }

    // MPI_Group_translate_ranks (sharedmem_group, dart_sharedmem_size[index],
    //     sharedmem_ranks, group_all, dart_unit_mapping[index]);
      MPI_Group_translate_ranks(
        sharedmem_group,
        team_data->sharedmem_nodesize,
        sharedmem_ranks,
        group_all,
        dart_unit_mapping);

      for (n = 0; n < size; n++) {
        team_data->sharedmem_tab[n] = -1;
      }
      for (i = 0; i < team_data->sharedmem_nodesize; i++) {
        team_data->sharedmem_tab[dart_unit_mapping[i]] = i;
      }
      free(sharedmem_ranks);
      free(dart_unit_mapping);
    }
#endif
    MPI_Win_lock_all(0, win);
    DART_LOG_DEBUG("TEAMCREATE - create team %d from parent team %d",
                   *newteam, teamid);
  }

  return DART_OK;
}

dart_ret_t dart_team_destroy(
  dart_team_t * teamid)
{
  MPI_Comm    comm;
  MPI_Win     win;
  uint16_t    index;
  dart_unit_t id;

  if (*teamid == DART_TEAM_NULL) {
    return DART_OK;
  }

  int result = dart_adapt_teamlist_convert(*teamid, &index);
  if (result == -1) {
    return DART_ERR_INVAL;
  }

  dart_team_data_t *team_data = &dart_team_data[index];

  comm = team_data->comm;

  dart_myid (&id);

  // free(dart_unit_mapping[index]);

  // MPI_Win_free (&(sharedmem_win_list[index]));
#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
  free(team_data->sharedmem_tab);
#endif
  win = team_data->window;
  MPI_Win_unlock_all(win);
  MPI_Win_free(&win);
  dart_adapt_teamlist_recycle(index, result);

  /* -- Release the communicator associated with teamid -- */
  MPI_Comm_free (&comm);

  DART_LOG_DEBUG("%2d: TEAMDESTROY  - destroy team %d", id, *teamid);

  *teamid = DART_TEAM_NULL;

  return DART_OK;
}


dart_ret_t dart_team_clone(dart_team_t team, dart_team_t *newteam)
{
  dart_group_t group;
  dart_team_get_group(team, &group);
  dart_ret_t ret = dart_team_create(team, group, newteam);
  dart_group_destroy(&group);
  return ret;
}

dart_ret_t dart_myid(dart_unit_t *unitid)
{
  if (dart_initialized()) {
    MPI_Comm_rank(MPI_COMM_WORLD, unitid);
  } else {
    *unitid = -1;
  }
  return DART_OK;
}

dart_ret_t dart_size(size_t *size)
{
  int s;
  MPI_Comm_size (MPI_COMM_WORLD, &s);
  (*size) = s;
  return DART_OK;
}

dart_ret_t dart_team_myid(
  dart_team_t   teamid,
  dart_unit_t * unitid)
{
  MPI_Comm comm;
  uint16_t index;
  int result = dart_adapt_teamlist_convert(teamid, &index);
  if (result == -1)
  {
    return DART_ERR_INVAL;
  }
  comm = dart_team_data[index].comm;
  MPI_Comm_rank(comm, unitid);

  return DART_OK;
}

dart_ret_t dart_team_size(
  dart_team_t   teamid,
  size_t      * size)
{
  MPI_Comm comm;
  uint16_t index;
  if (teamid == DART_TEAM_NULL) {
    return DART_ERR_INVAL;
  }
  int result = dart_adapt_teamlist_convert(teamid, &index);
  if (result == -1) {
    return DART_ERR_INVAL;
  }
  comm = dart_team_data[index].comm;
  // TODO: This should be a local operation.
  //       Team sizes could be cached and updated in dart_team_create.
  int s;
  MPI_Comm_size (comm, &s);
  (*size) = s;
  return DART_OK;
}

dart_ret_t dart_team_unit_l2g(
  dart_team_t   teamid,
  dart_unit_t   localid,
  dart_unit_t * globalid)
{
#if 0
  dart_unit_t *unitids;
  int size;
  int i = 0;
  dart_group_t group;
  dart_team_get_group (teamid, &group);
  MPI_Group_size (group.mpi_group, &size);
  if (localid >= size)
  {
    DART_LOG_ERROR ("Invalid localid input");
    return DART_ERR_INVAL;
  }
  unitids = (dart_unit_t*)malloc (sizeof(dart_unit_t) * size);
  dart_group_getmembers (&group, unitids);

  /* The unitids array is arranged in ascending order. */
  *globalid = unitids[localid];
//  printf ("globalid is %d\n", *globalid);
  return DART_OK;
#endif

  int size;
  dart_group_t group;

  dart_team_get_group (teamid, &group);
  MPI_Group_size (group->mpi_group, &size);

  if (localid >= size) {
    dart_group_destroy(&group);
    DART_LOG_ERROR ("Invalid localid input: %d", localid);
    return DART_ERR_INVAL;
  }
  if (teamid == DART_TEAM_ALL) {
    *globalid = localid;
  }
  else {
    MPI_Group group_all;
    MPI_Comm_group(MPI_COMM_WORLD, &group_all);
    MPI_Group_translate_ranks(
      group->mpi_group,
      1,
      &localid,
      group_all,
      globalid);
  }
  dart_group_destroy(&group);

  return DART_OK;
}

dart_ret_t dart_team_unit_g2l(
  dart_team_t   teamid,
  dart_unit_t   globalid,
  dart_unit_t * localid)
{
#if 0
  dart_unit_t *unitids;
  int size;
  int i;
  dart_group_t group;
  dart_team_get_group (teamid, &group);
  MPI_Group_size (group.mpi_group, &size);
  unitids = (dart_unit_t *)malloc (sizeof (dart_unit_t) * size);

  dart_group_getmembers (&group, unitids);


  for (i = 0; (i < size) && (unitids[i] < globalid); i++);

  if ((i == size) || (unitids[i] > globalid))
  {
    *localid = -1;
    return DART_OK;
  }

  *localid = i;
  return DART_OK;
#endif
  if(teamid == DART_TEAM_ALL) {
    *localid = globalid;
  }
  else {
    dart_group_t group;
    MPI_Group group_all;
    dart_team_get_group(teamid, &group);
    MPI_Comm_group(MPI_COMM_WORLD, &group_all);
    MPI_Group_translate_ranks(
      group_all,
      1,
      &globalid,
      group->mpi_group,
      localid);
    dart_group_destroy(&group);
  }
  return DART_OK;
}
