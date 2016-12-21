/**
 * \file dash/dart/base/internal/domain_locality.c
 */
#include <dash/dart/base/macro.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/base/locality.h>
#include <dash/dart/base/assert.h>
#include <dash/dart/base/hwinfo.h>

#include <dash/dart/base/string.h>
#include <dash/dart/base/array.h>
#include <dash/dart/base/math.h>
#include <dash/dart/base/internal/host_topology.h>
#include <dash/dart/base/internal/unit_locality.h>
#include <dash/dart/base/internal/domain_locality.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_locality.h>

#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sched.h>
#include <limits.h>

/* ======================================================================== *
 * Private Functions: Declarations                                          *
 * ======================================================================== */

dart_ret_t dart__base__locality__domain__create_node_subdomains(
  dart_domain_locality_t         * node_domain,
  dart_host_topology_t           * host_topology,
  dart_unit_mapping_t            * unit_mapping);

dart_ret_t dart__base__locality__domain__create_module_subdomains(
  dart_domain_locality_t         * module_domain,
  dart_host_topology_t           * host_topology,
  dart_unit_mapping_t            * unit_mapping,
  int                              module_scope_level);

/* ===================================================================== *
 * Internal Functions                                                    *
 * ===================================================================== */

dart_ret_t dart__base__locality__domain__init(
  dart_domain_locality_t * loc)
{
  if (loc == NULL) {
    DART_LOG_ERROR("dart__base__locality__domain_locality_init ! null");
    return DART_ERR_INVAL;
  }
  loc->domain_tag[0]    = '\0';
  loc->num_aliases      = 0;
  loc->aliases          = NULL;
  loc->host[0]          = '\0';
  loc->scope            = DART_LOCALITY_SCOPE_UNDEFINED;
  loc->global_index     = -1;
  loc->team             = DART_TEAM_NULL;
  loc->level            = 0;
  loc->relative_index   = 0;
  loc->parent           = NULL;
  loc->domains          = NULL;
  loc->num_domains      = 0;
  loc->unit_ids         = NULL;
  loc->num_units        = -1;
  loc->num_nodes        = -1;
  loc->num_cores        = -1;
  loc->shared_mem_bytes = -1;

  return DART_OK;
}

dart_ret_t dart__base__locality__domain__destruct(
  dart_domain_locality_t * domain)
{
  dart_ret_t ret = DART_OK;

  if (domain == NULL) {
    DART_LOG_DEBUG("dart__base__locality__domain__destruct > domain NULL");
    return DART_OK;
  }

  if (domain->num_domains > 0 && NULL == domain->domains) {
    DART_LOG_ERROR("dart__base__locality__domain__destruct ! "
                   "domain.domains must not be NULL for "
                   "domain.num_domains = %d in %s",
                   domain->num_domains, domain->domain_tag);
    return DART_ERR_INVAL;
  }
  else if (domain->num_domains == 0 && NULL != domain->domains) {
    DART_LOG_ERROR("dart__base__locality__domain__destruct ! "
                   "domain.domains expected to be NULL for "
                   "domain.num_domains = %d in %s",
                   domain->num_domains, domain->domain_tag);
    return DART_ERR_INVAL;
  }

  /* deallocate child nodes in depth-first recursion: */
  for (int subdom_idx = 0; subdom_idx < domain->num_domains; ++subdom_idx) {
    ret = dart__base__locality__domain__destruct(
            domain->domains + subdom_idx);
    if (DART_OK != ret) {
      DART_LOG_ERROR("dart__base__locality__domain__destruct ! "
                     "failed to destroy domain(%p)->domains[%d]",
                     (void *)domain, subdom_idx);
      return ret;
    }
  }
  /* deallocate node itself: */
  if (NULL != domain->domains) {
    free(domain->domains);
    domain->domains  = NULL;
  }
  if (NULL != domain->unit_ids) {
    free(domain->unit_ids);
    domain->unit_ids = NULL;
  }
  domain->num_domains = 0;
  domain->num_units   = 0;

  return DART_OK;
}

