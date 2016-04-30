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


/* ======================================================================== *
 * Domain Locality                                                          *
 * ======================================================================== */

void dart__base__locality__domain_locality_init(
  dart_domain_locality_t * loc)
{
  DART_LOG_TRACE("dart__base__locality__domain_locality_init() loc: %p", loc);
  loc->domain_tag[0]       = '\0';
  loc->scope               = DART_LOCALITY_SCOPE_UNDEFINED;
  loc->level               =  0;
  loc->host[0]             = '\0';
  loc->num_domains         =  0;
  loc->domains             = NULL;
  loc->num_numa            = -1;
  loc->numa_ids            = NULL;
  loc->num_cores            = -1;
  loc->cpu_ids             = NULL;
  loc->num_nodes           = -1;
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
}

dart_ret_t dart__base__locality__global_domain(
  dart_domain_locality_t ** global_domain)
{
  DART_LOG_DEBUG("dart__base__locality__global_domain()");

  dart_domain_locality_t * loc =
    (dart_domain_locality_t *)(malloc(sizeof(dart_domain_locality_t)));

  /* initialize domain descriptor: */
  dart__base__locality__domain_locality_init(loc);

  /* set domain tag: */
  strncpy(loc->domain_tag, ".", DART_LOCALITY_DOMAIN_TAG_MAX_SIZE);
  loc->domain_tag[DART_LOCALITY_DOMAIN_TAG_MAX_SIZE-1] = '\0';

  /* set domain scope and level: */
  loc->scope = DART_LOCALITY_SCOPE_GLOBAL;
  loc->level = 0;

  /* set domain host name: */
  gethostname(loc->host, DART_LOCALITY_HOST_MAX_SIZE);

#ifdef DART_ENABLE_LIKWID
  DART_LOG_DEBUG("dart__base__locality__global_domain: using LIKWID");

  /*
   * see likwid API documentation:
   * https://rrze-hpc.github.io/likwid/Doxygen/C-likwidAPI-code.html
   */
  int likwid_ret = topology_init();
  if (likwid_ret < 0) {
    DART_LOG_ERROR("dart__base__locality__global_domain: "
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
  }
#endif /* DART_ENABLE_LIKWID */

#ifdef DART_ENABLE_HWLOC
  DART_LOG_DEBUG("dart__base__locality__global_domain: using HWLOC");

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
  hwloc_topology_destroy(topology);
#endif /* DART_ENABLE_HWLOC */

#ifdef DART_ENABLE_PAPI
  DART_LOG_DEBUG("dart__base__locality__global_domain: using PAPI");

  const PAPI_hw_info_t * hwinfo = NULL;
  if (dart__locality__papi_init(&hwinfo) == DART_OK) {
    if (loc->num_sockets < 0) { loc->num_sockets = hwinfo->sockets; }
    if (loc->num_numa    < 0) { loc->num_numa    = hwinfo->nnodes;  }
    if (loc->num_cores    < 0) {
      int cores_per_socket = hwinfo->cores;
      loc->num_cores        = loc->num_sockets * cores_per_socket;
    }
    if (loc->min_cpu_mhz < 0 || loc->max_cpu_mhz < 0) {
      loc->min_cpu_mhz = hwinfo->cpu_min_mhz;
      loc->max_cpu_mhz = hwinfo->cpu_max_mhz;
    }
  }
#endif /* DART_ENABLE_PAPI */

#ifdef DART__ARCH__IS_MIC
  DART_LOG_DEBUG("dart__base__locality__global_domain: MIC architecture");

  if (loc->num_sockets < 0) { loc->num_sockets =  1; }
  if (loc->num_numa    < 0) { loc->num_numa    =  1; }
  if (loc->num_cores   < 0) { loc->num_cores   = 60; }
  if (loc->min_cpu_mhz < 0 || loc->max_cpu_mhz < 0) {
    loc->min_cpu_mhz = 1100;
    loc->max_cpu_mhz = 1100;
  }
#endif

#ifdef DART__PLATFORM__POSIX
  if (loc->num_cores < 0) {
		/*
     * NOTE: includes hyperthreading
     */
    int posix_ret = sysconf(_SC_NPROCESSORS_ONLN);
    loc->num_cores = (posix_ret > 0) ? posix_ret : loc->num_cores;
		DART_LOG_DEBUG(
      "dart__base__locality__global_domain: POSIX: loc->num_cores = %d",
      loc->num_cores);
  }
#endif

  int unit_master_cpu_id  = -1;
  int unit_master_numa_id = -1;

#ifdef DART__PLATFORM__LINUX
  unit_master_cpu_id = sched_getcpu();
#else
  DART_LOG_ERROR("dart__base__locality__global_domain: "
                 "Linux platform required");
  return DART_ERR_OTHER;
#endif

#ifdef DART_ENABLE_NUMA
  if (loc->num_numa < 0) {
    DART_LOG_DEBUG("dart__base__locality__global_domain: using numalib");
    loc->num_numa       = numa_max_node() + 1;
    unit_master_numa_id = numa_node_of_cpu(unit_master_cpu_id);
  }
#endif

  if (loc->num_nodes < 0) {
    loc->num_nodes = 1;
  }
  if (loc->num_cores < 0) {
    loc->num_cores  = 1;
  }
  if (loc->num_numa > 0) {
    loc->numa_ids    = (int *)(malloc(sizeof(int) * loc->num_numa));
    loc->numa_ids[0] = unit_master_numa_id;
  }
  if (loc->num_cores > 0) {
    loc->cpu_ids     = (int *)(malloc(sizeof(int) * loc->num_cores));
    loc->cpu_ids[0]  = unit_master_cpu_id;
  }

  *global_domain = loc;

  DART_LOG_DEBUG("dart__base__locality__global_domain > domain: %s -> %p",
                 domain_tag, *locality);
  return DART_OK;
}

/* ======================================================================== *
 * Unit Locality                                                            *
 * ======================================================================== */

void dart__base__locality__unit_locality_init(
  dart_unit_locality_t * loc)
{
  loc->unit_id       = DART_UNDEFINED_UNIT_ID;
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
}

dart_ret_t dart__base__locality__local_unit(
  dart_unit_locality_t ** locality)
{
  DART_LOG_DEBUG("dart__base__locality__local_unit()");
  dart_ret_t ret;

  *locality = NULL;

  dart_unit_locality_t * loc =
    (dart_unit_locality_t *)(malloc(sizeof(dart_unit_locality_t)));

  dart__base__locality__unit_locality_init(loc);

  ret = dart_myid(&(loc->unit_id));
  if (ret != DART_OK) {
    return ret;
  }

  strncpy(loc->domain_tag, ".0", 2);
  loc->domain_tag[2] = '\0';

  /*
   * TODO: Temporary implementation, copying locality information from
   *       domain '0'.
   *       Should be lookup of locality information for unit id.
   */
  dart_domain_locality_t * dloc;
  dart_domain_locality("0", &dloc);

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
  int n_threads = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);
  if (n_threads > 0) {
    loc->num_threads = n_threads / dloc->num_cores;
  }
  hwloc_topology_destroy(topology);
#else
  loc->num_threads = 1;
#endif

  free(dloc);

  *locality = loc;

  DART_LOG_DEBUG("dart__base__locality__local_unit > %p", *locality);
  return DART_OK;
}

