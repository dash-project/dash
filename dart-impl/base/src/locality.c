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

#include <dart/impl/base/internal/papi.h>
#include <dash/dart/base/internal/unit_locality.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_locality.h>

#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <sched.h>

#ifdef DART_ENABLE_LIKWID
#  include <likwid.h>
#endif

#ifdef DART_ENABLE_HWLOC
#  include <hwloc.h>
#  include <hwloc/helper.h>
#endif

#ifdef DART_ENABLE_PAPI
#  include <papi.h>
#endif

#ifdef DART_ENABLE_NUMA
#  include <utmpx.h>
#  include <numa.h>
#endif

/* ======================================================================== *
 * Private Data                                                             *
 * ======================================================================== */

dart_domain_locality_t dart__base__locality__domain_root_;
size_t  dart__base__locality__num_hosts_;
char ** dart__base__locality__host_names_;
dart_node_units_t * dart__base__locality__node_units_;

/* ======================================================================== *
 * Private Functions                                                        *
 * ======================================================================== */

dart_ret_t dart__base__locality__create_subdomains(
  dart_domain_locality_t * loc);

dart_ret_t dart__base__locality__global_domain_new(
  dart_domain_locality_t * global_domain);

/* ======================================================================== *
 * Helpers                                                                  *
 * ======================================================================== */

static int cmpstr_(const void * p1, const void * p2) {
  return strcmp(* (char * const *) p1, * (char * const *) p2);
}

/* ======================================================================== *
 * Init / Finalize                                                          *
 * ======================================================================== */

