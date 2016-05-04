/**
 * \file dash/dart/base/internal/domain_locality.c
 */
#include <dash/dart/base/macro.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/base/locality.h>
#include <dash/dart/base/assert.h>
#include <dash/dart/base/hwinfo.h>

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

dart_ret_t dart__base__locality__create_global_subdomain(
  dart_host_topology_t   * host_topology,
  dart_unit_mapping_t    * unit_mapping,
  dart_domain_locality_t * global_domain,
  dart_domain_locality_t * subdomain,
  int                      rel_idx);

dart_ret_t dart__base__locality__create_node_subdomain(
  dart_host_topology_t   * host_topology,
  dart_unit_mapping_t    * unit_mapping,
  dart_domain_locality_t * node_domain,
  dart_domain_locality_t * subdomain,
  int                      rel_idx);

dart_ret_t dart__base__locality__create_module_subdomain(
  dart_host_topology_t   * host_topology,
  dart_unit_mapping_t    * unit_mapping,
  dart_domain_locality_t * module_domain,
  dart_domain_locality_t * subdomain,
  int                      rel_idx);

dart_ret_t dart__base__locality__create_numa_subdomain(
  dart_host_topology_t   * host_topology,
  dart_unit_mapping_t    * unit_mapping,
  dart_domain_locality_t * numa_domain,
  dart_domain_locality_t * subdomain,
  int                      rel_idx);

/* ======================================================================== *
 * Internal Functions                                                       *
 * ======================================================================== */

dart_ret_t dart__base__locality__domain_locality_init(
  dart_domain_locality_t * loc)
{
  DART_LOG_TRACE("dart__base__locality__domain_locality_init() loc: %p", loc);
  if (loc == NULL) {
    DART_LOG_ERROR("dart__base__locality__domain_locality_init ! null");
    return DART_ERR_INVAL;
  }
  loc->domain_tag[0] = '\0';
  loc->host[0]       = '\0';
  loc->scope         = DART_LOCALITY_SCOPE_UNDEFINED;
  loc->level         =  0;
  loc->parent        = NULL;
  loc->num_domains   =  0;
  loc->domains       = NULL;
  loc->num_nodes     = -1;
  loc->num_units     = -1;
  DART_LOG_TRACE("dart__base__locality__domain_locality_init >");
  return DART_OK;
}

dart_ret_t dart__base__locality__domain_delete(
  dart_domain_locality_t * domain)
{
  if (domain == NULL) {
    return DART_OK;
  }
  /* deallocate child nodes in depth-first recursion: */
  int num_child_nodes = domain->num_domains;
  for (int subdom_idx = 0; subdom_idx < num_child_nodes; ++subdom_idx) {
    if (domain->num_domains <= subdom_idx) {
      /* child index out of range: */
      return DART_ERR_INVAL;
    }
    if (domain->domains == NULL) {
      /* child nodes field is unspecified: */
      return DART_ERR_INVAL;
    }
    dart__base__locality__domain_delete(
      domain->domains + subdom_idx);
  }
  /* deallocate node itself: */
  free(domain->domains);
  domain->domains     = NULL;
  domain->num_domains = 0;

  return DART_OK;
}

