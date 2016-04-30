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

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_locality.h>

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

#ifdef DART_ENABLE_PAPI
static void dart__base__locality__papi_handle_error(
  int papi_ret)
{
  /*
   * PAPI_EINVAL   papi.h is different from the version used to compile the
   *               PAPI library.
   * PAPI_ENOMEM   Insufficient memory to complete the operation.
   * PAPI_ECMP     This component does not support the underlying hardware.
   * PAPI_ESYS     A system or C library call failed inside PAPI, see the
   *               errno variable.
   */
  switch (papi_ret) {
    case PAPI_EINVAL:
         DART_LOG_ERROR("dart__base__locality: PAPI_EINVAL: "
                        "papi.h is different from the version used to "
                        "compile the PAPI library.");
         break;
    case PAPI_ENOMEM:
         DART_LOG_ERROR("dart__base__locality: PAPI_ENOMEM: "
                        "insufficient memory to complete the operation.");
         break;
    case PAPI_ECMP:
         DART_LOG_ERROR("dart__base__locality: PAPI_ENOMEM: "
                        "this component does not support the underlying "
                        "hardware.");
         break;
    case PAPI_ESYS:
         DART_LOG_ERROR("dart_domain_locality: PAPI_ESYS: "
                        "a system or C library call failed inside PAPI, see "
                        "the errno variable");
         DART_LOG_ERROR("dart__base__locality: PAPI_ESYS: errno: %d", errno);
         break;
    default:
         DART_LOG_ERROR("dart__base__locality: PAPI: unknown error: %d",
                        papi_ret);
         break;
  }
}

static dart_ret_t dart__base__locality__papi_init(
  const PAPI_hw_info_t ** hwinfo)
{
  int papi_ret;

  if (PAPI_is_initialized()) {
    *hwinfo = PAPI_get_hardware_info();
    return DART_OK;
  }
  DART_LOG_DEBUG("dart__base__locality: PAPI: init");

  papi_ret = PAPI_library_init(PAPI_VER_CURRENT);

  if (papi_ret != PAPI_VER_CURRENT && papi_ret > 0) {
    DART_LOG_ERROR("dart__base__locality: PAPI: version mismatch");
    return DART_ERR_OTHER;
  } else if (papi_ret < 0) {
    DART_LOG_ERROR("dart__base__locality: PAPI: init failed, returned %d",
                   papi_ret);
    dart__base__locality__papi_handle_error(papi_ret);
    return DART_ERR_OTHER;
  } else {
    papi_ret = PAPI_is_initialized();
    if (papi_ret != PAPI_LOW_LEVEL_INITED) {
      dart__base__locality__papi_handle_error(papi_ret);
      return DART_ERR_OTHER;
    }
  }

  papi_ret = PAPI_thread_init(pthread_self);

  if (papi_ret != PAPI_OK) {
    DART_LOG_ERROR("dart__base__locality: PAPI: PAPI_thread_init failed");
    return DART_ERR_OTHER;
  }

  *hwinfo = PAPI_get_hardware_info();
  if (*hwinfo == NULL) {
    DART_LOG_ERROR("dart__base__locality: PAPI: get hardware info failed");
    return DART_ERR_OTHER;
  }

  DART_LOG_DEBUG("dart__base__locality: PAPI: initialized");
  return DART_OK;
}
#endif /* DART_ENABLE_PAPI */


dart_domain_locality_t _dart__base__locality__domain_root;

/* ======================================================================== *
 * Init / Finalize                                                          *
 * ======================================================================== */

dart_ret_t dart__base__locality__init()
{
  DART_LOG_DEBUG("dart__base__locality__init()");

  dart_ret_t ret = dart__base__locality__global_domain_new(
                     &_dart__base__locality__domain_root);
  if (ret != DART_OK) {
    DART_LOG_ERROR("dart__base__locality__init ! "
                   "dart__base__locality__global_domain_new failed: %d", ret);
    return ret;
  }
  DART_LOG_DEBUG("dart__base__locality__init >");
  return DART_OK;
}

