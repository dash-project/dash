/**
 * \file dash/dart/base/locality.c
 */

#define _GNU_SOURCE
#include <string.h>
#include <inttypes.h>
#include <unistd.h>
#include <stdio.h>
#include <sched.h>
#include <limits.h>

#include <dash/dart/base/locality.h>
#include <dash/dart/base/macro.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/base/assert.h>
#include <dash/dart/base/hwinfo.h>

#include <dash/dart/base/internal/host_topology.h>
#include <dash/dart/base/internal/unit_locality.h>
#include <dash/dart/base/internal/domain_locality.h>

#include <dash/dart/base/string.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_locality.h>
#include <dash/dart/if/dart_communication.h>

/* ====================================================================== *
 * Private Data                                                           *
 * ====================================================================== */

#define DART__BASE__LOCALITY__MAX_TEAM_DOMAINS 32

static dart_host_topology_t *
dart__base__locality__host_topology_[DART__BASE__LOCALITY__MAX_TEAM_DOMAINS];

static dart_unit_mapping_t *
dart__base__locality__unit_mapping_[DART__BASE__LOCALITY__MAX_TEAM_DOMAINS];

static dart_domain_locality_t *
dart__base__locality__global_domain_[DART__BASE__LOCALITY__MAX_TEAM_DOMAINS];

/* ====================================================================== *
 * Private Functions                                                      *
 * ====================================================================== */

static int cmpstr_(const void * p1, const void * p2)
{
  return strcmp(* (char * const *) p1, * (char * const *) p2);
}

dart_ret_t dart__base__locality__scope_domains_rec(
  const dart_domain_locality_t   * domain,
  dart_locality_scope_t            scope,
  int                            * num_domains_out,
  dart_domain_locality_t       *** domains_out);

dart_ret_t dart__base__locality__group_subdomains(
  dart_domain_locality_t         * domain,
  const char                    ** group_subdomain_tags,
  int                              num_group_subdomain_tags,
  char                           * group_domain_tag_out);

/* ====================================================================== *
 * Init / Finalize                                                        *
 * ====================================================================== */

dart_ret_t dart__base__locality__init()
{
  for (int td = 0; td < DART__BASE__LOCALITY__MAX_TEAM_DOMAINS; ++td) {
    dart__base__locality__global_domain_[td] = NULL;
    dart__base__locality__host_topology_[td] = NULL;
    dart__base__locality__unit_mapping_[td]  = NULL;
  }
  return dart__base__locality__create(DART_TEAM_ALL);
}

dart_ret_t dart__base__locality__finalize()
{
  for (dart_team_t t = 0; t < DART__BASE__LOCALITY__MAX_TEAM_DOMAINS; ++t) {
    dart__base__locality__delete(t);
  }

  dart_barrier(DART_TEAM_ALL);
  return DART_OK;
}

/* ====================================================================== *
 * Create / Delete                                                        *
 * ====================================================================== */

/**
 * Exchange and collect locality information of all units in the specified
 * team.
 *
 * The team's unit locality information is stored in private array
 * \c dart__base__locality__unit_mapping_[team] with a capacity for
 * \c DART__BASE__LOCALITY__MAX_TEAM_DOMAINS teams.
 *
 * Outline of the locality initialization procedure:
 *
 * 1. All units collect their local hardware locality information
 *    -> dart_hwinfo_t
 *
 * 2. All-to-all exchange of hardware locality data
 *    -> dart_unit_mapping_t { unit, team, hwinfo, domain }
 *
 * 3. Construct host topology from unit mapping data
 *    -> dart_host_topology_t
 *
 * 4. Initialize locality domain hierarchy from unit mapping data and
 *    host topology
 *    -> dart_domain_locality_t
 */
