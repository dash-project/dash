/**
 * \file dash/dart/base/locality.c
 */

/*
 * Include config and utmpx.h first to prevent previous include of utmpx.h
 * without _GNU_SOURCE in included headers:
 */
#include <dash/dart/base/config.h>
#ifdef DART__PLATFORM__LINUX
#  define _GNU_SOURCE
#  include <utmpx.h>
#endif

#include <dash/dart/base/macro.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/base/locality.h>
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

#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sched.h>
#include <limits.h>

/* ====================================================================== *
 * Private Data                                                           *
 * ====================================================================== */

#define DART__BASE__LOCALITY__MAX_TEAM_DOMAINS 32

dart_host_topology_t *
dart__base__locality__host_topology_[DART__BASE__LOCALITY__MAX_TEAM_DOMAINS];

dart_unit_mapping_t *
dart__base__locality__unit_mapping_[DART__BASE__LOCALITY__MAX_TEAM_DOMAINS];

dart_domain_locality_t *
dart__base__locality__global_domain_[DART__BASE__LOCALITY__MAX_TEAM_DOMAINS];

/* ====================================================================== *
 * Private Functions                                                      *
 * ====================================================================== */

dart_ret_t dart__base__locality__scope_domains_rec(
  dart_domain_locality_t   * domain,
  dart_locality_scope_t      scope,
  int                      * num_domains_out,
  char                   *** domain_tags_out);

dart_ret_t dart__base__locality__domain_intersect_rec(
  dart_domain_locality_t   * domain_parent_in,
  int                        group_split_level,
  int                        num_groups,
  int                      * group_sizes,
  char                   *** group_domain_tags,
  dart_domain_locality_t   * domain_out);

dart_locality_scope_t dart__base__locality__scope_parent(
  dart_locality_scope_t      scope);

dart_locality_scope_t dart__base__locality__scope_child(
  dart_locality_scope_t      scope);

/* ====================================================================== *
 * Init / Finalize                                                        *
 * ====================================================================== */