dart_ret_t dart__base__locality__init()
{
  DART_LOG_DEBUG("dart__base__locality__init()");

  dart_ret_t ret;

  ret = dart__base__locality__global_domain_new(
          &dart__base__locality__domain_root_);
  if (ret != DART_OK) {
    DART_LOG_ERROR("dart__base__locality__init ! "
                   "dart__base__locality__global_domain_new failed: %d", ret);
    return ret;
  }

  ret = dart__base__unit_locality__init();
  if (ret != DART_OK) {
    DART_LOG_ERROR("dart__base__locality__init ! "
                   "dart__base__unit_locality__init failed: %d", ret);
    return ret;
  }
  /* Filter unique host names from locality information of all units.
   * Could be further optimized but only runs once durin startup. */
  size_t    nunits       = 0;
  const int max_host_len = DART_LOCALITY_HOST_MAX_SIZE;
  DART_ASSERT_RETURNS(dart_size(&nunits), DART_OK);
  DART_LOG_TRACE("dart__base__locality__init: copying host names");
  /* Copy host names of all units into array: */
  char ** hosts       = malloc(sizeof(char *) * nunits);
  for (size_t u = 0; u < nunits; ++u) {
    hosts[u] = malloc(sizeof(char) * max_host_len);
    dart_unit_locality_t * ul;
    DART_ASSERT_RETURNS(dart__base__unit_locality__get(u, &ul), DART_OK);
    strncpy(hosts[u], ul->host, max_host_len);
  }
  /* Sort host names to find duplicates in one pass: */
  qsort(hosts, nunits, sizeof(char*), cmpstr_);
  /* Find unique host names in array 'hosts': */
  size_t last_host_idx  = 0;
  /* Maximum number of units mapped to a single host: */
  size_t max_host_units = 0;
  /* Number of units mapped to current host: */
  size_t num_host_units = 0;
  DART_LOG_TRACE("dart__base__locality__init: filtering host names");
  for (size_t u = 0; u < nunits; ++u) {
    ++num_host_units;
    if (u == last_host_idx) { continue; }
    /* copies next differing host name to the left, like:
     *
     *     [ a a a a b b b c c c ]  last_host_index++ = 1
     *         .---------'
     *         v
     * ->  [ a b a a b b b c c c ]  last_host_index++ = 2
     *           .-----------'
     *           v
     * ->  [ a b c a b b b c c c ]  last_host_index++ = 3
     * ...
     */
    if (strcmp(hosts[u], hosts[last_host_idx]) != 0) {
      ++last_host_idx;
      strncpy(hosts[last_host_idx], hosts[u], max_host_len);
      if (num_host_units > max_host_units) {
        max_host_units = num_host_units;
      }
      num_host_units = 0;
    }
  }
  /* All entries after index last_host_ids are duplicates now: */
  size_t num_hosts = last_host_idx + 1;
  DART_LOG_TRACE("dart__base__locality__init: number of hosts: %d", num_hosts);
  DART_LOG_TRACE("dart__base__locality__init: maximum number of units "
                 "per hosts: %d", max_host_units);

  /* Map units to hosts: */
  dart__base__locality__node_units_ =
    malloc(num_hosts * sizeof(dart_node_units_t));
  for (int h = 0; h < num_hosts; ++h) {
    dart_node_units_t * node_units = &dart__base__locality__node_units_[h];
    /* allocate array with capacity of maximum units on a single host: */
    node_units->units = malloc(sizeof(dart_unit_t) * max_host_units);
    strncpy(node_units->host, hosts[h], max_host_len);
    node_units->num_units = 0;
    DART_LOG_TRACE("dart__base__locality__init: mapping units to host %d",
                   hosts[h]);
    for (size_t u = 0; u < nunits; ++u) {
      dart_unit_locality_t * ul;
      DART_ASSERT_RETURNS(dart__base__unit_locality__get(u, &ul), DART_OK);
      if (strncmp(ul->host, hosts[h], max_host_len) == 0) {
        node_units->units[node_units->num_units] = ul->unit;
        node_units->num_units++;
      }
    }
    /* shrink unit array to required capacity: */
    if (node_units->num_units < max_host_units) {
      DART_LOG_TRACE("dart__base__locality__init: shrinking node unit "
                     "array from %d to %d elements",
                     max_host_units, node_units->num_units);
      node_units->units = realloc(node_units->units, node_units->num_units);
      DART_ASSERT(node_units->units != NULL);
    }
  }
  dart__base__locality__num_hosts_  = num_hosts;
  dart__base__locality__host_names_ = (char **)(realloc(hosts, num_hosts));
  DART_ASSERT(dart__base__locality__host_names_ != NULL);

#ifdef DART_ENABLE_LOGGING
  for (int h = 0; h < num_hosts; ++h) {
    dart_node_units_t * node_units = &dart__base__locality__node_units_[h];
    char * hostname = dart__base__locality__host_names_[h];
    DART_LOG_TRACE("dart__base__locality__init: %d units mapped to host %s:",
                   node_units->num_units, hostname);
    for (int u = 0; u < node_units->num_units; ++u) {
      DART_LOG_TRACE("dart__base__locality__init: %s unit[%d]: %d",
                     hostname, u, node_units->units[u]);
    }
  }
#endif

  DART_LOG_DEBUG("dart__base__locality__init >");
  return DART_OK;
}

dart_ret_t dart__base__locality__finalize()
{
  DART_LOG_DEBUG("dart__base__locality__finalize()");

  dart_ret_t ret = dart__base__locality__domain_delete(
                     &dart__base__locality__domain_root_);
  if (ret != DART_OK) {
    DART_LOG_ERROR("dart__base__locality__finalize ! "
                   "dart__base__locality__domain_delete failed: %d", ret);
    return ret;
  }
  if (dart__base__locality__node_units_ != NULL) {
    free(dart__base__locality__node_units_);
    dart__base__locality__node_units_ = NULL;
  }
  if (dart__base__locality__host_names_ != NULL) {
    free(dart__base__locality__host_names_);
    dart__base__locality__host_names_ = NULL;
  }

  DART_LOG_DEBUG("dart__base__locality__finalize >");
  return DART_OK;
}