dart_ret_t dart__base__locality__create(
  dart_team_t team)
{
  DART_LOG_DEBUG("dart__base__locality__create() team(%d)", team);

  /*
   * TODO: Clarify if returning would be sufficient instead of failing
   *       assertion.
   */
  DART_ASSERT_MSG(
    (NULL == dart__base__locality__unit_mapping_[team]  &&
     NULL == dart__base__locality__global_domain_[team] &&
     NULL == dart__base__locality__host_topology_[team]),
    "dash__base__locality__create(): "
    "locality data of team is already initialized");

  dart_domain_locality_t * team_global_domain =
    malloc(sizeof(dart_domain_locality_t));
  dart__base__locality__global_domain_[team] =
    team_global_domain;

  /* Initialize the global domain as the root entry in the locality
   * hierarchy:
   */
  team_global_domain->scope          = DART_LOCALITY_SCOPE_GLOBAL;
  team_global_domain->level          = 0;
  team_global_domain->relative_index = 0;
  team_global_domain->team           = team;
  team_global_domain->parent         = NULL;
  team_global_domain->num_domains    = 0;
  team_global_domain->domains        = NULL;
  team_global_domain->num_units      = 0;
  team_global_domain->host[0]        = '\0';
  team_global_domain->domain_tag[0]  = '.';
  team_global_domain->domain_tag[1]  = '\0';

  size_t num_units = 0;
  DART_ASSERT_RETURNS(dart_team_size(team, &num_units), DART_OK);
  team_global_domain->num_units = num_units;
  team_global_domain->unit_ids  = malloc(num_units * sizeof(dart_unit_t));
  for (size_t u = 0; u < num_units; ++u) {
    dart_team_unit_t luid = { u };
    DART_ASSERT_RETURNS(
      dart_team_unit_l2g(team, luid, &team_global_domain->unit_ids[u]),
      DART_OK);
  }

  /* Exchange unit locality information between all units:
   */
  dart_unit_mapping_t * unit_mapping;
  DART_ASSERT_RETURNS(
    dart__base__unit_locality__create(team, &unit_mapping),
    DART_OK);
  dart__base__locality__unit_mapping_[team] = unit_mapping;

  /* Resolve host topology from the unit's host names:
   */
  dart_host_topology_t * topo;
  DART_ASSERT_RETURNS(
    dart__base__host_topology__create(unit_mapping, &topo),
    DART_OK);
  dart__base__locality__host_topology_[team] = topo;
  size_t num_nodes = topo->num_nodes;
  DART_LOG_TRACE("dart__base__locality__create: nodes: %ld", num_nodes);

  team_global_domain->num_nodes = num_nodes;

#ifdef DART_ENABLE_LOGGING
  for (int h = 0; h < topo->num_hosts; ++h) {
    dart_host_units_t  * node_units  = &topo->host_units[h];
    dart_host_domain_t * node_domain = &topo->host_domains[h];
    char * hostname = topo->host_names[h];
    DART_LOG_TRACE("dart__base__locality__create: "
                   "host %s: units:%d level:%d parent:%s", hostname,
                   node_units->num_units,
                   node_domain->level, node_domain->parent);
    for (int u = 0; u < node_units->num_units; ++u) {
      DART_LOG_TRACE("dart__base__locality__create: %s unit[%d]: %d",
                     hostname, u, node_units->units[u]);
    }
  }
#endif

  DART_LOG_DEBUG("dart__base__locality__create: "
                 "constructing domain hierarchy");
  /* Recursively create locality information of the global domain's
   * sub-domains:
   */
  DART_ASSERT_RETURNS(
    dart__base__locality__domain__create_subdomains(
      dart__base__locality__global_domain_[team],
      dart__base__locality__host_topology_[team],
      dart__base__locality__unit_mapping_[team]),
    DART_OK);

  DART_LOG_DEBUG("dart__base__locality__create >");
  return DART_OK;
}

dart_ret_t dart__base__locality__delete(
  dart_team_t team)
{
  dart_ret_t ret = DART_OK;

  DART_LOG_DEBUG("dart__base__locality__delete() team(%d)", team);

  if (NULL != dart__base__locality__global_domain_[team]) {
    ret = dart__base__locality__domain__destruct(
            dart__base__locality__global_domain_[team]);
    if (ret != DART_OK) {
      DART_LOG_ERROR("dart__base__locality__delete ! "
                     "dart__base__locality__domain_delete failed: %d", ret);
      return ret;
    }
    DART_LOG_DEBUG("dart__base__locality__delete: "
                   "free(dart__base__locality__global_domain_[%d])", team);
    free(dart__base__locality__global_domain_[team]);
    dart__base__locality__global_domain_[team] = NULL;
  }

  if (NULL != dart__base__locality__host_topology_[team]) {
    ret = dart__base__host_topology__destruct(
            dart__base__locality__host_topology_[team]);
    if (ret != DART_OK) {
      DART_LOG_ERROR("dart__base__locality__delete ! "
                     "dart__base__host_topology__destruct failed: %d", ret);
      return ret;
    }
    DART_LOG_DEBUG("dart__base__locality__delete: "
                   "free(dart__base__locality__host_topology_[%d])", team);
    free(dart__base__locality__host_topology_[team]);
    dart__base__locality__host_topology_[team] = NULL;
  }

  if (NULL != dart__base__locality__unit_mapping_[team]) {
    ret = dart__base__unit_locality__destruct(
            dart__base__locality__unit_mapping_[team]);
    if (ret != DART_OK) {
      DART_LOG_ERROR("dart__base__locality__delete ! "
                     "dart__base__unit_locality__destruct failed: %d", ret);
      return ret;
    }
    DART_LOG_DEBUG("dart__base__locality__delete: "
                   "free(dart__base__locality__unit_mapping_[%d])", team);
    dart__base__locality__unit_mapping_[team] = NULL;
  }

  DART_LOG_DEBUG("dart__base__locality__delete > team(%d)", team);
  return DART_OK;
}

/* ====================================================================== *
 * Domain Locality                                                        *
 * ====================================================================== */