dart_ret_t dart__base__locality__init()
{
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

dart_ret_t dart__base__locality__create(
  dart_team_t team)
{
  DART_LOG_DEBUG("dart__base__locality__create() team(%d)", team);

  dart_hwinfo_t * hwinfo;
  DART_ASSERT_RETURNS(dart_hwinfo(&hwinfo), DART_OK);

  for (int td = 0; td < DART__BASE__LOCALITY__MAX_TEAM_DOMAINS; ++td) {
    dart__base__locality__global_domain_[td] = NULL;
    dart__base__locality__host_topology_[td] = NULL;
  }

  dart_domain_locality_t * team_global_domain =
    malloc(sizeof(dart_domain_locality_t));
  dart__base__locality__global_domain_[team] =
    team_global_domain;

  /* Initialize the global domain as the root entry in the locality
   * hierarchy: */
  team_global_domain->scope          = DART_LOCALITY_SCOPE_GLOBAL;
  team_global_domain->level          = 0;
  team_global_domain->relative_index = 0;
  team_global_domain->team           = team;
  team_global_domain->parent         = NULL;
  team_global_domain->num_domains    = 0;
  team_global_domain->domains        = NULL;
  team_global_domain->hwinfo         = *hwinfo;
  team_global_domain->num_units      = 0;
  team_global_domain->host[0]        = '\0';
  team_global_domain->domain_tag[0]  = '.';
  team_global_domain->domain_tag[1]  = '\0';

  size_t num_units = 0;
  DART_ASSERT_RETURNS(dart_team_size(team, &num_units), DART_OK);
  team_global_domain->num_units = num_units;

  team_global_domain->unit_ids =
    malloc(num_units * sizeof(dart_unit_t));
  for (size_t u = 0; u < num_units; ++u) {
    team_global_domain->unit_ids[u] = u;
  }

  /* Exchange unit locality information between all units: */
  dart_unit_mapping_t * unit_mapping;
  DART_ASSERT_RETURNS(
    dart__base__unit_locality__create(team, &unit_mapping),
    DART_OK);
  dart__base__locality__unit_mapping_[team] = unit_mapping;

  /* Copy host names of all units into array: */
  const int max_host_len = DART_LOCALITY_HOST_MAX_SIZE;
  DART_LOG_TRACE("dart__base__locality__create: copying host names");
  char ** hosts = malloc(sizeof(char *) * num_units);
  for (size_t u = 0; u < num_units; ++u) {
    hosts[u] = malloc(sizeof(char) * max_host_len);
    dart_unit_locality_t * ul;
    DART_ASSERT_RETURNS(
      dart__base__unit_locality__at(unit_mapping, u, &ul),
      DART_OK);
    strncpy(hosts[u], ul->host, max_host_len);
  }

  dart_host_topology_t * topo = malloc(sizeof(dart_host_topology_t));
  DART_ASSERT_RETURNS(
    dart__base__host_topology__create(
      hosts, team, unit_mapping, topo),
    DART_OK);
  dart__base__locality__host_topology_[team] = topo;
  size_t num_nodes = topo->num_nodes;
  DART_LOG_TRACE("dart__base__locality__create: nodes: %d", num_nodes);

  team_global_domain->num_nodes = num_nodes;

#ifdef DART_ENABLE_LOGGING
  for (int h = 0; h < topo->num_hosts; ++h) {
    dart_node_units_t * node_units = &topo->node_units[h];
    char * hostname = topo->host_names[h];
    DART_LOG_TRACE("dart__base__locality__create: "
                   "host %s: units:%d level:%d parent:%s", hostname,
                   node_units->num_units, node_units->level,
                   node_units->parent);
    for (int u = 0; u < node_units->num_units; ++u) {
      DART_LOG_TRACE("dart__base__locality__create: %s unit[%d]: %d",
                     hostname, u, node_units->units[u]);
    }
  }
#endif

  /* recursively create locality information of the global domain's
   * sub-domains: */
  DART_ASSERT_RETURNS(
    dart__base__locality__create_subdomains(
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

  if (dart__base__locality__global_domain_[team] == NULL &&
      dart__base__locality__host_topology_[team] == NULL &&
      dart__base__locality__unit_mapping_[team]  == NULL) {
    return ret;
  }

  DART_LOG_DEBUG("dart__base__locality__delete() team(%d)", team);

  if (dart__base__locality__global_domain_[team] != NULL) {
    ret = dart__base__locality__domain_delete(
            dart__base__locality__global_domain_[team]);
    if (ret != DART_OK) {
      DART_LOG_ERROR("dart__base__locality__delete ! "
                     "dart__base__locality__domain_delete failed: %d", ret);
      return ret;
    }
    dart__base__locality__global_domain_[team] = NULL;
  }

  if (dart__base__locality__host_topology_[team] != NULL) {
    ret = dart__base__host_topology__delete(
            dart__base__locality__host_topology_[team]);
    if (ret != DART_OK) {
      DART_LOG_ERROR("dart__base__locality__delete ! "
                     "dart__base__host_topology__delete failed: %d", ret);
      return ret;
    }
    dart__base__locality__host_topology_[team] = NULL;
  }

  if (dart__base__locality__unit_mapping_[team] != NULL) {
    ret = dart__base__unit_locality__delete(
            dart__base__locality__unit_mapping_[team]);
    if (ret != DART_OK) {
      DART_LOG_ERROR("dart__base__locality__delete ! "
                     "dart__base__unit_locality__delete failed: %d", ret);
      return ret;
    }
    dart__base__locality__unit_mapping_[team] = NULL;
  }

  DART_LOG_DEBUG("dart__base__locality__delete > team(%d)", team);
  return DART_OK;
}

/* ====================================================================== *
 * Domain Locality                                                        *
 * ====================================================================== */

dart_ret_t dart__base__locality__domain(
  dart_team_t               team,
  const char              * domain_tag,
  dart_domain_locality_t ** domain_out)
{
  DART_LOG_DEBUG("dart__base__locality__domain() team(%d) domain(%s)",
                 team, domain_tag);

  *domain_out = NULL;

  dart_domain_locality_t * domain =
    dart__base__locality__global_domain_[team];

  /* pointer to dot separator in front of tag part:  */
  char * dot_begin  = strchr(domain_tag, '.');
  /* pointer to begin of tag part: */
  char * part_begin = dot_begin + 1;
  /* Iterate tag (.1.2.3) by parts (1, 2, 3):        */
  int    level     = 0;
  while (dot_begin != NULL && *dot_begin != '\0' && *part_begin != '\0') {
    char * dot_end;
    /* domain tag part converted to int is relative index of child: */
    long   tag_part      = strtol(part_begin, &dot_end, 10);
    int    subdomain_idx = (int)(tag_part);
    if (domain == NULL) {
      /* tree leaf node reached before last tag part: */
      DART_LOG_ERROR("dart__base__locality__domain ! team(%d) domain(%s): "
                     "subdomain at index %d in level %d is undefined",
                     team, domain_tag, subdomain_idx, level);
      return DART_ERR_INVAL;
    }
    if (domain->num_domains <= subdomain_idx) {
      /* child index out of range: */
      DART_LOG_ERROR("dart__base__locality__domain ! team(%d) domain(%s): "
                     "subdomain index %d in level %d is out ouf bounds "
                     "(number of subdomains: %d)",
                     team, domain_tag, subdomain_idx, level,
                     domain->num_domains);
      return DART_ERR_INVAL;
    }
    /* descend to child at relative index: */
    domain     = domain->domains + subdomain_idx;
    /* continue scanning of domain tag at next part: */
    dot_begin  = dot_end;
    part_begin = dot_end+1;
    level++;
  }
  *domain_out = domain;
  DART_LOG_DEBUG("dart__base__locality__domain > team(%d) domain(%s) -> %p",
                 team, domain_tag, domain);
  return DART_OK;
}

dart_ret_t dart__base__locality__scope_domains(
  dart_domain_locality_t  * domain_in,
  dart_locality_scope_t     scope,
  int                     * num_domains_out,
  char                  *** domain_tags_out)
{
  *num_domains_out = 0;
  *domain_tags_out = NULL;
  return dart__base__locality__scope_domains_rec(
           domain_in, scope, num_domains_out, domain_tags_out);
}

dart_ret_t dart__base__locality__domain_split_tags(
  dart_domain_locality_t  * domain_in,
  dart_locality_scope_t     scope,
  int                       num_parts,
  int                    ** group_sizes_out,
  char                 **** group_domain_tags_out)
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
    dart__base__locality__scope_domains(
      domain_in, scope, &num_domains, &domain_tags),
    DART_OK);

  /* Group domains in split scope into specified number of parts: */
  int max_group_domains      = (num_domains + (num_parts-1)) / num_parts;
  int group_first_domain_idx = 0;
  /*
   * TODO: Preliminary implementation, should balance number of units in
   *       groups.
   */
  for (int g = 0; g < num_parts; ++g) {
    int num_group_subdomains = max_group_domains;
    if ((g+1) * max_group_domains > num_domains) {
      num_group_subdomains = (g * max_group_domains) - num_domains;
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
              DART_LOCALITY_DOMAIN_TAG_MAX_SIZE);
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

dart_ret_t dart__base__locality__domain_intersect(
  dart_domain_locality_t   * domain_in,
  int                        num_groups,
  int                      * group_sizes,
  char                   *** group_domain_tags,
  dart_domain_locality_t  ** domain_out)
{
  DART_LOG_TRACE("dart__base__locality__domain_intersect() "
                 "domain_in: (%s: %d @ %d) num_groups: %d",
                 domain_in->domain_tag, domain_in->scope, domain_in->level,
                 num_groups);
  dart_ret_t ret;

  *domain_out = NULL;

  if (num_groups < 1) {
    return DART_ERR_INVAL;
  }

  /* --------------------------------------------------------------------- *
   * Step 1:                                                               *
   *   Resolve tag of the groups' parent domain in the input domain        *
   *   hierarchy (domain_in_groups_parent).                                *
   * --------------------------------------------------------------------- */

  /* Find common prefix of all group domain domain tags.
   * Example:
   *
   *   group_domain_tags = { { 0.0.1.2, 0.0.1.3 },
   *                         { 0.0.1.4, 0.0.2.0 } }
   *
   *   --> common domain tag prefix: '0.0'
   */
  char prefix[DART_LOCALITY_DOMAIN_TAG_MAX_SIZE];
  strcpy(prefix, group_domain_tags[0][0]);
  int  prefix_len = strlen(prefix);
  for (int g = 0; g < num_groups; g++) {
#ifdef DART_ENABLE_LOGGING
    for (int d = 0; d < group_sizes[g]; d++) {
      DART_LOG_TRACE("dart__base__locality__domain_intersect: "
                     "  group[%d].domain_tag[%d]: %s",
                     g, d, group_domain_tags[g][d]);
    }
#endif
    /* Common prefix of group domain tags: */
    char group_prefix[DART_LOCALITY_DOMAIN_TAG_MAX_SIZE];
    int  group_prefix_len = dart__base__strscommonprefix(
                              group_domain_tags[g],
                              group_sizes[g],
                              group_prefix);
    if (group_prefix[group_prefix_len - 1] == '.') {
      // Remove trailing '.':
      group_prefix_len--;
      group_prefix[group_prefix_len] = '\0';
    }
    DART_LOG_TRACE("dart__base__locality__domain_intersect: "
                   "common tag prefix of group %d: '%s', length %d",
                   g, group_prefix, group_prefix_len);
    if (group_prefix_len < prefix_len) {
      char prefix_new[DART_LOCALITY_DOMAIN_TAG_MAX_SIZE];
      prefix_len = dart__base__strcommonprefix(prefix,
                                               group_prefix,
                                               prefix_new);
      strncpy(prefix, prefix_new, prefix_len);
    }
  }
  prefix[prefix_len] = '\0';

  for (int g = 0; g < num_groups; g++) {
    for (int d = 0; d < group_sizes[g]; d++) {
      if (strncmp(prefix, group_domain_tags[g][d], prefix_len) == 0) {
        char * last_dot_pos = strrchr(prefix, '.');
        if (last_dot_pos != NULL) { *last_dot_pos = '\0'; }
      }
    }
  }

  DART_LOG_TRACE("dart__base__locality__domain_intersect: "
                 "group parent domain tag: '%s'", prefix);

  /* Get domain tagged with group domain tag prefix: */
  dart_domain_locality_t * domain_in_groups_parent;
  ret = dart__base__locality__domain(
          domain_in->team,
          prefix,
          &domain_in_groups_parent);
  if (ret != DART_OK) {
    return ret;
  }

  /* The number of levels in the common prefix of all group domain tags
   * denotes the level in the locality hierarchy that is split by the
   * groups.
   * Domains are never split at level 0 (domain tag '.') as there is only
   * one global scope.
   */
  int group_split_level = dart__base__strcnt(prefix, '.');
  DART_LOG_TRACE("dart__base__locality__domain_intersect: "
                 "group split level: %d", group_split_level);
  DART_ASSERT(group_split_level > 0);

  /* --------------------------------------------------------------------- *
   * Step 2:                                                               *
   *   Create new dart_domain_locality_t object for the resulting domain   *
   *   hierarchy (domain_split).                                           *
   *                                                                       *
   *   Note that the resulting (grouped) domain hierarchy has an           *
   *   additional locality scope DART_LOCALITY_SCOPE_GROUP at the split    *
   *   level \c ls, so level \c n in the input domain corresponds to       *
   *   level \c (n-1) in the result hierarchy for \c (n > ls).             *
   * --------------------------------------------------------------------- */

  /* Create intersect locality domain hierarchy as new object: */
  dart_domain_locality_t * domain_split =
    malloc(sizeof(dart_domain_locality_t));
  DART_ASSERT_RETURNS(
    dart__base__locality__domain_locality_init(domain_split),
    DART_OK);
  strcpy(domain_split->host, domain_in->host);
  domain_split->domain_tag[0]  = '.';
  domain_split->domain_tag[1]  = '\0';
  domain_split->level          = 0;
  domain_split->scope          = DART_LOCALITY_SCOPE_GLOBAL;
  domain_split->parent         = NULL;
  domain_split->relative_index = 0;
  /* Create one subdomain per group in the output domain: */
  domain_split->num_domains    = num_groups;
  domain_split->domains        = malloc(num_groups *
                                        sizeof(dart_domain_locality_t));

  /* Recurse original hierarchy and construct new split domain hierarchy
   * according to the specified grouping of domain tags.
   * Creates groups in subdomains of groups parent domain. */
  DART_ASSERT_RETURNS(
    dart__base__locality__domain_intersect_rec(
      domain_in,
      group_split_level,
      num_groups,
      group_sizes,
      group_domain_tags,
      domain_split),
    DART_OK);

  *domain_out = domain_split;

  DART_LOG_TRACE("dart__base__locality__domain_intersect >");
  return DART_OK;
}

/* ====================================================================== *
 * Unit Locality                                                          *
 * ====================================================================== */

dart_ret_t dart__base__locality__unit(
  dart_team_t             team,
  dart_unit_t             unit,
  dart_unit_locality_t ** locality)
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
                   "failed (%d)", team, unit, ret);
    return ret;
  }
  *locality = uloc;

  DART_LOG_DEBUG("dart__base__locality__unit > team(%d) unit(%d)",
                 team, unit);
  return DART_OK;
}