dart_ret_t dart__base__locality__domain__copy(
  const dart_domain_locality_t   * domain_src,
  dart_domain_locality_t         * domain_dst)
{
  dart_ret_t ret = DART_ERR_OTHER;

  dart__base__locality__domain__init(domain_dst);

  memcpy(domain_dst, domain_src, sizeof(dart_domain_locality_t));

  /* Copy unit ids:
   */
  if (domain_src->num_units > 0) {
    if (NULL == domain_src->unit_ids) {
      DART_LOG_ERROR("dart__base__locality__domain__copy: domain %s "
                     "has num_units = %d but domain->unit_ids is NULL",
                     domain_src->domain_tag, domain_src->num_units);
      return DART_ERR_OTHER;
    }
    domain_dst->unit_ids = malloc(sizeof(dart_unit_t) *
                                  domain_src->num_units);
    for (int u = 0; u < domain_src->num_units; u++) {
      domain_dst->unit_ids[u] = domain_src->unit_ids[u];
    }
  } else {
    if (NULL != domain_src->unit_ids) {
      DART_LOG_ERROR("dart__base__locality__domain__copy: domain %s "
                     "has num_units = %d, expected unit_ids == NULL",
                     domain_src->domain_tag, domain_src->num_units);
      return DART_ERR_OTHER;
    }
    domain_dst->unit_ids = NULL;
  }

  /* Copy subdomains:
   */
  if (domain_src->num_domains > 0) {
    if (NULL == domain_src->domains) {
      DART_LOG_ERROR("dart__base__locality__domain__copy: domain %s "
                     "has num_domains = %d, expected domains != NULL",
                     domain_src->domain_tag, domain_src->num_domains);
      return DART_ERR_OTHER;
    }
    domain_dst->domains = malloc(sizeof(dart_domain_locality_t) *
                                 domain_src->num_domains);
  } else {
    if (NULL != domain_src->domains) {
      DART_LOG_ERROR("dart__base__locality__domain__copy: domain %s "
                     "has num_domains = %d, expected domains = NULL",
                     domain_src->domain_tag, domain_src->num_domains);
      return DART_ERR_OTHER;
    }
    domain_dst->domains = NULL;
  }

  /* Recursively copy subdomains:
   */
  for (int sd = 0; sd < domain_src->num_domains; sd++) {
    const dart_domain_locality_t * subdomain_src = domain_src->domains + sd;
    dart_domain_locality_t       * subdomain_dst = domain_dst->domains + sd;

    ret = dart__base__locality__domain__copy(
            subdomain_src,
            subdomain_dst);
    if (ret != DART_OK) {
      return ret;
    }
    domain_dst->domains[sd].parent = domain_dst;
  }
  return DART_OK;
}

dart_ret_t dart__base__locality__domain__update_subdomains(
  dart_domain_locality_t         * domain)
{
  int is_unit_scope = ((int)domain->scope >= (int)DART_LOCALITY_SCOPE_CORE);
  DART_LOG_TRACE("dart__base__locality__domain__update_subdomains() "
                 "domain: %s, scope: %d, subdomains: %d, units: %d, "
                 "in unit scope: %s",
                 domain->domain_tag,  domain->scope,
                 domain->num_domains, domain->num_units,
                 (is_unit_scope ? "true" : "false"));

  if (!is_unit_scope) {
    domain->num_units = 0;
  }
  for (int sd = 0; sd < domain->num_domains; sd++) {
    dart_domain_locality_t * subdomain = domain->domains + sd;
    subdomain->team           = domain->team;
    subdomain->level          = domain->level + 1;
    subdomain->relative_index = sd;
    subdomain->parent         = domain;

    sprintf(subdomain->domain_tag, "%s.%d", domain->domain_tag, sd);

    DART_LOG_TRACE("dart__base__locality__domain__update_subdomains --v");
    /* Recursively update subdomains: */
    dart__base__locality__domain__update_subdomains(
      subdomain);
    DART_LOG_TRACE("dart__base__locality__domain__update_subdomains --^");

    if (!is_unit_scope) {
      domain->num_units += subdomain->num_units;
    }
  }
  if (domain->num_units   == 0 && domain->unit_ids != NULL) {
    DART_LOG_DEBUG("dart__base__locality__domain__destruct: "
                   "free(domain->unit_ids)");
    free(domain->unit_ids);
  }
  if (domain->num_domains == 0 && domain->domains  != NULL) {
    DART_LOG_DEBUG("dart__base__locality__domain__destruct: "
                   "free(domain->domains)");
    free(domain->domains);
  }
  if (domain->num_units > 0) {
    if (is_unit_scope) {
      dart_global_unit_t unit_id = domain->unit_ids[0];
      DART_LOG_DEBUG("dart__base__locality__domain__destruct: "
                     "free(domain->unit_ids)");
      free(domain->unit_ids);
      domain->unit_ids    = malloc(sizeof(dart_global_unit_t));
      domain->unit_ids[0] = unit_id;
    } else {
      domain->unit_ids    = malloc(sizeof(dart_global_unit_t) * domain->num_units);
      int domain_unit_idx = 0;
      for (int sd = 0; sd < domain->num_domains; sd++) {
        dart_domain_locality_t * subdomain = domain->domains + sd;
        memcpy(domain->unit_ids + domain_unit_idx,
               subdomain->unit_ids,
               sizeof(dart_global_unit_t) * subdomain->num_units);
        domain_unit_idx += subdomain->num_units;
      }
    }
  } else {
    if (NULL != domain->unit_ids) {
      free(domain->unit_ids);
    }
    domain->unit_ids = NULL;
  }
  DART_LOG_TRACE("dart__base__locality__domain__update_subdomains > "
                 "domain: %s, scope: %d, subdomains: %d, units: %d",
                 domain->domain_tag,  domain->scope,
                 domain->num_domains, domain->num_units);
  return DART_OK;
}