dart_ret_t dart__base__locality__team_domain(
  dart_team_t                        team,
  dart_domain_locality_t          ** domain_out)
{
  DART_LOG_DEBUG("dart__base__locality__team_domain() team(%d)", team);
  dart_ret_t ret = DART_ERR_NOTFOUND;

  *domain_out = NULL;
  dart_domain_locality_t * domain =
    dart__base__locality__global_domain_[team];

  ret = dart__base__locality__domain(domain, ".", domain_out);

  DART_LOG_DEBUG("dart__base__locality__team_domain > "
                 "team(%d) -> domain(%p)", team, (void *)(*domain_out));
  return ret;
}

dart_ret_t dart__base__locality__domain(
  const dart_domain_locality_t     * domain_in,
  const char                       * domain_tag,
  dart_domain_locality_t          ** domain_out)
{
  return dart__base__locality__domain__child(
           domain_in, domain_tag, domain_out);
}


dart_ret_t dart__base__locality__scope_domains(
  const dart_domain_locality_t   * domain_in,
  dart_locality_scope_t            scope,
  int                            * num_domains_out,
  dart_domain_locality_t       *** domains_out)
{
  *num_domains_out = 0;
  *domains_out     = NULL;
  return dart__base__locality__scope_domains_rec(
           domain_in, scope, num_domains_out, domains_out);
}

dart_ret_t dart__base__locality__scope_domain_tags(
  const dart_domain_locality_t     * domain_in,
  dart_locality_scope_t              scope,
  int                              * num_domains_out,
  char                           *** domain_tags_out)
{
  *num_domains_out = 0;
  *domain_tags_out = NULL;

  dart_domain_locality_t ** dart_scope_domains;
  dart_ret_t ret = dart__base__locality__scope_domains(
      domain_in,
      scope,
      num_domains_out,
      &dart_scope_domains);

  if (ret != DART_OK) {
    free(dart_scope_domains);
    return ret;
  }

  *domain_tags_out = (char **)(malloc(sizeof(char *) * (*num_domains_out)));
  for (int sd = 0; sd < *num_domains_out; sd++) {
    (*domain_tags_out)[sd] = (char *)(malloc(
                               sizeof(char) *
                               (strlen(dart_scope_domains[sd]->domain_tag) + 1)));
    strcpy((*domain_tags_out)[sd],
           dart_scope_domains[sd]->domain_tag);
  }

  return DART_OK;
}

dart_ret_t dart__base__locality__domain_split_tags(
  const dart_domain_locality_t     * domain_in,
  dart_locality_scope_t              scope,
  int                                num_parts,
  int                             ** group_sizes_out,
  char                          **** group_domain_tags_out)
{
  /* For 4 domains in the specified scope, a split into 2 parts results
   * in a domain hierarchy like:
   *
   *   group_domain_tags[g][d] -> {
   *                                0: [ domain_0, domain_1 ],
   *                                1: [ domain_2, domain_3 ], ...
   *                              }
   *
   */

  DART_LOG_TRACE("dart__base__locality__domain_split_tags() "
                 "team(%d) domain(%s) scope(%d) parts(%d)",
                 domain_in->team, domain_in->domain_tag, scope, num_parts);

  /* Subdomains of global domain.
   * Domains of split parts, grouping domains at split scope. */
  char *** group_domain_tags = malloc(num_parts * sizeof(char **));
  int    * group_sizes       = malloc(num_parts * sizeof(int));

  /* Get total number and tags of domains in split scope: */
  int     num_domains;
  char ** domain_tags;
  DART_ASSERT_RETURNS(
    dart__base__locality__scope_domain_tags(
      domain_in, scope, &num_domains, &domain_tags),
    DART_OK);

  DART_LOG_TRACE("dart__base__locality__domain_split_tags: "
                 "number of domains in scope %d: %d", scope, num_domains);

  /* Group domains in split scope into specified number of parts: */
  int max_group_domains      = (num_domains + (num_parts-1)) / num_parts;
  int group_first_domain_idx = 0;

  DART_LOG_TRACE("dart__base__locality__domain_split_tags: "
                 "max. domains per group: %d", max_group_domains);
  /*
   * TODO: Preliminary implementation, should balance number of units in
   *       groups.
   */
  for (int g = 0; g < num_parts; ++g) {
    int num_group_subdomains = max_group_domains;
    if ((g+1) * max_group_domains > num_domains) {
      num_group_subdomains = num_domains - (g * max_group_domains);
    }
    DART_LOG_TRACE("dart__base__locality__domain_split_tags: "
                   "domains in group %d: %d", g, num_group_subdomains);

    group_sizes[g]       = num_group_subdomains;
    group_domain_tags[g] = malloc(sizeof(char *) * num_group_subdomains);

    for (int d_rel = 0; d_rel < num_group_subdomains; ++d_rel) {
      int d_abs   = group_first_domain_idx + d_rel;
      int tag_len = strlen(domain_tags[d_abs]);
      group_domain_tags[g][d_rel] = malloc(sizeof(char) * (tag_len + 1));
      strncpy(group_domain_tags[g][d_rel], domain_tags[d_abs],
              tag_len);
      group_domain_tags[g][d_rel][tag_len] = '\0';
    }
    /* Create domain of group: */
    group_first_domain_idx += num_group_subdomains;
  }

  *group_sizes_out       = group_sizes;
  *group_domain_tags_out = group_domain_tags;

  DART_LOG_TRACE("dart__base__locality__domain_split_tags >");
  return DART_OK;
}