dart_ret_t dart__base__locality__finalize()
{
  DART_LOG_DEBUG("dart__base__locality__finalize()");

  dart_ret_t ret = dart__base__locality__domain_delete(
                     &_dart__base__locality__domain_root);
  if (ret != DART_OK) {
    DART_LOG_ERROR("dart__base__locality__finalize ! "
                   "dart__base__locality__domain_delete failed: %d", ret);
    return ret;
  }
  DART_LOG_DEBUG("dart__base__locality__finalize >");
  return DART_OK;
}

/* ======================================================================== *
 * Domain Locality                                                          *
 * ======================================================================== */

dart_ret_t dart__base__locality__get_subdomains(
  dart_domain_locality_t * loc);

dart_ret_t dart__base__locality__domain_locality_init(
  dart_domain_locality_t * loc)
{
  DART_LOG_TRACE("dart__base__locality__domain_locality_init() loc: %p", loc);
  if (loc == NULL) {
    DART_LOG_ERROR("dart__base__locality__domain_locality_init ! null");
    return DART_ERR_INVAL;
  }
  loc->domain_tag[0]       = '\0';
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
  loc->num_groups          = -1;
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
  DART_LOG_DEBUG("dart__base__locality__domain_delete() loc: %p domain(%s)",
                 domain, domain->domain_tag);
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

  DART_LOG_DEBUG("dart__base__locality__domain_delete >");
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

  if (loc->num_nodes < 0) {
    loc->num_nodes   = 1;
  }
  if (loc->num_groups < 0) {
    loc->num_groups  = 1;
  }
  if (loc->num_cores < 0) {
    loc->num_cores   = 1;
  }
  if (loc->num_numa  > 0) {
    loc->numa_ids    = (int *)(malloc(sizeof(int) * loc->num_numa));
    loc->numa_ids[0] = unit_master_numa_id;
  }
  if (loc->num_cores > 0) {
    loc->cpu_ids     = (int *)(malloc(sizeof(int) * loc->num_cores));
    loc->cpu_ids[0]  = unit_master_cpu_id;
  }
  if (loc->min_threads < 0 || loc->max_threads < 0) {
    loc->min_threads = 1;
    loc->max_threads = 1;
  }

  DART_LOG_TRACE("dart__base__locality__global_domain_new: finished: "
                 "num_sockets: %d num_numa: %d num_cores: %d",
                 loc->num_sockets, loc->num_numa, loc->num_cores);

  dart__base__locality__get_subdomains(loc);

  DART_LOG_DEBUG("dart__base__locality__global_domain_new > %p", loc);
  return DART_OK;
}

