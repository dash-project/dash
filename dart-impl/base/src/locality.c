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

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_locality.h>

#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sched.h>
#include <limits.h>

/* ======================================================================== *
 * Private Data                                                             *
 * ======================================================================== */

dart_host_topology_t   * dart__base__locality__host_topology_;
dart_domain_locality_t   dart__base__locality__domain_root_;

/* ======================================================================== *
 * Init / Finalize                                                          *
 * ======================================================================== */

dart_ret_t dart__base__locality__init()
{
  DART_LOG_DEBUG("dart__base__locality__init()");

  dart_hwinfo_t * hwinfo;
  DART_ASSERT_RETURNS(dart_hwinfo(&hwinfo), DART_OK);

  /* Initialize the global domain as the root entry in the locality
   * hierarchy: */
  dart__base__locality__domain_root_.scope  = DART_LOCALITY_SCOPE_GLOBAL;
  dart__base__locality__domain_root_.hwinfo = *hwinfo;

  char   hostname[DART_LOCALITY_HOST_MAX_SIZE];
  gethostname(hostname, DART_LOCALITY_HOST_MAX_SIZE);
  strncpy(dart__base__locality__domain_root_.host, hostname,
          DART_LOCALITY_HOST_MAX_SIZE);

  strncpy(dart__base__locality__domain_root_.domain_tag, ".", 1);

  size_t num_units = 0;
  DART_ASSERT_RETURNS(dart_size(&num_units), DART_OK);
  dart__base__locality__domain_root_.num_units = num_units;

  dart__base__locality__domain_root_.unit_ids =
    malloc(num_units * sizeof(dart_unit_t));
  for (int u = 0; u < num_units; ++u) {
    dart__base__locality__domain_root_.unit_ids[u] = u;
  }

  /* Exchange unit locality information between all units: */
  DART_ASSERT_RETURNS(
    dart__base__unit_locality__init(),
    DART_OK);

  /* Filter unique host names from locality information of all units.
   * Could be further optimized but only runs once during startup. */
  const int max_host_len = DART_LOCALITY_HOST_MAX_SIZE;
  DART_LOG_TRACE("dart__base__locality__init: copying host names");
  /* Copy host names of all units into array: */
  char ** hosts = malloc(sizeof(char *) * num_units);
  for (size_t u = 0; u < num_units; ++u) {
    hosts[u] = malloc(sizeof(char) * max_host_len);
    dart_unit_locality_t * ul;
    DART_ASSERT_RETURNS(dart__base__unit_locality__at(u, &ul), DART_OK);
    strncpy(hosts[u], ul->host, max_host_len);
  }

  dart_host_topology_t * topo = malloc(sizeof(dart_host_topology_t));
  DART_ASSERT_RETURNS(
    dart__base__host_topology__create(hosts, DART_TEAM_ALL, topo),
    DART_OK);
  dart__base__locality__host_topology_ = topo;
  size_t num_nodes   = dart__base__locality__host_topology_->num_nodes;
  size_t num_modules = dart__base__locality__host_topology_->num_modules;
  DART_LOG_TRACE("dart__base__locality__init: nodes:   %d", num_nodes);
  DART_LOG_TRACE("dart__base__locality__init: modules: %d", num_modules);

  dart__base__locality__domain_root_.num_nodes          = num_nodes;
  dart__base__locality__domain_root_.hwinfo.num_modules = num_modules;

#ifdef DART_ENABLE_LOGGING
  for (size_t h = 0; h < topo->num_hosts; ++h) {
    dart_node_units_t * node_units = &topo->node_units[h];
    char * hostname = topo->host_names[h];
    DART_LOG_TRACE("dart__base__locality__init: "
                   "host %s: units:%d level:%d parent:%s", hostname,
                   node_units->num_units, node_units->level,
                   node_units->parent);
    for (size_t u = 0; u < node_units->num_units; ++u) {
      DART_LOG_TRACE("dart__base__locality__init: %s unit[%d]: %d",
                     hostname, u, node_units->units[u]);
    }
  }
#endif

  /* recursively create locality information of the global domain's
   * sub-domains: */
  DART_ASSERT_RETURNS(
    dart__base__locality__create_subdomains(
      &dart__base__locality__domain_root_,
      dart__base__locality__host_topology_),
    DART_OK);

  DART_LOG_DEBUG("dart__base__locality__init >");
  return DART_OK;
}

dart_ret_t dart__base__locality__finalize()
{
  DART_LOG_DEBUG("dart__base__locality__finalize()");

  dart_ret_t ret;

  ret = dart__base__locality__domain_delete(
          &dart__base__locality__domain_root_);
  if (ret != DART_OK) {
    DART_LOG_ERROR("dart__base__locality__finalize ! "
                   "dart__base__locality__domain_delete failed: %d", ret);
    return ret;
  }

  ret = dart__base__host_topology__delete(
          dart__base__locality__host_topology_);
  if (ret != DART_OK) {
    DART_LOG_ERROR("dart__base__locality__finalize ! "
                   "dart__base__host_topology__delete failed: %d", ret);
    return ret;
  }

  DART_LOG_DEBUG("dart__base__locality__finalize >");
  return DART_OK;
}

/* ======================================================================== *
 * Domain Locality                                                          *
 * ======================================================================== */

dart_ret_t dart__base__locality__domain(
  const char              * domain_tag,
  dart_domain_locality_t ** locality)
{
  DART_LOG_DEBUG("dart__base__locality__domain() domain(%s)", domain_tag);

  dart_domain_locality_t * domain = &dart__base__locality__domain_root_;

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
      DART_LOG_ERROR("dart__base__locality__domain ! domain(%s): "
                     "subdomain at index %d in level %d is undefined",
                     domain_tag, subdomain_idx, level);
      return DART_ERR_INVAL;
    }
    if (domain->num_domains <= subdomain_idx) {
      /* child index out of range: */
      DART_LOG_ERROR("dart__base__locality__domain ! domain(%s): "
                     "subdomain index %d in level %d is out ouf bounds "
                     "(number of subdomains: %d)",
                     domain_tag, subdomain_idx, level, domain->num_domains);
      return DART_ERR_INVAL;
    }
    /* descend to child at relative index: */
    domain     = domain->domains + subdomain_idx;
    /* continue scanning of domain tag at next part: */
    dot_begin  = dot_end;
    part_begin = dot_end+1;
    level++;
  }
  *locality = domain;
  return DART_OK;
}

/* ======================================================================== *
 * Unit Locality                                                            *
 * ======================================================================== */

dart_ret_t dart__base__locality__unit(
  dart_unit_t             unit,
  dart_unit_locality_t ** locality)
{
  DART_LOG_DEBUG("dart__base__locality__unit() unit(%d)", unit);
  *locality = NULL;

  dart_unit_locality_t * uloc;
  dart_ret_t ret = dart__base__unit_locality__at(unit, &uloc);
  if (ret != DART_OK) {
    DART_LOG_ERROR("dart_unit_locality: "
                   "dart__base__locality__unit(unit:%d) failed (%d)",
                   unit, ret);
    return ret;
  }
  *locality = uloc;

  DART_LOG_DEBUG("dart__base__locality__unit > unit(%d)", unit);
  return DART_OK;
}