/* ======================================================================== *
 * Domain Locality                                                          *
 * ======================================================================== */

dart_ret_t dart__base__locality__domain_locality_init(
  dart_domain_locality_t * loc)
{
  DART_LOG_TRACE("dart__base__locality__domain_locality_init() loc: %p", loc);
  if (loc == NULL) {
    DART_LOG_ERROR("dart__base__locality__domain_locality_init ! null");
    return DART_ERR_INVAL;
  }
  loc->domain_tag[0]       = '\0';
  loc->parent              = NULL;
  loc->scope               = DART_LOCALITY_SCOPE_UNDEFINED;
  loc->level               =  0;
  loc->host[0]             = '\0';
  loc->num_domains         =  0;
  loc->domains             = NULL;
  loc->num_numa            = -1;
  loc->numa_ids            = NULL;
  loc->num_cores           = -1;
  loc->cpu_ids             = NULL;
  loc->num_nodes           = -1;
  loc->num_modules         = -1;
  loc->num_sockets         = -1;
  loc->min_cpu_mhz         = -1;
  loc->max_cpu_mhz         = -1;
  loc->min_threads         = -1;
  loc->max_threads         = -1;
  loc->cache_sizes[0]      = -1;
  loc->cache_sizes[1]      = -1;
  loc->cache_sizes[2]      = -1;
  loc->cache_line_sizes[0] = -1;
  loc->cache_line_sizes[1] = -1;
  loc->cache_line_sizes[2] = -1;
  loc->cache_shared[0]     =  0;
  loc->cache_shared[1]     =  0;
  loc->cache_shared[2]     =  0;
  DART_LOG_TRACE("dart__base__locality__domain_locality_init >");
  return DART_OK;
}