/* ====================================================================== *
 * Private Function Definitions                                           *
 * ====================================================================== */

dart_ret_t dart__base__locality__domain_intersect_rec(
  dart_domain_locality_t   * domain_in,
  int                        group_split_level,
  int                        num_groups,
  int                      * group_sizes,
  char                   *** group_domain_tags,
  dart_domain_locality_t   * domain_out)
{
  DART_LOG_TRACE("dart__base__locality__domain_intersect_rec() "
                 "parent_in: (%s: %d @ %d) domain_out: (%s: %d @ %d) "
                 "num_groups: %d",
                 domain_parent_in->domain_tag,
                 domain_parent_in->scope,
                 domain_parent_in->level,
                 domain_out->domain_tag,
                 domain_out->scope,
                 domain_out->level,
                 num_groups);

  /* Preconditions of a single recursion step:
   *
   * - Input- and output hierarchies represented by domain_in and
   *   domain_out refer to the same domain, i.e. the same entry in the same
   *   locality hierarchy of the same team.
   * - All attributes in the output domain \c domain_out are specified,
   *   except for child node lists such as the fields \c unit_ids and \c
   *   domains which are not allocated and set to \c NULL.
   *   The capacity fields \c num_units and \num_domains must be specified,
   *   however.
   *
   * Postconditions of a single recursion step:
   *
   * - The child node lists of \c domain_in are specified.
   * - The domains \c domain_in->domains[sd] and \c domain_out->domains[sd]
   *   are passed to the next recursion as arguments of \c domain_in and
   *   \c domain_out in a sequential iteration in
   *   (sd: (0...domain_in->num_domains].
   */

  dart_locality_scope_t subdomain_scope =
    dart__base__locality__scope_child(domain_out->scope);

  /* The recursion procecure has three stages:
   *
   * 1. If the group split hierarchy level has not been reached yet, just
   *    clone the entries from the input domain.
   * 2. When the group split level is reached, introduce an additional
   *    hierarchy level at scope DART_LOCALITY_SCOPE_GROUP.
   *    Hierarchy levelsi \c l below the split level \c (l: l > ls) now
   *    correspond to entries in the input domain such that
   *    \c (l_in ~ l_out-1 | l_out > ls).
   *    Below the split level \c ls, data is not copied from the input
   *    domain but resolved from the group domain tags instead.
   * 3. When the lowest locality level has been reached, capacities of
   *    domains above the split level (still containing cloned date from
   *    input domain) are updated using the aggregated capacities of their
   *    subdomains (tail recursion).
   */

  DART_ASSERT(domain_in_team == domain_out->team);

  /* Maps group index to number of subdomains that are delegated to the
   * group g: */
  int  * num_delegate_subdomains     = NULL;
  /* Maps group index to relative indices of subdomains that are delegated to
   * the group g: */
  int ** delegate_subdomain_indices  = NULL;
  /* Total number of subdomains that will be transferred to group domains: */
  int total_delegate_subdomains      = 0;
  /* Number of groups that will take ownership of subdomains: */
  int num_delegate_groups            = 0;
  if (domain_out->level == group_split_level) {
    /* Current level is earliest group split level so this subdomain might
     * be affected by grouping caused by a split.
     *
     * Check if there are group domain tags that consist of the entire tag of
     * this subdomain as a prefix and a single additional tag part.
     *
     * If so, then encapsulate them in a group domain and pass ownership:
     * - Remove them from the domain list of this subdomain.
     * - Add a domain at group level to the domain list of this subdomain.
     *   The removed child domains are then delegated to the group domain.
     */
    int domain_tag_len = strlen(domain_out->domain_tag);
    for (int g = 0; g < num_groups; g++) {
      /* Whether one or more subdomains are delegated to group g: */
      int delegate_to_group             = 0;
      for (int d = 0; d < group_sizes[g]; d++) {
        if (strncmp(domain_out->domain_tag, group_domain_tags[g][d],
                    domain_tag_len)) {
          /* Tag of domain_out is prefix of a group domain tag ... */
          if (dart__base__strcnt(domain_out->domain_tag, '.') ==
              dart__base__strcnt(group_domain_tags[g][d], '.') - 1) {
            /* ... and the group domain tag's scope is an immediate child
             * so the module passes ownership of the domain identified by
             * group_domain_tags[g][d]:
             */
            /* Collect all subdomains that will be passed to group g: */
            if (delegate_subdomain_indices == NULL) {
              delegate_subdomain_indices =
                calloc(num_groups * sizeof(int *), sizeof(int *));
            }
            if (delegate_subdomain_indices[g] == NULL) {
              delegate_subdomain_indices[g] =
                malloc(group_sizes[g] * sizeof(int));
            }
            if (num_delegate_subdomains == NULL) {
              num_delegate_subdomains =
                malloc(num_groups * sizeof(int));
            }
            char * subdomain_tag = group_domain_tags[g][d];
            delegate_subdomain_indices[g][num_delegate_subdomains[g]] =
              strtol(strrchr(subdomain_tag, '.') + 1, NULL, 10);

            delegate_to_group = 1;
            total_delegate_subdomains++;
            num_delegate_subdomains[g]++;
          }
        }
      }
      num_delegate_groups += delegate_to_group;
    }
  }
  /* Below group split level, domain is a group and its relative index
   * represents the relative group index.
   * Resolve domains and units using group domain tags.
   */
  else if (domain_out->level == group_split_level + 1) {
  }

  if (total_delegate_subdomains > 0) {
    /* Remove delegated subdomains from domain capacity: */
    domain_out->num_domains -= total_delegate_subdomains;
    /* Add new group subdomains to domain capacity: */
    domain_out->num_domains += num_delegate_groups;
  }

  /* Initialize list attributes of domain_in: */
  domain_out->domains = malloc(domain_out->num_domains *
                               sizeof(dart_domain_locality_t));
  domain_out->domains = malloc(domain_out->num_units *
                               sizeof(dart_unit_t));

  for (int sd = 0; sd < domain_in->num_domains; ++sd) {
    DART_LOG_TRACE("dart__base__locality__domain_intersect_rec "
                   "subdomain[%d]", sd);
    /*
     * Initialize and recurse into the subdomain at relative index sd.
     *
     * Note: Index sd refers to the relative index of the subdomain in the
     *       input domain domain_in.
     */

    dart_domain_locality_t * subdomain_out = &(domain_out->domains[sd]);
    /* Locality scope at group split level is only added in output domain:
     */
    dart_domain_locality_t * subdomain_in  =
      (domain_out->level != group_split_level)
       ? domain_in->domains + sd
       : domain_in;

    /* While group split level has not been reached, clone domain capacities
     * from input domain.
     * Subdomains that are not part of a domain group will be pruned in the
     * bottom-up phase.
     */
    int num_domains = subdomain_in->num_domains;
    /* Unit capacities are set in recursion below group split level and
     * aggregated in bottom-up phase: */
    int num_units   = 0;

    /*
     * Delegate subdomains to groups first, if any:
     */
    if (num_delegate_groups > 0) {
      int g = sd;
      if (delegate_subdomain_indices != NULL &&
          delegate_subdomain_indices[g] != NULL) {
        /* There are subdomains to be delegated to group g: */
        subdomain_scope = DART_LOCALITY_SCOPE_GROUP;
        num_domains     = num_delegate_subdomains[g];
      }
    }

    /* Initialize subdomain as blank object of dart_domain_locality_t:
     */
    DART_ASSERT_RETURNS(
      dart__base__locality__domain_locality_init(subdomain_out),
      DART_OK);

    /* Set fields of the subdomain object as described in the recursion
     * preconditions:
     */
    strcpy(subdomain_out->host, domain_in->host);
    subdomain_out->level          = domain_out->level + 1;
    subdomain_out->scope          = subdomain_scope;
    subdomain_out->parent         = domain_out;
    subdomain_out->relative_index = sd;
    subdomain_out->domains        = NULL;
    subdomain_out->unit_ids       = NULL;

    /* Copy domain tag from parent to subdomain tag and append subdomain's
     * relative index:
     */
    int parent_tag_len = 0;
    if (domain_out->level > 0) {
      parent_tag_len = sprintf(subdomain_out->domain_tag, "%s",
                               domain_out->domain_tag);
    }
    sprintf(subdomain_out->domain_tag + parent_tag_len, ".%d", sd);

    subdomain_out->num_domains = num_domains;
    subdomain_out->domains     = malloc(num_domains *
                                    sizeof(dart_domain_locality_t));
    subdomain_out->num_units   = num_units;

    DART_ASSERT_RETURNS(
      dart__base__locality__domain_intersect_rec(
        subdomain_in,
        group_split_level,
        num_groups,
        group_sizes,
        group_domain_tags,
        subdomain_out),
      DART_OK);

    /* ------------------------------------------------------------------- *
     * Bottom-Up Phase:                                                    *
     *   After recursion into the subdomain_out object, it is now fully    *
     *   specified and it's fields are now aggreated and to update domain  *
     *   entries in parent scopes.                                         *
     * ------------------------------------------------------------------- */

    domain_out->num_units   += subdomain_out->num_units;
    domain_out->num_domains += subdomain_out->num_domains;
  }
  /* --------------------------------------------------------------------- *
   * Finalizing Phase:                                                     *
   *   All subdomains of domain_out are now fully specified.               *
   *   Aggregate attribute values of subdomains and update its properties. *
   * --------------------------------------------------------------------- */

  DART_LOG_TRACE("dart__base__locality__domain_intersect_rec >");
  return DART_OK;
}