dart_ret_t dart__base__locality__create_subdomains(
  dart_domain_locality_t * domain,
  dart_host_topology_t   * host_topology,
  dart_unit_mapping_t    * unit_mapping)
{
  DART_LOG_DEBUG("dart__base__locality__create_subdomains() "
                 "parent: %p scope: %d level: %d",
                 domain, domain->scope, domain->level);
  /*
   * First step: determine the number of sub-domains and their scope.
   */
  dart_locality_scope_t sub_scope;
  sub_scope           = DART_LOCALITY_SCOPE_UNDEFINED;
  const char * module_hostname;
  switch (domain->scope) {
    case DART_LOCALITY_SCOPE_UNDEFINED:
      DART_LOG_ERROR("dart__base__locality__create_subdomains ! "
                     "locality scope undefined");
      return DART_ERR_INVAL;
    case DART_LOCALITY_SCOPE_GLOBAL:
      DART_ASSERT_RETURNS(
        dart__base__host_topology__num_nodes(
          host_topology,
          &domain->num_domains),
        DART_OK);
      sub_scope           = DART_LOCALITY_SCOPE_NODE;
      break;
    case DART_LOCALITY_SCOPE_NODE:
      DART_ASSERT_RETURNS(
        dart__base__host_topology__num_node_modules(
          host_topology,
          domain->host,
          &domain->num_domains),
        DART_OK);
      sub_scope           = DART_LOCALITY_SCOPE_MODULE;
      break;
    case DART_LOCALITY_SCOPE_MODULE:
      domain->num_domains = domain->hwinfo.num_numa;
      sub_scope           = DART_LOCALITY_SCOPE_NUMA;
      DART_ASSERT_RETURNS(
        dart__base__host_topology__node_module(
          host_topology,
          domain->host,
          domain->relative_index,
          &module_hostname),
        DART_OK);
      /* Requires to resolve number of units in this module domain.
       * Cannot use local hwinfo, number of cores could refer to non-local
       * module. Use host topology instead. */
      dart_unit_t * module_units;
      DART_ASSERT_RETURNS(
        dart__base__host_topology__module_units(
          host_topology,
          module_hostname,
          &module_units,
          &domain->num_units),
        DART_OK);
      break;
    case DART_LOCALITY_SCOPE_NUMA:
      domain->num_domains = domain->num_units;
      sub_scope           = DART_LOCALITY_SCOPE_CORE;
      break;
    default:
      domain->num_domains = 0;
      break;
  }
  DART_LOG_TRACE("dart__base__locality__create_subdomains: subdomains: %d",
                 domain->num_domains);
  if (domain->num_domains == 0) {
    domain->domains = NULL;
    DART_LOG_DEBUG("dart__base__locality__create_subdomains > "
                   "domain: %p - scope: %d level: %d "
                   "subdomains: %d domain(%s) - final",
                   domain, domain->scope, domain->level,
                   domain->num_domains, domain->domain_tag);
    return DART_OK;
  }
  /* Allocate subdomains: */
  domain->domains = (dart_domain_locality_t *)(
                       malloc(domain->num_domains *
                              sizeof(dart_domain_locality_t)));
  /*
   * Second step: determine the subdomain capacities and distribute
   * domain elements like units and cores.
   */
  for (int rel_idx = 0; rel_idx < domain->num_domains; ++rel_idx) {
    DART_LOG_TRACE("dart__base__locality__create_subdomains: "
                   "initialize, level: %d, subdomain %d of %d",
                   domain->level + 1, rel_idx, domain->num_domains);

    dart_domain_locality_t * subdomain = domain->domains + rel_idx;
    dart__base__locality__domain_locality_init(subdomain);

    /* Initialize hwinfo from parent as most properties are identical: */
    subdomain->hwinfo         = domain->hwinfo;
    subdomain->parent         = domain;
    subdomain->scope          = sub_scope;
    subdomain->relative_index = rel_idx;
    subdomain->level          = domain->level + 1;
    subdomain->node_id        = domain->node_id;
    /* set domain tag of subdomain: */
    strncpy(subdomain->host, domain->host, DART_LOCALITY_HOST_MAX_SIZE);
    int base_tag_len = 0;
    if (domain->level > 0) {
      base_tag_len = sprintf(subdomain->domain_tag, "%s",
                             domain->domain_tag);
    }
    /* append the subdomain tag part to subdomain tag, e.g. ".0.1": */
    sprintf(subdomain->domain_tag + base_tag_len, ".%d", rel_idx);

    switch (domain->scope) {
      case DART_LOCALITY_SCOPE_GLOBAL:
        dart__base__locality__create_global_subdomain(
          host_topology, unit_mapping, domain, subdomain, rel_idx);
        break;
      case DART_LOCALITY_SCOPE_NODE:
        dart__base__locality__create_node_subdomain(
          host_topology, unit_mapping, domain, subdomain, rel_idx);
        break;
      case DART_LOCALITY_SCOPE_MODULE:
        dart__base__locality__create_module_subdomain(
          host_topology, unit_mapping, domain, subdomain, rel_idx);
        break;
      case DART_LOCALITY_SCOPE_NUMA:
        dart__base__locality__create_numa_subdomain(
          host_topology, unit_mapping, domain, subdomain, rel_idx);
        break;
      case DART_LOCALITY_SCOPE_CORE:
        subdomain->hwinfo.num_numa    = 1;
        subdomain->hwinfo.num_cores   = 1;
        subdomain->num_nodes          = 1;
        subdomain->num_units          = 1;
        subdomain->unit_ids           = malloc(sizeof(dart_unit_t));
        subdomain->unit_ids[0]        = domain->unit_ids[rel_idx];
        break;
      default:
        break;
    }
    /* recursively initialize subdomains: */
    DART_ASSERT_RETURNS(
        dart__base__locality__create_subdomains(
          subdomain, host_topology, unit_mapping),
        DART_OK);
  }
  DART_LOG_DEBUG("dart__base__locality__create_subdomains >");
  return DART_OK;
}

/* ======================================================================== *
 * Private Functions: Implementations                                       *
 * ======================================================================== */