dart_ret_t dart__base__locality__domain_group(
  dart_domain_locality_t           * domain,
  int                                group_size,
  const char                      ** group_subdomain_tags,
  char                             * group_domain_tag_out)
{
  DART_LOG_TRACE("dart__base__locality__domain_group() "
                 "domain_in: (%s: %d @ %d) group size: %d",
                 domain->domain_tag, domain->scope, domain->level,
                 group_size);
#ifdef DART_ENABLE_LOGGING
  for (int sd = 0; sd < group_size; sd++) {
    DART_LOG_TRACE("dart__base__locality__domain_group: "
                   "group_subdomain_tags[%d]: %s",
                   sd, group_subdomain_tags[sd]);
  }
#endif

  if (group_size < 1) {
    return DART_ERR_INVAL;
  }

  dart_ret_t ret = DART_OK;

  DART_LOG_TRACE("dart__base__locality__domain_group: "
                 "group size: %d", group_size);

  group_domain_tag_out[0] = '\0';

  /* The group's parent domain: */
  dart_domain_locality_t * group_parent_domain;
  ret = dart__base__locality__domain__parent(
          domain,
          group_subdomain_tags, group_size,
          &group_parent_domain);

  if (ret != DART_OK) {
    return ret;
  }

  DART_LOG_TRACE("dart__base__locality__domain_group: "
                 "group parent: %s",
                 group_parent_domain->domain_tag);

  /* Find parents of specified subdomains that are an immediate child node
   * of the input domain.
   */
  int immediate_subdomains_group = 1;
  int num_group_parent_domain_tag_parts =
        dart__base__strcnt(group_parent_domain->domain_tag, '.');
  for (int sd = 0; sd < group_size; sd++) {
    const char * group_domain_tag = group_subdomain_tags[sd];
    DART_LOG_TRACE("dart__base__locality__domain_group: "
                   "    group_subdomain_tags[%d]: %s",
                   sd, group_domain_tag);
    if (dart__base__strcnt(group_domain_tag, '.') !=
        num_group_parent_domain_tag_parts + 1) {
      immediate_subdomains_group = 0;
      break;
    }
  }
  if (immediate_subdomains_group) {
    DART_LOG_TRACE("dart__base__locality__domain_group: "
                   "group of immediate child domains");
    /* Subdomains in group are immediate child nodes of group parent
     * domain:
     */
    ret = dart__base__locality__group_subdomains(
            group_parent_domain,
            group_subdomain_tags, group_size,
            group_domain_tag_out);
    if (ret != DART_OK) {
      return ret;
    }
  } else {
    DART_LOG_TRACE("dart__base__locality__domain_group: "
                   "group of indirect child domains");

    /* Subdomains in group are in indirect child nodes of group parent
     * domain.
     * Find immediate child nodes that are parents of group subdomains.
     * Example:
     *
     *     parent:        .0
     *     group domains: { .0.1.2, .0.1.3, .0.2.0 }
     *
     *     --> { .0.1, .0.1, .0.2 }
     *
     *     --> groups:  { .0.1, .0.2 }
     */
    char ** immediate_subdomain_tags    = malloc(sizeof(char *) *
                                                 group_size);
    char *  group_parent_domain_tag     = group_parent_domain->domain_tag;
    int     group_parent_domain_tag_len = strlen(group_parent_domain_tag);
    DART_LOG_TRACE("dart__base__locality__domain_group: parent: %s",
                   group_parent_domain_tag);
    for (int sd = 0; sd < group_size; sd++) {
      /* Resolve relative index of subdomain: */
      immediate_subdomain_tags[sd] =
        calloc(DART_LOCALITY_DOMAIN_TAG_MAX_SIZE, sizeof(char));

      int group_subdomain_tag_len = strlen(group_subdomain_tags[sd]);

      if (group_subdomain_tag_len <= group_parent_domain_tag_len) {
        /* Indicates invalid parameters, usually caused by multiple units
         * mapped to the same domain to be grouped.
         */
        DART_LOG_ERROR("dart__base__locality__domain_group ! "
                       "group subdomain %s with invalid parent domain %s",
                       group_subdomain_tags[sd], group_parent_domain_tag);
        return DART_ERR_INVAL;
      }

      /* Position of second dot separator in tag of grouped domain after the
       * end of the parent domain tag, for example:
       *   parent:          .0.1
       *   grouped domain:  .0.1.4.0
       *   dot_pos: 6 ------------'
       */
      char * dot_pos = strchr(group_subdomain_tags[sd] +
                              group_parent_domain_tag_len + 1, '.');
      int immediate_subdomain_tag_len;
      if (dot_pos == NULL) {
        /* subdomain is immediate child of parent: */
        immediate_subdomain_tag_len = group_subdomain_tag_len;
      } else {
        /* subdomain is indirect child of parent: */
        immediate_subdomain_tag_len = dot_pos -
                                      group_subdomain_tags[sd];
      }
      strncpy(immediate_subdomain_tags[sd], group_subdomain_tags[sd],
              immediate_subdomain_tag_len);
      immediate_subdomain_tags[sd][immediate_subdomain_tag_len] = '\0';
    }
    int num_group_subdomains = dart__base__strsunique(
                                 immediate_subdomain_tags,
                                 group_size);
    DART_LOG_TRACE("dart__base__locality__domain_group: "
                   "num_group_subdomains: %d", num_group_subdomains);
#ifdef DART_ENABLE_LOGGING
    for (int gsd = 0; gsd < num_group_subdomains; gsd++) {
      DART_LOG_TRACE("dart__base__locality__domain_group: "
                     "group.subdomain[%d]: %s",
                     gsd, immediate_subdomain_tags[gsd]);
    }
#endif
    /*
     * Note: Required to append group domain at the end of the group
     *       parent's subdomain list to ensure that tags of domains not
     *       included in group remain valid.
     */
    group_parent_domain->domains =
      realloc(group_parent_domain->domains,
              sizeof(dart_domain_locality_t) *
                (group_parent_domain->num_domains + 1));
    dart_domain_locality_t * group_domain =
      group_parent_domain->domains + group_parent_domain->num_domains;

    dart__base__locality__domain__init(group_domain);

    group_domain->team           = group_parent_domain->team;
    group_domain->scope          = DART_LOCALITY_SCOPE_GROUP;
    group_domain->level          = group_parent_domain->level + 1;
    group_domain->parent         = group_parent_domain;
    group_domain->relative_index = group_parent_domain->num_domains;
    group_domain->num_nodes      = group_parent_domain->num_nodes;
    group_domain->num_units      = 0;
    group_domain->unit_ids       = NULL;
    group_domain->num_domains    = 0;
    group_domain->domains        = malloc(sizeof(dart_domain_locality_t) *
                                          num_group_subdomains);

    strncpy(group_domain->domain_tag, group_parent_domain_tag,
            group_parent_domain_tag_len);
    group_domain->domain_tag[group_parent_domain_tag_len] = '\0';

    /* TODO: Check if this implementation is correct.
             Incrementing an existing domain's relative index could result
             in naming collisions as the relative index of the subdomain
             can differ from the last place in their domain tag.
    */
    int group_domain_tag_len =
      sprintf(group_domain->domain_tag + group_parent_domain_tag_len,
              ".%d", group_domain->relative_index) +
      group_parent_domain_tag_len;
    group_domain->domain_tag[group_domain_tag_len] = '\0';

    strcpy(group_domain_tag_out, group_domain->domain_tag);

    /* Initialize group subdomains:
     */
    DART_LOG_TRACE("dart__base__locality__domain_group: "
                   "initialize %d subdomains of group (%s)",
                   num_group_subdomains, group_domain->domain_tag);

    for (int gsd = 0; gsd < num_group_subdomains; gsd++) {
      dart_domain_locality_t * group_subdomain_in;

      /* Copy
       *   domain.domains[domain_tag = group[g].immediate_subdomains[gsd]]
       * to
       *   group[g].domains[gsd]:
       */
      DART_LOG_TRACE("dart__base__locality__domain_group: "
                     "load domain.domains[domain_tag = "
                     "(group.immediate_subdomain_tags[%d] = %s])",
                     gsd, immediate_subdomain_tags[gsd]);

      ret = dart__base__locality__domain(
              domain, immediate_subdomain_tags[gsd],
              &group_subdomain_in);
      if (ret != DART_OK) { break; }

      DART_LOG_TRACE("dart__base__locality__domain_group: "
                     "copy domain.domains[domain_tag = %s] to "
                     "group.domains[%d]",
                     immediate_subdomain_tags[gsd], gsd);
      ret = dart__base__locality__domain__copy(
              group_subdomain_in,
              group_domain->domains + gsd);
      if (ret != DART_OK) { break; }

      group_domain->domains[gsd].parent = group_domain;
      group_domain->num_domains++;
      group_domain->num_units += group_domain[gsd].num_units;

    } /* for group_domain.domains */

    if (ret != DART_OK) { return ret; }

    group_domain->unit_ids = malloc(sizeof(dart_unit_t) *
                                    group_domain->num_units);

    /* Remove entries from group domain that are not part of the group:
     */
    DART_LOG_TRACE("dart__base__locality__domain_group: "
                   "select %d subdomains in group = %s",
                   group_size, group_domain->domain_tag);
    ret = dart__base__locality__domain_select_subdomains(
            group_domain,
            group_subdomain_tags,
            group_size);
    if (ret != DART_OK) { return ret; }

    DART_LOG_TRACE("dart__base__locality__domain_group: "
                   "update group (%s) after adding subdomains",
                   group_domain->domain_tag);
    ret = dart__base__locality__domain__update_subdomains(
            group_domain);
    if (ret != DART_OK) { return ret; }

    /* Remove grouped domains from parent's subdomains:
     */
    for (int sd = 0; sd < group_parent_domain->num_domains; sd++) {
      dart_domain_locality_t * group_parent_subdomain =
                                 group_parent_domain->domains + sd;
      /* Whether this sibling of the group domain contains subdomains
       * affected by the group:
       */
      int contains_grouped_domains = 0;
      for (int gd = 0; gd < group_size; ++gd) {
        if (strcmp(group_parent_subdomain->domain_tag,
                   immediate_subdomain_tags[gd]) == 0) {
          contains_grouped_domains = 1;
          break;
        }
      }
      if (contains_grouped_domains) {
        DART_LOG_TRACE("dart__base__locality__domain_group: "
                       "remove grouped domains from %s",
                       group_parent_subdomain->domain_tag);
        ret = dart__base__locality__domain_exclude_subdomains(
                group_parent_subdomain,
                group_subdomain_tags,
                group_size);
        if (ret != DART_OK) { break; }
      }
    }

    for (int sd = 0; sd < group_size; sd++) {
      DART_LOG_TRACE("dart__base__locality__domain_group: "
                     "free(immediate_subdomain_tags[%d])", sd);
      free(immediate_subdomain_tags[sd]);
    }
    DART_LOG_TRACE("dart__base__locality__domain_group: "
                   "free(immediate_subdomain_tags)");
    free(immediate_subdomain_tags);

    if (ret != DART_OK) { return ret; }

    group_parent_domain->num_domains++;
  }
  if (ret != DART_OK) { return ret; }

  DART_LOG_TRACE("dart__base__locality__domain_group >");
  return DART_OK;
}

