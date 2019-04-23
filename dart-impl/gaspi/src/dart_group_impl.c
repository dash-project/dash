#include <stdio.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_team_group.h>
#include <dash/dart/gaspi/dart_group_impl.h>
#include <dash/dart/if/dart_locality.h>
#include <dash/dart/if/dart_initialization.h>

#include <dash/dart/base/logging.h>
#include <dash/dart/base/macro.h>
#include <dash/dart/base/assert.h>
#include <dash/dart/base/locality.h>

static dart_group_t allocate_group()
{
  return (dart_group_t) malloc(sizeof(struct dart_group_struct));
};

dart_ret_t dart_group_sizeof(size_t *size)
{
    *size = sizeof(dart_group_t);
    return DART_OK;
}

dart_ret_t dart_group_create(
  dart_group_t *group)
{
    (*group) = allocate_group();
    (*group)->nmem = 0;
    for (int i = 0; i < MAXSIZE_GROUP; ++i)
    {
        ((*group)->g2l)[i] = -1;
        ((*group)->l2g)[i] = -1;
    }
    return DART_OK;
}

dart_ret_t dart_group_destroy(dart_group_t *group)
{
    (*group)->nmem = 0;
    return DART_OK;
}

dart_ret_t dart_group_clone(const dart_group_t gin, dart_group_t *gout)
{
    (*gout)->nmem = gin->nmem;
    for (int i = 0; i < MAXSIZE_GROUP; ++i)
    {
        ((*gout)->g2l)[i] = (gin->g2l)[i];
        ((*gout)->l2g)[i] = (gin->l2g)[i];
    }
    return DART_OK;
}

// helper function not in the interface
void group_rebuild(dart_group_t* group)
{
    dart_group_t g = *group;
    int n = 0;
    // rebuild the data structure, based only on the g2l array
    // if (g2l[i]>=0) then the unit with global id i is part
    // of the group.
    for (int i = 0; i < MAXSIZE_GROUP; ++i)
    {
        if (g->g2l[i] >= 0)
        {
            g->l2g[n] = i;
            g->g2l[i] = n;
            n++;
        }
    }
    g->nmem = n;
}


dart_ret_t dart_group_union(const dart_group_t g1,
                            const dart_group_t g2,
                            dart_group_t *gout)
{
    for (int i = 0; i < MAXSIZE_GROUP; i++)
    {
        ((*gout)->g2l)[i] = -1;
        if ((g1->g2l)[i] >= 0 || (g2->g2l)[i] >= 0)
        {
            // just set g2l[i] to 1 to indicate that i is a member
            // group_rebuild then updates the group data structure
            ((*gout)->g2l)[i] = 1;
        }
    }

    group_rebuild(gout);
    return DART_OK;
}


dart_ret_t dart_group_intersect(const dart_group_t g1,
                                const dart_group_t g2,
                                dart_group_t *gout)
{
    for (int i = 0; i < MAXSIZE_GROUP; i++)
    {
        ((*gout)->g2l)[i] = -1;
        if ((g1->g2l)[i] >= 0 && (g2->g2l)[i] >= 0)
        {
            // set to 1 to indicate that i is a member
            // group_rebuild then updates the group data structure
            ((*gout)->g2l)[i] = 1;
        }
    }

    group_rebuild(gout);

    return DART_OK;
}

dart_ret_t dart_group_addmember(
  dart_group_t        g,
  dart_global_unit_t  unitid)
{
    g->g2l[unitid.id] = 1;
    group_rebuild(&g);

    return DART_OK;
}

dart_ret_t dart_group_delmember(
  dart_group_t        g,
  dart_global_unit_t  unitid)
{
    g->g2l[unitid.id] = -1;
    group_rebuild(&g);

    return DART_OK;
}

dart_ret_t dart_group_ismember(
                  const dart_group_t   g,
                  dart_global_unit_t   unitid,
                  int32_t             *ismember)
{
    (*ismember) = ((g->g2l)[unitid.id] >= 0);

    return DART_OK;
}

dart_ret_t dart_group_size(
  const dart_group_t   g,
  size_t             * size)
{
    (*size) = g->nmem;

    return DART_OK;
}

dart_ret_t dart_group_getmembers(
  const dart_group_t   g,
  dart_global_unit_t * unitids)
{
    int j = 0;
    for (int i = 0; i < MAXSIZE_GROUP; i++)
    {
        if( ((g->g2l)[i] >= 0) )
        {
            unitids[j].id=i;
            j++;
        }
    }
    return DART_OK;
}