dart_ret_t dart__base__locality__scope_domains_rec(
  dart_domain_locality_t  * domain,
  dart_locality_scope_t     scope,
  int                     * num_domains_out,
  char                  *** domain_tags_out)
{
  dart_ret_t ret;
  DART_LOG_TRACE("dart__base__locality__scope_domains() level %d",
                 domain->level);

  if (domain->scope == scope) {
    DART_LOG_TRACE("dart__base__locality__scope_domains domain %s matched",
                   domain->domain_tag);
    int     dom_idx           = *num_domains_out;
    *num_domains_out         += 1;
    char ** domain_tags_temp  = (char **)(
                                  realloc(*domain_tags_out,
                                            sizeof(char *) *
                                            (*num_domains_out)));
    if (domain_tags_temp != NULL) {
      int domain_tag_size         = strlen(domain->domain_tag) + 1;
      *domain_tags_out            = domain_tags_temp;
      (*domain_tags_out)[dom_idx] = malloc(sizeof(char) * domain_tag_size);
      strncpy((*domain_tags_out)[dom_idx], domain->domain_tag,
              DART_LOCALITY_DOMAIN_TAG_MAX_SIZE);
    } else {
      DART_LOG_ERROR("dart__base__locality__scope_domains ! realloc failed");
      return DART_ERR_OTHER;
    }
  } else {
    for (int d = 0; d < domain->num_domains; ++d) {
      ret = dart__base__locality__scope_domains_rec(
              &domain->domains[d],
              scope,
              num_domains_out,
              domain_tags_out);
      if (ret != DART_OK) {
        return ret;
      }
    }
  }
  if (*num_domains_out <= 0) {
    DART_LOG_ERROR("dart__base__locality__scope_domains ! no domains found");
    return DART_ERR_NOTFOUND;
  }
  DART_LOG_TRACE("dart__base__locality__scope_domains >");
  return DART_OK;
}