/**
 * Find subdomain at arbitrary level below a specified domain.
 */
dart_ret_t dart__base__locality__domain__child(
  const dart_domain_locality_t   * domain,
  const char                     * subdomain_tag,
  dart_domain_locality_t        ** subdomain_out)
{
  if (strcmp(domain->domain_tag, subdomain_tag) == 0) {
    *subdomain_out = (dart_domain_locality_t *)(domain);
    return DART_OK;
  }
  /*
   * TODO: Optimize, currently using exhaustive search.
   */
  for (int sd = 0; sd < domain->num_domains; ++sd) {
    if (dart__base__locality__domain__child(
          &domain->domains[sd], subdomain_tag, subdomain_out)
        == DART_OK) {
      DART_LOG_TRACE("dart__base__locality__domain__child - "
                     "domain:%s, subdomain_tag:%s -> %s",
                     (NULL != domain) ? domain->domain_tag : "?",
                     subdomain_tag, (*subdomain_out)->domain_tag);
      return DART_OK;
    }
  }
  DART_LOG_TRACE("dart__base__locality__domain__child - "
                 "no subdomain %s in %s", subdomain_tag, domain->domain_tag);
  *subdomain_out = NULL;
  return DART_ERR_NOTFOUND;
}

/**
 * Find common parent of specified domains.
 */
dart_ret_t dart__base__locality__domain__parent(
  const dart_domain_locality_t   * domain_in,
  const char                    ** subdomain_tags,
  int                              num_subdomain_tags,
  dart_domain_locality_t        ** domain_out)
{
  *domain_out = NULL;

  /* Parent domain tag of subdomains is common prefix of subdomain tags:
   */
  char subdomains_prefix[DART_LOCALITY_DOMAIN_TAG_MAX_SIZE];
  int  subdomains_prefix_len = dart__base__strscommonprefix(
                                 subdomain_tags,
                                 num_subdomain_tags,
                                 subdomains_prefix);
  /* Remove trailing '.' */
  if (subdomains_prefix[subdomains_prefix_len - 1] == '.') {
    subdomains_prefix_len--;
    subdomains_prefix[subdomains_prefix_len] = '\0';
  }
  if (subdomains_prefix_len == 0) {
    *domain_out = (dart_domain_locality_t *)(domain_in);
    return DART_OK;
  }

  /* Find domain tagged with subdomains prefix: */
  return dart__base__locality__domain__child(
           domain_in, subdomains_prefix, domain_out);
}

/**
 * Remove all child nodes from a domain that do not match the specified
 * domain tags.
 */
dart_ret_t dart__base__locality__domain__filter_subdomains_if(
  dart_domain_locality_t         * domain,
  dart_domain_predicate_t          pred)
{
  dart__unused(domain);
  dart__unused(pred);

  return DART_OK;
}

/**
 * Remove all child nodes from a domain that match or do not match the
 * specified domain tags.
 */