/*dart_ret_t dart_group_split(const dart_group_t *g,
                            size_t nsplits,
                            dart_group_t **gsplit) */
dart_ret_t dart_group_split(
            const dart_group_t    g,
            size_t                n,
            size_t              * nout,
            dart_group_t        * gout)
{
    int j    = 0;
    int nmem = (g->nmem);
    int bdiv = nmem / n;
    int brem = nmem % n;
    int bsize;
    //(*gout)->nsplit = n;
    //actual number of groups in wich g is splitted
    //n <~~ number to split group at least in
    *nout = 0;

    for (int i = 0; i < n; ++i)
    {
        bsize = (i < brem) ? (bdiv + 1) : bdiv;
        dart_group_create(&gout[i]);

        for (int k = 0; (k < bsize) && (j < nmem); ++k, ++j)
        {
            (gout[i]->g2l)[g->l2g[j]] = 1;
        }

        group_rebuild(&gout[i]);

        ++(*nout);
    }

    return DART_OK;
}

//adapted from mpi implementation
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

   if (group == NULL) {
     DART_LOG_ERROR("Invalid group argument: %p", group);
     return DART_ERR_INVAL;
   }


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

   /* Splitting into more groups than domains not supported: */
   if (num_groups > (size_t)num_domains) {
     num_groups = num_domains;
     *nout      = num_groups;
   }
   if(num_groups == 0) {
     DART_LOG_ERROR("num_groups has to be greater than 0");
     return DART_ERR_OTHER;
   }

   /* create a group for every domain in the specified scope: */

   int total_domains_units           = 0;
   dart_domain_locality_t ** domains = malloc(num_domains * sizeof(*domains));
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

   if (num_groups == (size_t)num_domains) {
     /* one domain per group: */
     for (size_t g = 0; g < num_groups; ++g) {
       int                  group_num_units = domains[g]->num_units;
       dart_global_unit_t * unit_ids        = domains[g]->unit_ids;

       if (group_num_units <= 0) {
        DART_LOG_DEBUG("dart_group_locality_split: no units in group %zu", g);
        gout[g] = NULL;
       } else {
       // int * group_unit_ids = malloc(group_num_units * sizeof(int));

        gout[g] = allocate_group();
        dart_group_create(&gout[g]);
        for (int u = 0; u < group_num_units; ++u) {
           dart_group_addmember(gout[g], unit_ids[u]);
           DART_LOG_TRACE("dart_group_locality_split: group[%zu].units[%d] "
                          "global unit id: %d",
                          g, u, group_unit_ids[u]);
        }
       }
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
       int   num_group_domains   = max_group_domains;
       dart_global_unit_t * group_unit_ids = NULL;

       if ((g+1) * max_group_domains > (size_t)num_domains) {
        num_group_domains = (g * max_group_domains) - num_domains;
       }
       DART_LOG_TRACE("dart_group_locality_split: domains in group %zu: %d",
                      g, num_group_domains);
       int group_first_dom_idx = g * num_group_domains;
       int group_last_dom_idx  = group_first_dom_idx + num_group_domains;
       for (int d = group_first_dom_idx; d < group_last_dom_idx; ++d) {
        group_num_units += domains[d]->num_units;
       }
       if (group_num_units > 0) {
        group_unit_ids = malloc(sizeof(dart_global_unit_t) *
                                   group_num_units);
       } else {
        DART_LOG_DEBUG("dart_group_locality_split: no units in group %zu", g);
        gout[g] = NULL;
        continue;
       }
       int group_unit_idx = 0;
       for (int d = group_first_dom_idx; d < group_last_dom_idx; ++d) {
        for (int du = 0; du < domains[d]->num_units; ++du) {
           int u = group_unit_idx + du;
           group_unit_ids[u] = domains[d]->unit_ids[du];
           DART_LOG_TRACE("dart_group_locality_split: "
                          "group[%zu].unit_ids[%d] = domain[%d].unit_ids[%d]",
                          g, u, d, du);
        }
        group_unit_idx += domains[d]->num_units;
       }

       gout[g] = allocate_group();
       dart_group_create(&gout[g]);
       for (int u = 0; u < group_num_units; ++u) {
          dart_group_addmember(gout[g], group_unit_ids[u]);
          DART_LOG_TRACE("dart_group_locality_split: group[%zu].units[%d] "
                        "global unit id: %d",
                        g, u, group_unit_ids[u]);
      }

       free(group_unit_ids);
     }
  #endif
   }

   free(domains);
   free(domain_tags);

   DART_LOG_TRACE("dart_group_locality_split >");

   return DART_OK;
}