dart_ret_t dart__base__locality__get_subdomains(
  dart_domain_locality_t * loc)
{
  DART_LOG_DEBUG("dart__base__locality__get_subdomains() loc: %p - "
                 "scope: %d level: %d subdomains: %d domain(%s)",
                 loc, loc->scope, loc->level, loc->num_domains,
                 loc->domain_tag);

  int sub_scope       = (int)(DART_LOCALITY_SCOPE_UNDEFINED);
  int sub_level       = loc->level + 1;
  int sub_num_nodes   = loc->num_nodes;
  int sub_num_groups  = loc->num_groups;
  int sub_num_numa    = loc->num_numa;
  int sub_num_sockets = loc->num_sockets;
  int sub_num_cores   = loc->num_cores;
  switch (loc->scope) {
    case DART_LOCALITY_SCOPE_UNDEFINED:
      DART_LOG_ERROR("dart__base__locality__get_subdomains ! "
                     "locality scope undefined");
      return DART_ERR_INVAL;
    case DART_LOCALITY_SCOPE_GLOBAL:
      DART_LOG_TRACE("dart__base__locality__get_subdomains: sub: nodes");
      loc->num_domains    = loc->num_nodes;
      sub_num_nodes       = 1;
      sub_scope           = DART_LOCALITY_SCOPE_NODE;
      break;
    case DART_LOCALITY_SCOPE_NODE:
      DART_LOG_TRACE("dart__base__locality__get_subdomains: sub: groups");
      loc->num_domains    = loc->num_groups;
      sub_num_groups      = 1;
      sub_scope           = DART_LOCALITY_SCOPE_PROC_GROUP;
      break;
    case DART_LOCALITY_SCOPE_PROC_GROUP:
      DART_LOG_TRACE("dart__base__locality__get_subdomains: sub: NUMA nodes");
      loc->num_domains    = loc->num_numa;
      sub_num_numa        = 1;
      sub_num_cores       = loc->num_cores / loc->num_numa;
      sub_scope           = DART_LOCALITY_SCOPE_NUMA;
      break;
    case DART_LOCALITY_SCOPE_NUMA:
      DART_LOG_TRACE("dart__base__locality__get_subdomains: sub: UMA nodes");
      loc->num_domains    = loc->num_cores;
      sub_num_numa        = 1;
      sub_num_cores       = 1;
      sub_scope           = DART_LOCALITY_SCOPE_UNIT;
      break;
    case DART_LOCALITY_SCOPE_UNIT:
      DART_LOG_TRACE("dart__base__locality__get_subdomains: sub: cores");
      loc->num_domains    = 0;
      sub_num_cores       = 1;
      sub_scope           = DART_LOCALITY_SCOPE_CORE;
      break;
    default:
      DART_LOG_TRACE("dart__base__locality__get_subdomains: sub: other");
      loc->num_domains    = 0;
      break;
  }
  DART_LOG_TRACE("dart__base__locality__get_subdomains: subdomains: %d",
                 loc->num_domains);
  if (loc->num_domains == 0) {
    loc->domains = NULL;
    DART_LOG_DEBUG("dart__base__locality__get_subdomains > loc: %p - "
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
    subdomain->level       = sub_level;
    subdomain->scope       = sub_scope;
    subdomain->num_nodes   = sub_num_nodes;
    subdomain->num_numa    = sub_num_numa;
    subdomain->num_groups  = sub_num_groups;
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
    ret = dart__base__locality__get_subdomains(subdomain);
    if (ret != DART_OK) {
      DART_LOG_ERROR("dart__base__locality__get_subdomains ! "
                     "failed to initialize subdomains of '%s' (%d)",
                     subdomain->domain_tag, ret);
      return ret;
    }
  }
  DART_LOG_DEBUG("dart__base__locality__get_subdomains > loc: %p - "
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

  dart_domain_locality_t * domain = &_dart__base__locality__domain_root;

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
  loc->num_cores     = -1;
  loc->num_sockets   = -1;
  loc->num_numa      = -1;
  loc->numa_ids      = NULL;
  loc->num_cores     = -1;
  loc->cpu_ids       = NULL;
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
  dart_ret_t ret;

  ret = dart__base__locality__unit_locality_init(loc);
  if (ret != DART_OK) {
    DART_LOG_ERROR("dart__base__locality__local_unit_new ! "
                   "dart__base__locality__unit_locality_init failed: %d",
                   ret);
    return ret;
  }
  ret = dart_myid(&loc->unit);
  if (ret != DART_OK) {
    DART_LOG_ERROR("dart__base__locality__local_unit_new ! "
                   "dart_myid failed: %d", ret);
    return ret;
  }

  /* assign global domain to unit locality descriptor: */
  strncpy(loc->domain_tag, ".", 1);
  loc->domain_tag[1] = '\0';

  /*
   * TODO: Temporary implementation, copying locality information from
   *       domain '0'.
   *       Should be lookup of locality information for unit id.
   */
  dart_domain_locality_t * dloc;
  ret = dart_domain_locality(".", &dloc);
  if (ret != DART_OK) {
    DART_LOG_ERROR("dart__base__locality__local_unit_new ! "
                   "dart_domain_locality failed: %d", ret);
    return ret;
  }

  loc->num_cores = 1;
  if (loc->num_cores == 1) {
    loc->num_sockets = 1;
    loc->num_numa    = 1;
  } else {
    loc->num_sockets = dloc->num_sockets;
    loc->num_numa    = dloc->num_numa;
  }
  loc->numa_ids    = dloc->numa_ids;
  loc->cpu_ids     = dloc->cpu_ids;
  loc->min_cpu_mhz = dloc->min_cpu_mhz;
  loc->max_cpu_mhz = dloc->max_cpu_mhz;

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
#else
  loc->num_threads = 1;
#endif

  DART_LOG_DEBUG("dart__base__locality__local_unit_new > loc(%p)", loc);
  return DART_OK;
}