dart_ret_t dart__base__locality__domain__filter_subdomains(
  dart_domain_locality_t         * domain,
  const char                    ** subdomain_tags,
  int                              num_subdomain_tags,
  int                              remove_matches)
{
  dart_ret_t ret;

  int is_unit_scope = ((int)domain->scope >= (int)DART_LOCALITY_SCOPE_CORE);
  int matched       = 0;
  int unit_idx      = 0;
  int subdomain_idx = 0;
  // int num_numa   = 0;

  DART_LOG_TRACE("dart__base__locality__domain__filter_subdomains() "
                 "domain: %s, level: %d, domains: %d, units: %d",
                 domain->domain_tag,  domain->level,
                 domain->num_domains, domain->num_units);

#ifdef DART_ENABLE_LOGGING
    for (int gsd = 0; gsd < num_subdomain_tags; gsd++) {
      DART_LOG_TRACE("dart__base__locality__domain__filter_subdomains: "
                     "subdomain_tags[%d]: %s",
                     gsd, subdomain_tags[gsd]);
    }
#endif

  if (is_unit_scope) {
    return DART_OK;
  }

  for (int sd = 0; sd < domain->num_domains; sd++) {
    /* ---------------------------------------------------------------- *
     * Selection predicate is just this block.                          *
     * Could use functors to allow arbitrary selection functions.       *
     *                                                                  */
    char * subdomain_tag     = domain->domains[sd].domain_tag;
    size_t subdomain_tag_len = strlen(subdomain_tag);
    matched = 0;
    for (int dt = 0; dt < num_subdomain_tags; dt++) {
      size_t filter_tag_len    = strlen(subdomain_tags[dt]);
      size_t common_prefix_len = dart__base__strcommonprefix(
                                   subdomain_tag, subdomain_tags[dt], NULL);
      /* When removing matches: Match domains with full domain tag filter in
       * prefix, e.g. ".0.1" matches ".0.1.0"
       * -> minimum match length is length of filter tag
       * When selecting matches: Match domain tags that are fully included
       * in filter tag
       * -> minimum match length is length of subdomain:
       */
      size_t min_tag_match_len = (remove_matches == 1)
                                 ? filter_tag_len
                                 : subdomain_tag_len;

      if (common_prefix_len >= min_tag_match_len) {
        matched = 1;
        break;
      }
    }
    /*                                                                   *
     * ----------------------------------------------------------------- */

    if (matched == remove_matches) {
      continue;
    }
    DART_LOG_TRACE("dart__base__locality__domain__filter_subdomains : "
                   "  --v  subdomain[%d] = %s matched", sd, subdomain_tag);

    if (subdomain_idx != sd) {
      memcpy(domain->domains + subdomain_idx,
             domain->domains + sd,
             sizeof(dart_domain_locality_t));
      domain->domains[subdomain_idx].relative_index = subdomain_idx;
    }

    ret = dart__base__locality__domain__filter_subdomains(
            domain->domains + subdomain_idx,
            subdomain_tags,
            num_subdomain_tags,
            remove_matches);
    if (ret != DART_OK) {
      return ret;
    }

    DART_LOG_TRACE("dart__base__locality__domain__filter_subdomains : "
                   "  --^  subdomain[%d] = %s: domains: %d, units: %d", sd,
                   domain->domains[subdomain_idx].domain_tag,
                   domain->domains[subdomain_idx].num_domains,
                   domain->domains[subdomain_idx].num_units);

    /* Collect units and domains bottom-up after maximum recursion
     * depth has been reached:
     */
    if (domain->domains[subdomain_idx].num_units > 0) {
      memcpy(domain->unit_ids + unit_idx,
             domain->domains[subdomain_idx].unit_ids,
             domain->domains[subdomain_idx].num_units *
               sizeof(dart_unit_t));
      unit_idx += domain->domains[subdomain_idx].num_units;
    }

    // num_numa += domain->domains[subdomain_idx].hwinfo.num_numa;
    subdomain_idx++;
  }

  /*
   * Bottom-up accumulation of units and domains:
   */
  DART_LOG_TRACE("dart__base__locality__domain__filter_subdomains : "
                 "--> collected in %s: domains: %d, units: %d",
                 domain->domain_tag, subdomain_idx, unit_idx);

  // domain->hwinfo.num_numa = num_numa;

  if (NULL != domain->unit_ids) {
    if (domain->num_units != unit_idx) {
      dart_global_unit_t * tmp =
        realloc(domain->unit_ids, unit_idx * sizeof(dart_global_unit_t));
      if (unit_idx == 0) {
        domain->unit_ids = NULL;
      } else if (tmp != NULL) {
        domain->unit_ids = tmp;
      }
      domain->num_units = unit_idx;
    }
  }
  if (NULL != domain->domains) {
    if (domain->num_domains != subdomain_idx) {
      dart_domain_locality_t * tmp =
        realloc(domain->domains,
                subdomain_idx * sizeof(dart_domain_locality_t));
      if (subdomain_idx == 0) {
        domain->domains = NULL;
      } else if (tmp != NULL) {
        domain->domains = tmp;
      }
      domain->num_domains = subdomain_idx;
    }
  }
  DART_LOG_TRACE("dart__base__locality__domain__filter_subdomains >");
  return DART_OK;
}