dart_ret_t dart__base__locality__create_global_subdomain(
  dart_host_topology_t   * host_topology,
  dart_unit_mapping_t    * unit_mapping,
  dart_domain_locality_t * global_domain,
  dart_domain_locality_t * subdomain,
  int                      rel_idx)
{
  /* Loop iterates on nodes. Partitioning is trivial, split into one
   * node per sub-domain. */
  dart__unused(global_domain);
  dart__unused(unit_mapping);

  DART_LOG_TRACE("dart__base__locality__create_subdomains: "
                 "== SPLIT GLOBAL ==");
  DART_LOG_TRACE("dart__base__locality__create_subdomains: == %d of %d",
                 rel_idx, global_domain->num_domains);
  /* units in node at relative index: */
  const char * node_hostname;
  DART_ASSERT_RETURNS(
    dart__base__host_topology__node(
      host_topology,
      rel_idx,
      &node_hostname),
    DART_OK);
  DART_LOG_TRACE("dart__base__locality__create_subdomains: host: %s",
                 node_hostname);
  strncpy(subdomain->host, node_hostname, DART_LOCALITY_HOST_MAX_SIZE);

  dart_unit_t * node_unit_ids;
  int           num_node_units;
  DART_ASSERT_RETURNS(
    dart__base__host_topology__node_units(
      host_topology,
      node_hostname,
      &node_unit_ids,
      &num_node_units),
    DART_OK);
  /* relative sub-domain index at global scope is node id: */
  subdomain->node_id    = rel_idx;
  subdomain->num_nodes  = 1;
  subdomain->num_units  = num_node_units;
  subdomain->unit_ids   = malloc(num_node_units * sizeof(dart_unit_t));
  for (int nu = 0; nu < num_node_units; ++nu) {
    subdomain->unit_ids[nu] = node_unit_ids[nu];
  }
  return DART_OK;
}

dart_ret_t dart__base__locality__create_node_subdomain(
  dart_host_topology_t   * host_topology,
  dart_unit_mapping_t    * unit_mapping,
  dart_domain_locality_t * node_domain,
  dart_domain_locality_t * subdomain,
  int                      rel_idx)
{
  /* Loop splits into processing modules.
   * Usually there is only one module (the host system), otherwise
   * partitioning is heterogenous. */
  dart__unused(unit_mapping);

  DART_LOG_TRACE("dart__base__locality__create_subdomains: "
                 "== SPLIT NODE ==");
  DART_LOG_TRACE("dart__base__locality__create_subdomains: == %d of %d",
                 rel_idx, node_domain->num_domains);
  const char * module_hostname;
  DART_ASSERT_RETURNS(
    dart__base__host_topology__node_module(
      host_topology,
      node_domain->host,
      rel_idx,
      &module_hostname),
    DART_OK);
  DART_LOG_TRACE("dart__base__locality__create_subdomains: host: %s",
                 module_hostname);
  /* set subdomain hostname to module's hostname: */
  strncpy(subdomain->host, module_hostname,
          DART_LOCALITY_HOST_MAX_SIZE);
  strncpy(subdomain->host, module_hostname, DART_LOCALITY_HOST_MAX_SIZE);

  dart_unit_t * module_unit_ids;
  int           num_module_units;
  DART_ASSERT_RETURNS(
    dart__base__host_topology__module_units(
      host_topology,
      module_hostname,
      &module_unit_ids,
      &num_module_units),
    DART_OK);
  subdomain->num_nodes = 1;
  subdomain->num_units = num_module_units;
  subdomain->unit_ids  = malloc(num_module_units * sizeof(dart_unit_t));
  for (int nu = 0; nu < num_module_units; ++nu) {
    subdomain->unit_ids[nu] = module_unit_ids[nu];
  }
  return DART_OK;
}