dart_ret_t dart__base__locality__set_subdomains(
  const char             * domain_tag,
  dart_domain_locality_t * subdomains,
  int                      num_subdomains)
{
  dart_domain_locality_t * domain;
  /* find node in domain tree for specified domain tag: */
  if (dart__base__locality__domain(domain_tag, &domain) != DART_OK) {
    return DART_ERR_INVAL;
  }

  /* allocate child nodes: */
  domain->num_domains = num_subdomains;
  domain->domains     = (dart_domain_locality_t *)(
                            malloc(
                              num_subdomains *
                              sizeof(dart_domain_locality_t)));
  /* initialize child nodes: */
  int sub_level = domain->level + 1;
  for (int subdom_idx = 0; subdom_idx < num_subdomains; ++subdom_idx) {
    domain->domains[subdom_idx]             = subdomains[subdom_idx];
    domain->domains[subdom_idx].level       = sub_level;
    domain->domains[subdom_idx].num_domains = 0;
    domain->domains[subdom_idx].domains     = NULL;
  }

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

dart_ret_t dart__base__locality__global_domain_new(
  dart_domain_locality_t * loc)
{
  DART_LOG_DEBUG("dart__base__locality__global_domain_new() loc: %p", loc);
  dart_ret_t ret;

  /* initialize domain descriptor: */
  ret = dart__base__locality__domain_locality_init(loc);
  if (ret != DART_OK) {
    DART_LOG_ERROR("dart__base__locality__global_domain_new ! "
                   "dart__base__locality__domain_locality_init failed: %d",
                   ret);
    return ret;
  }

  /* set domain tag: */
  strncpy(loc->domain_tag, ".", DART_LOCALITY_DOMAIN_TAG_MAX_SIZE);
  loc->domain_tag[DART_LOCALITY_DOMAIN_TAG_MAX_SIZE-1] = '\0';

  /* set domain scope and level: */
  loc->scope = DART_LOCALITY_SCOPE_GLOBAL;
  loc->level = 0;

  /* set domain host name: */
  gethostname(loc->host, DART_LOCALITY_HOST_MAX_SIZE);

#ifdef DART_ENABLE_LIKWID
  DART_LOG_TRACE("dart__base__locality__global_domain_new: using likwid");

  /*
   * see likwid API documentation:
   * https://rrze-hpc.github.io/likwid/Doxygen/C-likwidAPI-code.html
   */
  int likwid_ret = topology_init();
  if (likwid_ret < 0) {
    DART_LOG_ERROR("dart__base__locality__global_domain_new: "
                   "likwid: topology_init failed, returned %d", likwid_ret);
  } else {
    CpuInfo_t     info = get_cpuInfo();
    CpuTopology_t topo = get_cpuTopology();
    if (loc->min_cpu_mhz < 0 || loc->max_cpu_mhz < 0) {
      loc->min_cpu_mhz = info->clock;
      loc->max_cpu_mhz = info->clock;
    }
    if (loc->num_sockets < 0) {
      loc->num_sockets = topo->numSockets;
    }
    if (loc->num_numa < 0) {
      loc->num_numa    = loc->num_sockets;
    }
    if (loc->num_cores < 0) {
      loc->num_cores    = topo->numCoresPerSocket * loc->num_sockets;
    }
    topology_finalize();
    DART_LOG_TRACE("dart__base__locality__global_domain_new: likwid: "
                   "num_sockets: %d num_numa: %d num_cores: %d",
                   loc->num_sockets, loc->num_numa, loc->num_cores);
  }
#endif /* DART_ENABLE_LIKWID */

#ifdef DART_ENABLE_HWLOC
  DART_LOG_TRACE("dart__base__locality__global_domain_new: using hwloc");

  hwloc_topology_t topology;
  hwloc_topology_init(&topology);
  hwloc_topology_load(topology);
  // Resolve cache sizes, ordered by locality (i.e. smallest first):
  int level = 0;
  hwloc_obj_t obj;
  for (obj = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, 0);
       obj;
       obj = obj->parent) {
    if (obj->type == HWLOC_OBJ_CACHE) {
      loc->cache_sizes[level]      = obj->attr->cache.size;
      loc->cache_line_sizes[level] = obj->attr->cache.linesize;
      ++level;
    }
  }
  if (loc->num_sockets < 0) {
    // Resolve number of sockets:
    int depth = hwloc_get_type_depth(topology, HWLOC_OBJ_SOCKET);
    if (depth != HWLOC_TYPE_DEPTH_UNKNOWN) {
      loc->num_sockets = hwloc_get_nbobjs_by_depth(topology, depth);
    }
  }
  if (loc->num_numa < 0) {
    // Resolve number of NUMA nodes:
    int n_numa_nodes = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_NODE);
    if (n_numa_nodes > 0) {
      loc->num_numa = n_numa_nodes;
    }
  }
  if (loc->num_cores < 0) {
    // Resolve number of cores:
    int n_cores = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_CORE);
    if (n_cores > 0) {
      loc->num_cores = n_cores;
    }
	}
  if (loc->num_cores > 0 && (loc->min_threads < 0 || loc->max_threads < 0)) {
    // Resolve number of threads per core:
    int n_cpus       = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);
    loc->min_threads = n_cpus / loc->num_cores;
    loc->max_threads = n_cpus / loc->num_cores;
  }
  hwloc_topology_destroy(topology);
  DART_LOG_TRACE("dart__base__locality__global_domain_new: hwloc: "
                 "num_sockets: %d num_numa: %d num_cores: %d",
                 loc->num_sockets, loc->num_numa, loc->num_cores);
#endif /* DART_ENABLE_HWLOC */