/* ====================================================================== *
 * Unit Locality                                                          *
 * ====================================================================== */

dart_ret_t dart__base__locality__unit(
  dart_team_t                        team,
  dart_team_unit_t                   unit,
  dart_unit_locality_t            ** locality)
{
  DART_LOG_DEBUG("dart__base__locality__unit() team(%d) unit(%d)",
                 team, unit);
  *locality = NULL;

  dart_unit_locality_t * uloc;
  dart_ret_t ret = dart__base__unit_locality__at(
                     dart__base__locality__unit_mapping_[team], unit,
                     &uloc);
  if (ret != DART_OK) {
    DART_LOG_ERROR("dart_unit_locality: "
                   "dart__base__locality__unit(team:%d unit:%d) "
                   "failed (%d)", team, unit.id, ret);
    return ret;
  }
  *locality = uloc;

  DART_LOG_DEBUG("dart__base__locality__unit > team(%d) unit(%d)",
                 team, unit.id);
  return DART_OK;
}

/* ====================================================================== *
 * Private Function Definitions                                           *
 * ====================================================================== */

/**
 * Move subset of a domain's immediate child nodes into a group subdomain.
 */
dart_ret_t dart__base__locality__group_subdomains(
  dart_domain_locality_t           * domain,
  /** Domain tags of child nodes to move into group subdomain. */
  const char                      ** group_subdomain_tags,
  /** Number of child nodes to move into group subdomain. */
  int                                num_group_subdomain_tags,
  /** Domain tag of the created group subdomain. */
  char                             * group_domain_tag_out)
{
  DART_LOG_TRACE("dart__base__locality__group_subdomains() "
                 "group parent domain: %s num domains: %d "
                 "num_group_subdomain_tags: %d",
                 domain->domain_tag, domain->num_domains,
                 num_group_subdomain_tags);

  group_domain_tag_out[0] = '\0';

  if (domain->num_domains < 1) {
    DART_LOG_ERROR("dart__base__locality__group_subdomains ! "
                   "no subdomains, cannot create group");
    return DART_ERR_NOTFOUND;
  }

  int num_grouped         = num_group_subdomain_tags;
  int num_ungrouped       = domain->num_domains - num_grouped;
  int num_subdomains_new  = num_ungrouped + 1;

  /* The domain tag of the group to be added must be a successor of the last
   * subdomain's (the group domain's last sibling) tag to avoid collisions.
   * Relative index of last subdomain can differ from its last domain tag
   * segment, so we need to read and increment the suffix of its domain tag
   * to obtain the group's domain tag. */
#if 0
  int domain_last_rel_idx = domain->domains[domain->num_domains - 1]
                              .relative_index;
#endif
  char * domain_last_tag_suffix_pos =
           strrchr(domain->domains[domain->num_domains - 1].domain_tag, '.');
  int    domain_last_tag_suffix     = atoi(domain_last_tag_suffix_pos + 1);

  /* Child nodes are ordered by domain tag.
   * Create sorted copy of subdomain tags to partition child nodes in a
   * single pass:
   */
  char ** group_subdomain_tags_sorted =
    malloc(num_group_subdomain_tags * sizeof(char *));
  for (int sdt = 0; sdt < num_group_subdomain_tags; ++sdt) {
    group_subdomain_tags_sorted[sdt] =
      malloc((strlen(group_subdomain_tags[sdt]) + 1) * sizeof(char));
    strcpy(group_subdomain_tags_sorted[sdt], group_subdomain_tags[sdt]);
  }
  qsort(group_subdomain_tags_sorted, num_group_subdomain_tags,
        sizeof(char*), cmpstr_);

  int num_existing_domain_groups = 0;
  for (int sd = 0; sd < domain->num_domains; sd++) {
    if (domain->domains[sd].scope == DART_LOCALITY_SCOPE_GROUP) {
      num_existing_domain_groups++;
    }
  }
  num_ungrouped -= num_existing_domain_groups;

  /* Partition child nodes of domain into grouped and ungrouped
   * subdomains:
   */
  dart_domain_locality_t * ungrouped_domains =
    malloc(sizeof(dart_domain_locality_t) * num_ungrouped);
  dart_domain_locality_t * group_domains =
    malloc(sizeof(dart_domain_locality_t) * num_existing_domain_groups);
  dart_domain_locality_t * grouped_domains =
    malloc(sizeof(dart_domain_locality_t) * num_grouped);

  /* Copy child nodes into partitions:
   */
  int sdt                  = 0;
  int group_idx            = 0;
  int grouped_idx          = 0;
  int ungrouped_idx        = 0;
  int group_domain_rel_idx = num_ungrouped + num_existing_domain_groups;

  for (int sd = 0; sd < domain->num_domains; sd++) {
    dart_domain_locality_t * subdom        = &domain->domains[sd];
    dart_domain_locality_t * domain_copy;

    if (subdom->scope == DART_LOCALITY_SCOPE_GROUP) {
      domain_copy = &group_domains[group_idx];
      group_idx++;
    }
    else if (sdt < num_group_subdomain_tags &&
             strcmp(subdom->domain_tag, group_subdomain_tags_sorted[sdt])
             == 0) {
      domain_copy = &grouped_domains[grouped_idx];
      grouped_idx++;
      sdt++;
    } else {
      domain_copy = &ungrouped_domains[ungrouped_idx];
      ungrouped_idx++;
    }
    memcpy(domain_copy, subdom, sizeof(dart_domain_locality_t));
  }

  for (int sdt = 0; sdt < num_group_subdomain_tags; ++sdt) {
    DART_LOG_TRACE("dart__base__locality__domain_group: "
                   "free(group_subdomain_tags[%d])", sdt);
    free(group_subdomain_tags_sorted[sdt]);
  }
  DART_LOG_TRACE("dart__base__locality__domain_group: "
                 "free(group_subdomain_tags_sorted)");
  free(group_subdomain_tags_sorted);

  /* Check that all subdomains have been found: */
  if (sdt != num_group_subdomain_tags) {
    DART_LOG_ERROR("dart__base__locality__group_subdomains ! "
                   "only found %d of %d requested subdomains",
                   sdt, num_group_subdomain_tags);
    return DART_ERR_NOTFOUND;
  }

  /* Append group domain to subdomains:
   */
  domain->domains =
    realloc(domain->domains,
            sizeof(dart_domain_locality_t) * num_subdomains_new);
  DART_ASSERT(domain->domains != NULL);

  domain->num_domains = num_subdomains_new;

  /* Initialize group domain and set it as the input domain's last child
   * node:
   */
  dart_domain_locality_t * group_domain =
    &domain->domains[group_domain_rel_idx];

  dart__base__locality__domain__init(group_domain);

  /* TODO: hwinfo of group domain is not initialized.
   */
  group_domain->parent         = domain;
  group_domain->relative_index = group_domain_rel_idx;
  group_domain->level          = domain->level + 1;
  group_domain->scope          = DART_LOCALITY_SCOPE_GROUP;
  group_domain->num_domains    = num_grouped;
  group_domain->domains        = malloc(sizeof(dart_domain_locality_t) *
                                        num_grouped);

  int tag_len      = sprintf(group_domain->domain_tag, "%s",
                             domain->domain_tag);
  int tag_last_idx = domain_last_tag_suffix + 1;
  sprintf(group_domain->domain_tag + tag_len, ".%d", tag_last_idx);
  DART_LOG_TRACE("dart__base__locality__group_subdomains: "
                 "group_domain.tag: %s relative index: %d grouped: %d "
                 "ungrouped: %d",
                 group_domain->domain_tag, group_domain->relative_index,
                 num_grouped, num_ungrouped);

  strcpy(group_domain_tag_out, group_domain->domain_tag);

  /*
   * Set grouped partition of subdomains as child nodes of group domain:
   */
  group_domain->num_units = 0;
  group_domain->num_nodes = 0;
  for (int gd = 0; gd < num_grouped; gd++) {
    memcpy(&group_domain->domains[gd], &grouped_domains[gd],
           sizeof(dart_domain_locality_t));
  }
  dart__base__locality__domain__update_subdomains(
    group_domain);

  /*
   * Collect unit ids of group domain:
   */
  group_domain->unit_ids = malloc(sizeof(dart_unit_t) *
                                  group_domain->num_units);
  int group_domain_unit_idx = 0;
  for (int gd = 0; gd < num_grouped; gd++) {
    dart_domain_locality_t * group_subdomain = group_domain->domains + gd;
    memcpy(group_domain->unit_ids + group_domain_unit_idx,
           group_subdomain->unit_ids,
           sizeof(dart_unit_t) * group_subdomain->num_units);
    group_domain_unit_idx += group_subdomain->num_units;
  }

  for (int sd = 0; sd < num_ungrouped; sd++) {
    DART_LOG_TRACE(
      "dart__base__locality__group_subdomains: ==> domains[%d] u: %s",
      sd, domain->domains[sd].domain_tag);
    memcpy(&domain->domains[sd],
           &ungrouped_domains[sd],
           sizeof(dart_domain_locality_t));

    /* Pointers are invalidated by realloc, update parent pointers of
     * subdomains: */
    domain->domains[sd].parent         = domain;
    domain->domains[sd].relative_index = sd;
  }

  for (int g = 0; g < num_existing_domain_groups; g++) {
    int abs_g = g + num_ungrouped;
    DART_LOG_TRACE(
      "dart__base__locality__group_subdomains: ==> domains[%d] g: %s",
      abs_g, domain->domains[abs_g].domain_tag);
    memcpy(&domain->domains[abs_g], &group_domains[g],
           sizeof(dart_domain_locality_t));

    /* Pointers are invalidated by realloc, update parent pointers of
     * subdomains: */
    domain->domains[abs_g].parent         = domain;
    domain->domains[abs_g].relative_index = abs_g;
  }
  DART_LOG_TRACE(
    "dart__base__locality__group_subdomains: ==> domains[%d] *: %s",
    group_domain->relative_index, group_domain->domain_tag);

#ifdef DART_ENABLE_LOGGING
  int g_idx = 0;
  for (int sd = 0; sd < domain->num_domains; sd++) {
    dart_domain_locality_t * subdom = domain->domains + sd;
    DART_LOG_TRACE(
      "dart__base__locality__group_subdomains: --> domains[%d:%d]: "
      "tag:'%s' scope:%d subdomains:%d ADDR[%p]",
      sd, subdom->relative_index, subdom->domain_tag,
      subdom->scope, subdom->num_domains, (void *)subdom);
    if (subdom->scope == DART_LOCALITY_SCOPE_GROUP) {
      for (int gsd = 0; gsd < subdom->num_domains; gsd++) {
        dart_domain_locality_t * group_subdom = &(subdom->domains[gsd]);
        dart__unused(group_subdom);
        DART_LOG_TRACE(
          "dart__base__locality__group_subdomains: -->   groups[%d:%d]."
          "domains[%d]: tag:'%s' scope: %d subdomains:%d",
          g_idx, group_subdom->relative_index, gsd,
          group_subdom->domain_tag,
          group_subdom->scope, group_subdom->num_domains);
      }
      g_idx++;
    }
  }
#endif

  DART_LOG_TRACE("dart__base__locality__domain_group: "
                 "free(grouped_domains)");
  free(grouped_domains);

  DART_LOG_TRACE("dart__base__locality__domain_group: "
                 "free(ungrouped_domains)");
  free(ungrouped_domains);

  DART_LOG_TRACE("dart__base__locality__group_subdomains >");
  return DART_OK;
}