dart_ret_t dart__base__locality__create_module_subdomain(
  dart_host_topology_t   * host_topology,
  dart_unit_mapping_t    * unit_mapping,
  dart_domain_locality_t * module_domain,
  dart_domain_locality_t * subdomain,
  int                      rel_idx)
{
  /* Loop splits into NUMA nodes. */
  DART_LOG_TRACE("dart__base__locality__create_subdomains: "
                 "== SPLIT MODULE ==");
  DART_LOG_TRACE("dart__base__locality__create_subdomains: == %d of %d",
                 rel_idx, module_domain->num_domains);

  size_t        num_numa_units  = 0;
  char *        module_hostname = module_domain->host;
  /* set subdomain hostname to module's hostname: */
  strncpy(subdomain->host, module_hostname,
          DART_LOCALITY_HOST_MAX_SIZE);
  int           num_module_units;
  dart_unit_t * module_unit_ids;
  DART_ASSERT_RETURNS(
    dart__base__host_topology__module_units(
        host_topology,
        module_hostname,
        &module_unit_ids,
        &num_module_units),
    DART_OK);
  /* Assign units in the node that have the NUMA domain's NUMA id: */
  /* First pass: Count number of units in NUMA domain: */
  for (int mu = 0; mu < num_module_units; ++mu) {
    dart_unit_t module_unit_id = module_unit_ids[mu];
    /* query the unit's hw- and locality information: */
    dart_unit_locality_t * module_unit_loc;
    dart__base__unit_locality__at(
      unit_mapping, module_unit_id, &module_unit_loc);
    int module_unit_numa_id = module_unit_loc->hwinfo.numa_id;
    /* TODO: assuming that rel_idx corresponds to NUMA id: */
    DART_LOG_TRACE("dart__base__locality__create_subdomains: "
                   "unit %d numa id: %d", mu, module_unit_numa_id);
    if (module_unit_numa_id == rel_idx) {
      ++num_numa_units;
    }
  }
  subdomain->hwinfo.num_numa    = 1;
  subdomain->hwinfo.num_cores   = num_numa_units;
  subdomain->num_nodes          = 1;
  subdomain->num_units          = num_numa_units;
  subdomain->unit_ids           = malloc(subdomain->num_units *
                                         sizeof(dart_unit_t));
  /* Second pass: Assign unit ids in NUMA domain: */
  dart_unit_t numa_unit_idx = 0;
  for (int mu = 0; mu < num_module_units; ++mu) {
    dart_unit_t module_unit_id = module_unit_ids[mu];
    /* query the unit's hw- and locality information: */
    dart_unit_locality_t * module_unit_loc;
    dart__base__unit_locality__at(
      unit_mapping, module_unit_id, &module_unit_loc);
    int module_unit_numa_id = module_unit_loc->hwinfo.numa_id;
    if (module_unit_numa_id == rel_idx) {
      dart_unit_t numa_unit_id = module_unit_id;
      DART_LOG_TRACE("dart__base__locality__create_subdomains: "
                     "NUMA unit %d of %d: unit id %d",
                     numa_unit_idx, subdomain->num_units, numa_unit_id);
      subdomain->unit_ids[numa_unit_idx] = numa_unit_id;
      ++numa_unit_idx;
    }
  }
  return DART_OK;
}

dart_ret_t dart__base__locality__create_numa_subdomain(
  dart_host_topology_t   * host_topology,
  dart_unit_mapping_t    * unit_mapping,
  dart_domain_locality_t * numa_domain,
  dart_domain_locality_t * subdomain,
  int                      rel_idx)
{
  /* Loop splits into UMA segments within a NUMA domain or module.
   * Using balanced split, segments are assumed to be homogenous at
   * this level. */
  dart__unused(host_topology);

  DART_LOG_TRACE("dart__base__locality__create_subdomains: "
                 "== SPLIT NUMA ==");
  DART_LOG_TRACE("dart__base__locality__create_subdomains: == %d of %d",
                 rel_idx, numa_domain->num_domains);
  size_t num_uma_units          = numa_domain->num_units /
                                  numa_domain->num_domains;
  subdomain->num_nodes          = 1;
  subdomain->hwinfo.num_numa    = 1;
  subdomain->hwinfo.num_cores   = numa_domain->hwinfo.num_cores /
                                  numa_domain->num_domains;
  subdomain->num_units          = num_uma_units;
  subdomain->unit_ids           = malloc(subdomain->num_units *
                                         sizeof(dart_unit_t));
  for (int u = 0; u < subdomain->num_units; ++u) {
    int numa_unit_idx      = (rel_idx * num_uma_units) + u;
    dart_unit_t unit_id    = numa_domain->unit_ids[numa_unit_idx];
    subdomain->unit_ids[u] = unit_id;
    DART_LOG_TRACE("dart__base__locality__create_subdomains: "
                   "UMA unit %d of %d (NUMA unit %d): unit id %d",
                   u, subdomain->num_units, numa_unit_idx, unit_id);
    /* set host and domain tag of unit in unit locality map: */
    dart_unit_locality_t * unit_loc;
    DART_ASSERT_RETURNS(
      dart__base__unit_locality__at(unit_mapping, unit_id, &unit_loc),
      DART_OK);
    DART_LOG_TRACE("dart__base__locality__create_subdomains: "
                   "setting unit %d domain_tag: %s host: %s",
                   unit_id, subdomain->domain_tag, subdomain->host);
    strncpy(unit_loc->domain_tag, subdomain->domain_tag,
            DART_LOCALITY_DOMAIN_TAG_MAX_SIZE);
  }
  return DART_OK;
}