#ifdef DART_ENABLE_PAPI
  DART_LOG_TRACE("dart__base__locality__global_domain_new: using PAPI");

  const PAPI_hw_info_t * hwinfo = NULL;
  if (dart__base__locality__papi_init(&hwinfo) == DART_OK) {
    if (loc->num_sockets < 0) { loc->num_sockets = hwinfo->sockets; }
    if (loc->num_numa    < 0) { loc->num_numa    = hwinfo->nnodes;  }
    if (loc->num_cores   < 0) {
      int cores_per_socket = hwinfo->cores;
      loc->num_cores        = loc->num_sockets * cores_per_socket;
    }
    if (loc->min_cpu_mhz < 0 || loc->max_cpu_mhz < 0) {
      loc->min_cpu_mhz = hwinfo->cpu_min_mhz;
      loc->max_cpu_mhz = hwinfo->cpu_max_mhz;
    }
    DART_LOG_TRACE("dart__base__locality__global_domain_new: PAPI: "
                   "num_sockets: %d num_numa: %d num_cores: %d",
                   loc->num_sockets, loc->num_numa, loc->num_cores);
  }
#endif /* DART_ENABLE_PAPI */

#ifdef DART__ARCH__IS_MIC
  DART_LOG_TRACE("dart__base__locality__global_domain_new: MIC architecture");

  if (loc->num_sockets < 0) { loc->num_sockets =  1; }
  if (loc->num_numa    < 0) { loc->num_numa    =  1; }
  if (loc->num_cores   < 0) { loc->num_cores   = 60; }
  if (loc->min_cpu_mhz < 0 || loc->max_cpu_mhz < 0) {
    loc->min_cpu_mhz = 1100;
    loc->max_cpu_mhz = 1100;
  }
  if (loc->min_threads < 0 || loc->max_threads < 0) {
    loc->min_threads = 4;
    loc->max_threads = 4;
  }
#endif

#ifdef DART__PLATFORM__POSIX
  if (loc->num_cores < 0) {
		/*
     * NOTE: includes hyperthreading
     */
    int posix_ret = sysconf(_SC_NPROCESSORS_ONLN);
    loc->num_cores = (posix_ret > 0) ? posix_ret : loc->num_cores;
		DART_LOG_TRACE(
      "dart__base__locality__global_domain_new: POSIX: loc->num_cores = %d",
      loc->num_cores);
  }
#endif

  int unit_master_cpu_id  = -1;
  int unit_master_numa_id = -1;

#ifdef DART__PLATFORM__LINUX
  unit_master_cpu_id = sched_getcpu();
#else
  DART_LOG_ERROR("dart__base__locality__global_domain_new: "
                 "Linux platform required");
  return DART_ERR_OTHER;
#endif

#ifdef DART_ENABLE_NUMA
  DART_LOG_TRACE("dart__base__locality__global_domain_new: using numalib");
  if (loc->num_numa < 0) {
    loc->num_numa       = numa_max_node() + 1;
  }
  if (unit_master_cpu_id >= 0) {
    unit_master_numa_id = numa_node_of_cpu(unit_master_cpu_id);
  }
  DART_LOG_TRACE("dart__base__locality__global_domain_new: numalib: "
                 "num_sockets: %d num_numa: %d num_cores: %d",
                 loc->num_sockets, loc->num_numa, loc->num_cores);
#else
  if (loc->num_numa < 0) {
    loc->num_numa = 1;
  }
#endif

  if (loc->num_nodes   < 0) {
    loc->num_nodes     = 1;
  }
  if (loc->num_modules < 0) {
    loc->num_modules   = 1;
  }
  if (loc->num_cores   < 0) {
    loc->num_cores     = 1;
  }
  if (loc->num_numa    > 0) {
    loc->numa_ids      = (int *)(malloc(sizeof(int) * loc->num_numa));
    loc->numa_ids[0]   = unit_master_numa_id;
  }
  if (loc->num_cores   > 0) {
    loc->cpu_ids       = (int *)(malloc(sizeof(int) * loc->num_cores));
    loc->cpu_ids[0]    = unit_master_cpu_id;
  }
  if (loc->min_threads < 0 || loc->max_threads < 0) {
    loc->min_threads   = 1;
    loc->max_threads   = 1;
  }

  DART_LOG_TRACE("dart__base__locality__global_domain_new: finished: "
                 "num_sockets: %d num_numa: %d num_cores: %d",
                 loc->num_sockets, loc->num_numa, loc->num_cores);

  dart__base__locality__create_subdomains(loc);

  DART_LOG_DEBUG("dart__base__locality__global_domain_new > %p", loc);
  return DART_OK;
}