dart_locality_scope_t dart__base__locality__scope_parent(
  dart_locality_scope_t scope)
{
  switch (scope) {
    case DART_LOCALITY_SCOPE_GLOBAL: return DART_LOCALITY_SCOPE_NODE;
    case DART_LOCALITY_SCOPE_NODE:   return DART_LOCALITY_SCOPE_MODULE;
    case DART_LOCALITY_SCOPE_MODULE: return DART_LOCALITY_SCOPE_NUMA;
    case DART_LOCALITY_SCOPE_NUMA:   return DART_LOCALITY_SCOPE_CORE;
    default:                         return DART_LOCALITY_SCOPE_UNDEFINED;
  }
}

dart_locality_scope_t dart__base__locality__scope_child(
  dart_locality_scope_t scope)
{
  switch (scope) {
    case DART_LOCALITY_SCOPE_CORE:   return DART_LOCALITY_SCOPE_NUMA;
    case DART_LOCALITY_SCOPE_NUMA:   return DART_LOCALITY_SCOPE_MODULE;
    case DART_LOCALITY_SCOPE_MODULE: return DART_LOCALITY_SCOPE_NODE;
    case DART_LOCALITY_SCOPE_NODE:   return DART_LOCALITY_SCOPE_GLOBAL;
    default:                         return DART_LOCALITY_SCOPE_UNDEFINED;
  }
}