dart_ret_t dart__base__locality__domain__create_subdomains(
  dart_domain_locality_t         * global_domain,
  dart_host_topology_t           * host_topology,
  dart_unit_mapping_t            * unit_mapping)
{
  /* Iterate node domains in given root domain: */
  int num_nodes;
  DART_ASSERT_RETURNS(
    dart__base__host_topology__num_nodes(
      host_topology, &num_nodes),
    DART_OK);
  DART_LOG_TRACE("dart__base__locality__domain__create_node_subdomains: "
                 "num_nodes:%d", num_nodes);

  /* Child domains of root are at node level: */
  global_domain->num_cores        = 0;
  global_domain->num_domains      = num_nodes;
  global_domain->scope            = DART_LOCALITY_SCOPE_GLOBAL;
  global_domain->level            = 0;
  global_domain->shared_mem_bytes = 0;
  global_domain->global_index     = 0;
  global_domain->relative_index   = 0;
  global_domain->domains          = malloc(num_nodes *
                                           sizeof(dart_domain_locality_t));
  strcpy(global_domain->domain_tag, ".");

  for (int n = 0; n < num_nodes; n++) {
    dart_domain_locality_t * node_domain = &global_domain->domains[n];
    dart__base__locality__domain__init(node_domain);

    node_domain->scope            = DART_LOCALITY_SCOPE_NODE;
    node_domain->level            = global_domain->level + 1;
    node_domain->shared_mem_bytes = -1;
    node_domain->global_index     = n;
    node_domain->relative_index   = n;
    node_domain->parent           = global_domain;
    node_domain->team             = global_domain->team;
    node_domain->num_units        = 0;
    sprintf(node_domain->domain_tag, ".%d", node_domain->relative_index);

    const char * node_hostname;
    DART_ASSERT_RETURNS(
      dart__base__host_topology__node(
        host_topology, n, &node_hostname),
      DART_OK);
    strncpy(node_domain->host, node_hostname, DART_LOCALITY_HOST_MAX_SIZE);

    DART_ASSERT_RETURNS(
      dart__base__locality__domain__create_node_subdomains(
        node_domain, host_topology, unit_mapping),
      DART_OK);

    for (int sd = 0; sd < node_domain->num_domains; sd++) {
      dart_domain_locality_t * node_subdomain = &node_domain->domains[sd];
      int node_num_units_prev  = node_domain->num_units;
      node_domain->num_units  += node_subdomain->num_units;
      node_domain->unit_ids    = realloc(node_domain->unit_ids,
                                         node_domain->num_units *
                                           sizeof(dart_global_unit_t));
      memcpy(node_domain->unit_ids + node_num_units_prev,
             node_subdomain->unit_ids,
             node_subdomain->num_units * sizeof(dart_unit_t));
    }

    /* Bottom-up recursion operations: */
    global_domain->num_cores += node_domain->num_cores;
  }
  return DART_OK;
}

dart_ret_t dart__base__locality__domain__create_node_subdomains(
  dart_domain_locality_t         * node_domain,
  dart_host_topology_t           * host_topology,
  dart_unit_mapping_t            * unit_mapping)
{
  /* Expects node_domain to be initialized. */

  DART_LOG_TRACE("dart__base__locality__domain__create_node_subdomains() "
                 "node_domain { host:%s, domain_tag:%s, num_units:%d }",
                 node_domain->host, node_domain->domain_tag,
                 node_domain->num_units);
  /*
   * TODO: Should only be performed by node leader units and the
   *       result broadcasted to units located on the node.
   */

  int num_modules;
  DART_ASSERT_RETURNS(
    dart__base__host_topology__num_node_modules(
      host_topology, node_domain->host, &num_modules),
    DART_OK);
  DART_LOG_TRACE("dart__base__locality__domain__create_node_subdomains: "
                 "node_hostname:%s num_modules:%d",
                 node_domain->host, num_modules);

  node_domain->num_domains = num_modules;
  node_domain->domains     = malloc(num_modules *
                                    sizeof(dart_domain_locality_t));

  int sum_module_cores = 0;
  for (int m = 0; m < num_modules; m++) {
    dart_domain_locality_t * module_domain = &node_domain->domains[m];
    dart__base__locality__domain__init(module_domain);

    module_domain->scope            = DART_LOCALITY_SCOPE_MODULE;
    module_domain->level            = node_domain->level + 1;
    module_domain->shared_mem_bytes = -1;
    module_domain->global_index     = m;
    module_domain->relative_index   = m;
    module_domain->parent           = node_domain;
    module_domain->team             = node_domain->team;
    sprintf(module_domain->domain_tag, "%s.%d",
            node_domain->domain_tag,
            module_domain->relative_index);

    const char * module_hostname;
    DART_ASSERT_RETURNS(
      dart__base__host_topology__node_module(
        host_topology, node_domain->host, m, &module_hostname),
      DART_OK);
    DART_LOG_TRACE("dart__base__locality__domain__create_node_subdomains: "
                   "module_index:%d module_hostname:%s",
                   m, module_hostname);
    strncpy(module_domain->host, module_hostname,
            DART_LOCALITY_HOST_MAX_SIZE);

    const dart_global_unit_t * module_unit_ids;
    const int                * module_numa_ids;
    int                        module_num_numa;
    DART_ASSERT_RETURNS(
      dart__base__host_topology__host_domain(
        host_topology, module_domain->host,
        &module_unit_ids, &module_domain->num_units,
        &module_numa_ids, &module_num_numa),
      DART_OK);

    module_domain->unit_ids =
      malloc(module_domain->num_units * sizeof(dart_unit_t));
    memcpy(module_domain->unit_ids, module_unit_ids,
           module_domain->num_units * sizeof(dart_unit_t));

    DART_ASSERT_RETURNS(
      dart__base__locality__domain__create_module_subdomains(
        module_domain, host_topology, unit_mapping, 0),
      DART_OK);

    /* Bottom-up recursion operations here */

    sum_module_cores += module_domain->num_cores;
  }
  node_domain->num_cores = sum_module_cores;

  DART_LOG_TRACE("dart__base__locality__domain__create_node_subdomains >");
  return DART_OK;
}