dart_ret_t dart__base__locality__create_subdomains(
  dart_domain_locality_t * loc)
{
  DART_LOG_DEBUG("dart__base__locality__create_subdomains() loc: %p - "
                 "scope: %d level: %d subdomains: %d domain(%s)",
                 loc, loc->scope, loc->level, loc->num_domains,
                 loc->domain_tag);

  dart_locality_scope_t sub_scope;
  sub_scope           = DART_LOCALITY_SCOPE_UNDEFINED;
  int sub_level       = loc->level + 1;
  int sub_num_nodes   = loc->num_nodes;
  int sub_num_modules = loc->num_modules;
  int sub_num_numa    = loc->num_numa;
  int sub_num_sockets = loc->num_sockets;
  int sub_num_cores   = loc->num_cores;
  switch (loc->scope) {
    case DART_LOCALITY_SCOPE_UNDEFINED:
      DART_LOG_ERROR("dart__base__locality__create_subdomains ! "
                     "locality scope undefined");
      return DART_ERR_INVAL;
    case DART_LOCALITY_SCOPE_GLOBAL:
      DART_LOG_TRACE("dart__base__locality__create_subdomains: sub: nodes");
      loc->num_domains    = loc->num_nodes;
      sub_num_nodes       = 1;
      sub_scope           = DART_LOCALITY_SCOPE_NODE;
      break;
    case DART_LOCALITY_SCOPE_NODE:
      DART_LOG_TRACE("dart__base__locality__create_subdomains: sub: groups");
      loc->num_domains    = loc->num_modules;
      sub_num_modules     = 1;
      sub_scope           = DART_LOCALITY_SCOPE_MODULE;
      break;
    case DART_LOCALITY_SCOPE_MODULE:
      DART_LOG_TRACE("dart__base__locality__create_subdomains: sub: NUMA nodes");
      loc->num_domains    = loc->num_numa;
      sub_num_numa        = 1;
      sub_num_cores       = loc->num_cores / loc->num_numa;
      sub_scope           = DART_LOCALITY_SCOPE_NUMA;
      break;
    case DART_LOCALITY_SCOPE_NUMA:
      DART_LOG_TRACE("dart__base__locality__create_subdomains: sub: UMA nodes");
      loc->num_domains    = loc->num_cores;
      sub_num_numa        = 1;
      sub_num_cores       = 1;
      sub_scope           = DART_LOCALITY_SCOPE_UNIT;
      break;
    case DART_LOCALITY_SCOPE_UNIT:
      DART_LOG_TRACE("dart__base__locality__create_subdomains: sub: cores");
      loc->num_domains    = 0;
      sub_num_cores       = 1;
      sub_scope           = DART_LOCALITY_SCOPE_CORE;
      break;
    default:
      DART_LOG_TRACE("dart__base__locality__create_subdomains: sub: other");
      loc->num_domains    = 0;
      break;
  }
  DART_LOG_TRACE("dart__base__locality__create_subdomains: subdomains: %d",
                 loc->num_domains);
  if (loc->num_domains == 0) {
    loc->domains = NULL;
    DART_LOG_DEBUG("dart__base__locality__create_subdomains > loc: %p - "
                   "scope: %d level: %d subdomains: %d domain(%s) - final",
                   loc, loc->scope, loc->level, loc->num_domains,
                   loc->domain_tag);
    return DART_OK;
  }

  loc->domains = (dart_domain_locality_t *)(
                    malloc(loc->num_domains *
                           sizeof(dart_domain_locality_t)));
  dart_ret_t ret;
  for (int d = 0; d < loc->num_domains; ++d) {
    dart_domain_locality_t * subdomain = loc->domains + d;
    dart__base__locality__domain_locality_init(subdomain);
    /* initialize subdomain scope, level and hwinfo: */
    subdomain->parent      = loc;
    subdomain->level       = sub_level;
    subdomain->scope       = sub_scope;
    subdomain->num_nodes   = sub_num_nodes;
    subdomain->num_numa    = sub_num_numa;
    subdomain->num_modules = sub_num_modules;
    subdomain->num_sockets = sub_num_sockets;
    subdomain->num_cores   = sub_num_cores;
    /* host of subdomain is same as host of parent domain: */
    strncpy(subdomain->host, loc->host, DART_LOCALITY_HOST_MAX_SIZE);
    /* initialize subdomain tag with parent domain tag, e.g. ".0",
     * returns pointer to end of domain tag: */
    int base_tag_len = 0;
    if (loc->level > 0) {
      /* only copy base domain tag if it contains preceeding domain parts: */
      base_tag_len = sprintf(subdomain->domain_tag, "%s",
                             loc->domain_tag);
    }
    /* append the subdomain tag part to subdomain tag, e.g. ".0.1": */
    sprintf(subdomain->domain_tag + base_tag_len, ".%d", d);
    /* recursively initialize subdomains: */
    ret = dart__base__locality__create_subdomains(subdomain);
    if (ret != DART_OK) {
      DART_LOG_ERROR("dart__base__locality__create_subdomains ! "
                     "failed to initialize subdomains of '%s' (%d)",
                     subdomain->domain_tag, ret);
      return ret;
    }
  }
  DART_LOG_DEBUG("dart__base__locality__create_subdomains > loc: %p - "
                 "scope: %d level: %d subdomains: %d domain(%s)",
                 loc, loc->scope, loc->level, loc->num_domains,
                 loc->domain_tag);
  return DART_OK;
}