dart_ret_t dart__base__locality__scope_domains_rec(
  const dart_domain_locality_t     * domain,
  dart_locality_scope_t              scope,
  int                              * num_domains_out,
  dart_domain_locality_t         *** domains_out)
{
  dart_ret_t ret;
  DART_LOG_TRACE("dart__base__locality__scope_domains() level %d",
                 domain->level);

  if (domain->scope == scope) {
    DART_LOG_TRACE("dart__base__locality__scope_domains: domain %s matched",
                   domain->domain_tag);
    int     dom_idx           = *num_domains_out;
    *num_domains_out         += 1;
    dart_domain_locality_t ** domains_temp =
      (dart_domain_locality_t **)(realloc(*domains_out,
                                         sizeof(dart_domain_locality_t *) *
                                         (*num_domains_out)));
    if (domains_temp != NULL) {
      *domains_out            = domains_temp;
      (*domains_out)[dom_idx] = (dart_domain_locality_t *)(domain);
      DART_LOG_TRACE("dart__base__locality__scope_domains: "
                     "domain %d: %s",
                     dom_idx, (*domains_out)[dom_idx]);
    } else {
      DART_LOG_ERROR("dart__base__locality__scope_domains ! "
                     "realloc failed");
      return DART_ERR_OTHER;
    }
  } else {
    for (int d = 0; d < domain->num_domains; ++d) {
      ret = dart__base__locality__scope_domains_rec(
              &domain->domains[d],
              scope,
              num_domains_out,
              domains_out);
      if (ret != DART_OK) {
        return ret;
      }
    }
  }
  if (*num_domains_out <= 0) {
    DART_LOG_DEBUG("dart__base__locality__scope_domains ! "
                   "no domains found");
    return DART_ERR_NOTFOUND;
  }
  DART_LOG_TRACE("dart__base__locality__scope_domains >");
  return DART_OK;
}

