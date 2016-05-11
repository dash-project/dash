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
  dart_domain_locality_t  * domain,
  dart_locality_scope_t     scope,
  int                     * num_domains_out,
  char                  *** domain_tags_out);

dart_ret_t dart__base__locality__domain_intersect_rec(
  dart_domain_locality_t   * domain_in,
  int                        num_groups,
  char                    ** group_domain_tags,
  dart_domain_locality_t   * domain_out);

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
                 "team(%d) domain(%s)");
  dart_ret_t ret;

  *domain_out = NULL;

  /* Find common prefix of all domain tags in intersection: */
  char prefix[DART_LOCALITY_DOMAIN_TAG_MAX_SIZE];
  int  prefix_len = DART_LOCALITY_DOMAIN_TAG_MAX_SIZE;
  for (int g = 0; g < num_groups; ++g) {
    /* Common prefix of group domain tags: */
    char group_prefix[DART_LOCALITY_DOMAIN_TAG_MAX_SIZE];
    int  group_prefix_len = dart__base__strscommonprefix(
                              group_domain_tags[g],
                              group_sizes[g],
                              (char **)(&group_prefix));
    if (group_prefix_len < prefix_len ||
        strncmp(group_prefix, prefix, prefix_len) == 0) {
      /* Common prefix of domain tags in group is shorter than current
       * root prefix or root prefix is too specific: */
      char prefix_new[DART_LOCALITY_DOMAIN_TAG_MAX_SIZE];
      prefix_len = dart__base__strcommonprefix(prefix,
                                               group_prefix,
                                               (char **)(&prefix_new));
      strncpy(prefix, prefix_new, prefix_len);
    }
  }

  /* Get domain tagged with group domain tag prefix: */
  dart_domain_locality_t * domain_in_groups_parent;
  ret = dart__base__locality__domain(
          domain_in->team,
          prefix,
          &domain_in_groups_parent);
  if (ret != DART_OK) {
    return ret;
  }

  /* Create intersect locality domain hierarchy as new object: */
  dart_domain_locality_t * domain_split =
    malloc(sizeof(dart_domain_locality_t));
  DART_ASSERT_RETURNS(
    dart__base__locality__domain_locality_init(domain_split),
    DART_OK);
  /* Create one subdomain per group in the output domain: */
  strcpy(domain_split->host, domain_in->host);
  domain_split->domain_tag[0]  = '\0';
  domain_split->level          = 0;
  domain_split->scope          = DART_LOCALITY_SCOPE_GLOBAL;
  domain_split->relative_index = 0;
  domain_split->num_domains    = num_groups;
  domain_split->domains        = malloc(num_groups *
                                        sizeof(dart_domain_locality_t));

  /* Recurse original hierarchy and construct new split domain hierarchy
   * according to the specified grouping of domain tags.
   * Creates groups in subdomains of groups parent domain.
   * Parent domains are irrelevant as they are not part of any group. */
  for (int g = 0; g < num_groups; ++g) {
    domain_split->domains[g].parent         = domain_split;
    domain_split->domains[g].relative_index = g;
    DART_ASSERT_RETURNS(
      dart__base__locality__domain_intersect_rec(
        domain_in_groups_parent, group_sizes[g], group_domain_tags[g],
        &(domain_split->domains[g])),
      DART_OK);
  }

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
  dart_domain_locality_t   * domain_parent_in,
  int                        num_groups,
  char                    ** group_domain_tags,
  dart_domain_locality_t   * domain_out)
{
  /* Note that domain_parent_in->domain-tag is the domain tag prefix of
   * domains in the current scope. */

  /* Initialize subdomains: */
  for (int sd = 0; sd < domain_out->num_domains; ++sd) {
    /* Initialize a single domain and recurse into its subdomains: */
    dart_domain_locality_t * subdomain_out = &(domain_out->domains[sd]);
    dart_domain_locality_t * subdomain_in  = &(domain_parent_in->domains[sd]);
    strcpy(subdomain_out->host, domain_parent_in->host);
    subdomain_out->level          = domain_out->level + 1;
    subdomain_out->scope          = domain_parent_in->scope;
    subdomain_out->parent         = subdomain_out;
    subdomain_out->relative_index = sd;
    /* Copy domain tag from parent to subdomain tag and append subdomain's
     * relative index: */
    int parent_tag_len = 0;
    if (domain_out->level > 0) {
      parent_tag_len = sprintf(subdomain_out->domain_tag, "%s",
                               domain_out->domain_tag);
    }
    sprintf(subdomain_out->domain_tag + parent_tag_len, ".%d", sd);
    if (subdomain_out->level == 1) {
      subdomain_out->num_domains = num_groups;
    }
    DART_ASSERT_RETURNS(
      dart__base__locality__domain_intersect_rec(
        subdomain_in, num_groups, group_domain_tags,
        subdomain_out),
      DART_OK);
  }
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