dart_ret_t dart__base__locality__domain(
  const char              * domain_tag,
  dart_domain_locality_t ** locality)
{
  DART_LOG_DEBUG("dart__base__locality__domain() domain(%s)", domain_tag);

  dart_domain_locality_t * domain = &dart__base__locality__domain_root_;

  /* pointer to dot separator in front of tag part:  */
  const char * dot_begin = domain_tag;
  /* pointer to dot separator in after tag part:     */
  char *       dot_end   = (char *)(domain_tag) + 1;
  /* Iterate tag (.1.2.3) by parts (1, 2, 3):        */
  while (*dot_end != '\0') {
    /* domain tag part converted to int is relative index of child: */
    long tag_part      = strtol(dot_begin, &dot_end, 10);
    int  subdomain_idx = (int)(tag_part);
    if (domain == NULL) {
      /* tree leaf node reached before last tag part: */
      return DART_ERR_INVAL;
    }
    if (domain->num_domains <= subdomain_idx) {
      /* child index out of range: */
      return DART_ERR_INVAL;
    }
    /* descend to child at relative index: */
    domain    = domain->domains + subdomain_idx;
    /* continue scanning of domain tag at next part: */
    dot_begin = dot_end;
  }
  *locality = domain;
  return DART_OK;
}

/* ======================================================================== *
 * Unit Locality                                                            *
 * ======================================================================== */

dart_ret_t dart__base__locality__unit_locality_init(
  dart_unit_locality_t * loc)
{
  DART_LOG_TRACE("dart__base__locality__unit_locality_init() loc: %p", loc);
  if (loc == NULL) {
    DART_LOG_ERROR("dart__base__locality__unit_locality_init ! null");
    return DART_ERR_INVAL;
  }
  loc->unit          = DART_UNDEFINED_UNIT_ID;
  loc->domain_tag[0] = '\0';
  loc->host[0]       = '\0';
  loc->numa_id       = -1;
  loc->core_id       = -1;
  loc->num_cores     = -1;
  loc->num_threads   = -1;
  loc->max_cpu_mhz   = -1;
  loc->min_cpu_mhz   = -1;
  DART_LOG_TRACE("dart__base__locality__unit_locality_init >");
  return DART_OK;
}