dart_ret_t dart__base__locality__domain__create_module_subdomains(
  dart_domain_locality_t         * module_domain,
  dart_host_topology_t           * host_topology,
  dart_unit_mapping_t            * unit_mapping,
  int                              module_scope_level)
{
  DART_LOG_TRACE("dart__base__locality__domain__create_module_subdomains() "
                 "module_scope_level:%d module_domain { "
                 "host:%s, domain_tag:%s, num_units:%d, global_index:%d }",
                 module_scope_level,
                 module_domain->host, module_domain->domain_tag,
                 module_domain->num_units, module_domain->global_index);

  DART_LOG_TRACE_ARRAY(
    "dart__base__locality__domain__create_module_subdomains", "%d",
    module_domain->unit_ids, module_domain->num_units);
  /*
   * NOTE: Locality scopes may be heterogeneous but are expected
   *       to be homogeneous within a module domain.
   *       For example, this would be a valid use case:
   *
   *       module[0] { unit[0]: [CORE,CACHE,PACKAGE,NUMA],
   *                   unit[1]: [CORE,CACHE,PACKAGE,NUMA] }
   *       module[1] { unit[2]: [CORE,CACHE,CACHE,CACHE,NUMA],
   *                   unit[2]: [CORE,CACHE,CACHE,CACHE,NUMA] }
   */
  if (module_domain->num_units < 1) {
    module_domain->num_units = 0;
    module_domain->unit_ids  = NULL;
    DART_LOG_TRACE(
      "dart__base__locality__domain__create_module_subdomains > no units");
    return DART_OK;
  }

  /* Collect scope lists of all units at the given module, converting:
   *
   *    u[0].scopes: [CORE:0, CACHE:4, CACHE:0, NUMA:0]
   *    u[1].scopes: [CORE:1, CACHE:5, CACHE:0, NUMA:0]
   *    u[2].scopes: [CORE:2, CACHE:6, CACHE:1, NUMA:0]
   *    u[3].scopes: [CORE:3, CACHE:7, CACHE:1, NUMA:0]
   *
   * to transposed structure:
   *
   *    level[0]: { scope:NUMA,  gids:    [       0       ],
   *                             sub_gids:[[  0   ,   1  ]]   }
   *
   *    level[1]: { scope:CACHE, gids:    [   0,      1   ],
   *                             sub_gids:[[4 , 5],[6 , 7]]   }
   *
   *    level[2]: { scope:CACHE, gids:    [ 4,  5,  6,  7 ],
   *                             sub_gids:[[0],[1],[2],[3]]   }
   *
   * such that subdomains of a domain with global index G are referenced
   * in sub_gids[G].
   */

  /* Obtain array of distinct global indices from unit scopes at current
   * level:
   */

  dart_locality_scope_t module_scopes[DART_LOCALITY_MAX_DOMAIN_SCOPES];

  int num_scopes = 0;
  dart_unit_locality_t * module_leader_unit_loc;
  dart_team_unit_t unit_id;
  DART_ASSERT_RETURNS(
    dart_team_unit_g2l(module_domain->team, module_domain->unit_ids[0],
                       &unit_id),
    DART_OK);
  DART_LOG_TRACE(
    "dart__base__locality__domain__create_module_subdomains: "
    "module_domain->unit_ids[0]: %d -> local: %d",
    module_domain->unit_ids[0].id, unit_id.id);

  DART_ASSERT_RETURNS(
    dart__base__unit_locality__at(
      unit_mapping, unit_id, &module_leader_unit_loc),
    DART_OK);
  num_scopes = module_leader_unit_loc->hwinfo.num_scopes;

  DART_ASSERT(num_scopes > 0);

  if (module_scope_level == 0) {
    /* At module level, get number of cores from leader unit: */
    int num_module_cores     = module_leader_unit_loc->hwinfo.num_cores;
    module_domain->num_cores = num_module_cores;
  } else {
  }

  for (int s = 0; s < num_scopes; s++) {
    module_scopes[s] = module_leader_unit_loc->hwinfo.scopes[s].scope;
  }
  DART_LOG_TRACE_ARRAY(
    "dart__base__locality__domain__create_module_subdomains", "%d",
    module_scopes, num_scopes);

  int subdomain_gid_idx = num_scopes - (module_scope_level + 1);

#if defined(DART_ENABLE_LOGGING)
  dart_locality_scope_t subdomain_scope = module_scopes[subdomain_gid_idx];
  DART_LOG_TRACE("dart__base__locality__domain__create_module_subdomains: "
                 "-- module_scope_level:%d subdomain_gid_idx:%d "
                 "-> subdomain_scope:%d",
                 module_scope_level, subdomain_gid_idx, subdomain_scope);
#endif

  /* Array of the global indices of the current module subdomains.
   * Maximum number of global indices, including duplicates, is number of
   * units:
   */
  int module_subdomain_gids[module_domain->num_units];

  /* Copy global indices from scopes list of all units U at the module
   * domain to module_subdomain_gids[U]:
   */
  int gid_idx = 0;
  for (int u_idx = 0; u_idx < module_domain->num_units; u_idx++) {
    dart_global_unit_t unit_gid = module_domain->unit_ids[u_idx];
    dart_team_unit_t   unit_lid;
    DART_ASSERT_RETURNS(
      dart_team_unit_g2l(module_domain->team, unit_gid, &unit_lid),
      DART_OK);

    dart_unit_locality_t * module_unit_loc;
    DART_ASSERT_RETURNS(
      dart__base__unit_locality__at(
        unit_mapping, unit_lid, &module_unit_loc),
      DART_OK);
    dart_hwinfo_t * unit_hwinfo = &module_unit_loc->hwinfo;

    int unit_level_gid = unit_hwinfo->scopes[subdomain_gid_idx+1].index;
    int unit_sub_gid   = -1;
    if (subdomain_gid_idx >= 0) {
      unit_sub_gid = unit_hwinfo->scopes[subdomain_gid_idx].index;
    }
    DART_LOG_TRACE(
      "dart__base__locality__domain__create_module_subdomains: ---- "
      "module_domain.unit_ids[%d] => unit:%d sub_gid:%d level_gid:%d "
      "module_domain.global_index:%d",
      u_idx, unit_lid, unit_sub_gid, unit_level_gid,
      module_domain->global_index);
    /* Ignore units that are not contained in current module domain: */
    if (module_scope_level == 0 ||
        unit_level_gid == module_domain->global_index) {
      module_subdomain_gids[gid_idx] = unit_sub_gid;
      DART_LOG_TRACE(
        "dart__base__locality__domain__create_module_subdomains: ---- "
        "level:%d unit:%d module_subdomain_gids[%d]:%d",
        module_scope_level, u_idx, gid_idx, module_subdomain_gids[gid_idx]);
      gid_idx++;
    }
  }
  int num_module_units = gid_idx;
  /* Sorts values in array module_subdomain_gids such that the first
   * num_subdomain elements contain its distinct values: */
  int num_subdomains   = dart__base__intsunique(
                           module_subdomain_gids, num_module_units);

  module_domain->num_domains = num_subdomains;
  module_domain->domains     = malloc(module_domain->num_domains *
                                      sizeof(dart_domain_locality_t));

  DART_LOG_TRACE_ARRAY(
    "dart__base__locality__domain__create_module_subdomains", "%d",
    module_subdomain_gids, num_subdomains);

  DART_LOG_TRACE("dart__base__locality__domain__create_module_subdomains: "
                 "module_domain.num_domains:%d, module_domain.num_units:%d "
                 " == %d ?",
                 module_domain->num_domains, module_domain->num_units,
                 num_module_units);

  for (int sd = 0; sd < module_domain->num_domains; sd++) {

    DART_LOG_TRACE("dart__base__locality__domain__create_module_subdomains: "
                   "module subdomain index:%d / num_domains:%d",
                   sd, module_domain->num_domains);

    dart_domain_locality_t * subdomain = &module_domain->domains[sd];
    dart__base__locality__domain__init(subdomain);

    subdomain->level          = module_domain->level + 1;
    subdomain->scope          = module_scopes[subdomain_gid_idx];
    subdomain->relative_index = sd;
    subdomain->global_index   = module_subdomain_gids[sd];
    subdomain->parent         = module_domain;
    subdomain->team           = module_domain->team;

    /* Set module subdomain tag: */
    sprintf(subdomain->domain_tag, "%s.%d",
            module_domain->domain_tag,
            subdomain->relative_index);

    DART_LOG_TRACE("dart__base__locality__domain__create_module_subdomains: "
                   "-- module->domains[%d].domain_tag:%s",
                   sd, subdomain->domain_tag);

    /* Set module subdomain hostname: */
    const char * module_subdomain_host;
    if (dart__base__host_topology__node_module(
          host_topology, module_domain->host, sd,
          &module_subdomain_host)
        == DART_OK) {
      strncpy(subdomain->host, module_subdomain_host,
              DART_LOCALITY_HOST_MAX_SIZE);
    } else {
      strncpy(subdomain->host, module_domain->host,
              DART_LOCALITY_HOST_MAX_SIZE);
    }
    DART_LOG_TRACE("dart__base__locality__domain__create_module_subdomains: "
                   "-- module->domains[%d].host:%s",
                   sd, subdomain->host);

    /* Set module subdomain units. Filter units in module domain by
     * global index of the subdomain of this iteration: */
    subdomain->num_units = 0;
    subdomain->unit_ids  = malloc(module_domain->num_units *
                                         sizeof(dart_unit_t));
    for (int u_idx = 0; u_idx < module_domain->num_units; u_idx++) {
      dart_global_unit_t unit_gid = module_domain->unit_ids[u_idx];
      dart_team_unit_t   unit_lid;
      DART_ASSERT_RETURNS(
        dart_team_unit_g2l(module_domain->team, unit_gid, &unit_lid),
        DART_OK);

      dart_unit_locality_t * unit_loc;
      DART_ASSERT_RETURNS(
        dart__base__unit_locality__at(
          unit_mapping, unit_lid, &unit_loc),
        DART_OK);
      DART_LOG_TRACE(
        "dart__base__locality__domain__create_module_subdomains: ---- "
        "module_unit[%d](= unit:%d).scopes[%d].index:%d =?= "
        "subdomain.global_index:%d",
        u_idx, unit_lid, subdomain_gid_idx,
        unit_loc->hwinfo.scopes[subdomain_gid_idx].index,
        subdomain->global_index);

      if (unit_loc->hwinfo.scopes[subdomain_gid_idx].index ==
          subdomain->global_index) {
        subdomain->unit_ids[subdomain->num_units] = unit_gid;
        subdomain->num_units++;
      }
    }
    DART_LOG_TRACE("dart__base__locality__domain__create_module_subdomains: "
                   "-- module->domains[%d].num_units:%d",
                   sd, subdomain->num_units);

    subdomain->unit_ids = realloc(subdomain->unit_ids,
                                  subdomain->num_units * sizeof(dart_unit_t));
    DART_ASSERT(NULL != subdomain->unit_ids);

    /* Number of units in subdomain is set at this point.
     * Below module level, a module subdomain's number of affine cores is:
     */
    int balanced_cores_per_subdomain = module_domain->num_cores /
                                       module_domain->num_units;
    subdomain->num_cores             = balanced_cores_per_subdomain *
                                       subdomain->num_units;

    if (subdomain->num_cores < 1) {
      subdomain->num_cores = 1;
    }

    if (subdomain_gid_idx <= 0) {

      /* Reached CORE scope: */
      DART_LOG_TRACE(
        "dart__base__locality__domain__create_module_subdomains: "
        "reached CORE scope (num_units:%d)",
        subdomain->num_units);

      /* TODO: Travis might map multiple MPI processes to the same
       *       physical core.
       */
      for (int u_idx = 0; u_idx < subdomain->num_units; u_idx++) {
        dart_global_unit_t unit_gid = subdomain->unit_ids[u_idx];
        dart_team_unit_t   unit_lid;
        DART_ASSERT_RETURNS(
          dart_team_unit_g2l(subdomain->team, unit_gid, &unit_lid),
          DART_OK);

        DART_LOG_TRACE(
          "dart__base__locality__domain__create_module_subdomains: "
          "reached CORE scope (num_units:%d), setting domain tag for "
          "unit_lid:%d to %s",
          subdomain->num_units, unit_lid, subdomain->domain_tag);

        dart_unit_locality_t * unit_loc;
        DART_ASSERT_RETURNS(
          dart__base__unit_locality__at(
            unit_mapping, unit_lid, &unit_loc),
          DART_OK);

        strncpy(unit_loc->domain_tag, subdomain->domain_tag,
                DART_LOCALITY_DOMAIN_TAG_MAX_SIZE);
        unit_loc->hwinfo.num_cores = subdomain->num_cores /
                                     subdomain->num_units;
        if (unit_loc->hwinfo.num_cores < 1) {
          unit_loc->hwinfo.num_cores = 1;
        }
      }
    } else {
      /* Recurse to next scope level in the module domain: */
      DART_ASSERT_RETURNS(
        dart__base__locality__domain__create_module_subdomains(
          subdomain, host_topology, unit_mapping,
          module_scope_level+1),
        DART_OK);
    }
  }

  DART_LOG_TRACE("dart__base__locality__domain__create_module_subdomains >");
  return DART_OK;
}