dart_ret_t dart__base__locality__local_unit_new(
  dart_unit_locality_t * loc)
{
  DART_LOG_DEBUG("dart__base__locality__local_unit_new() loc(%p)", loc);
  if (loc == NULL) {
    DART_LOG_ERROR("dart__base__locality__local_unit_new ! null");
    return DART_ERR_INVAL;
  }
  dart_unit_t myid = DART_UNDEFINED_UNIT_ID;

  DART_ASSERT_RETURNS(dart__base__locality__unit_locality_init(loc), DART_OK);
  DART_ASSERT_RETURNS(dart_myid(&myid), DART_OK);

  /* assign global domain to unit locality descriptor: */
  strncpy(loc->domain_tag, ".", 1);
  loc->domain_tag[1] = '\0';

  dart_domain_locality_t * dloc;
  DART_ASSERT_RETURNS(dart_domain_locality(".", &dloc), DART_OK);

  loc->unit        = myid;
  loc->num_cores   = 1;
  loc->core_id     = dloc->cpu_ids[0];
  loc->numa_id     = dloc->numa_ids[0];
  loc->min_cpu_mhz = dloc->min_cpu_mhz;
  loc->max_cpu_mhz = dloc->max_cpu_mhz;

  strncpy(loc->host, dloc->host, DART_LOCALITY_HOST_MAX_SIZE);

#ifdef DART_ENABLE_HWLOC
  hwloc_topology_t topology;
  hwloc_topology_init(&topology);
  hwloc_topology_load(topology);
  // Resolve number of threads per core:
  int n_cpus = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);
  if (n_cpus > 0) {
    loc->num_threads = n_cpus / dloc->num_cores;
  }
  hwloc_topology_destroy(topology);
#endif

#ifdef DART__ARCH__IS_MIC
  DART_LOG_TRACE("dart__base__locality__local_unit_new: "
                 "MIC architecture");

  if (loc->numa_id     < 0) { loc->numa_id     =  0; }
  if (loc->num_cores   < 0) { loc->num_cores   = 60; }
  if (loc->min_cpu_mhz < 0 || loc->max_cpu_mhz < 0) {
    loc->min_cpu_mhz = 1100;
    loc->max_cpu_mhz = 1100;
  }
  if (loc->num_threads < 0) {
    loc->num_threads = 4;
  }
#endif
  if (loc->num_threads < 0) {
    loc->num_threads = 1;
  }

  DART_LOG_DEBUG("dart__base__locality__local_unit_new > loc(%p)", loc);
  return DART_OK;
}

dart_ret_t dart__base__locality__node_units(
  const char   * hostname,
  dart_unit_t ** units,
  int          * num_units)
{
  DART_LOG_TRACE("dart__base__locality__node_units() host: %s", hostname);
  *num_units     = 0;
  int host_found = 0;
  for (int h = 0; h < dart__base__locality__num_hosts_; ++h) {
    dart_node_units_t * node_units = &dart__base__locality__node_units_[h];
    if (strncmp(node_units->host, hostname, DART_LOCALITY_HOST_MAX_SIZE) == 0) {
      *units     = node_units->units;
      *num_units = node_units->num_units;
      host_found = 1;
      break;
    }
  }
  if (host_found) {
    DART_LOG_TRACE("dart__base__locality__node_units > num_units: %d",
                   *num_units);
    return DART_OK;
  }
  DART_LOG_ERROR("dart__base__locality__node_units ! no entry for host '%s')",
                 hostname);
  return DART_ERR_NOTFOUND;
}
