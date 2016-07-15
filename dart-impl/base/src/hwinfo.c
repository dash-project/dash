
/*
 * Include config and utmpx.h first to prevent previous include of utmpx.h
 * without _GNU_SOURCE in included headers:
 */
#include <dash/dart/base/config.h>
#ifdef DART__PLATFORM__LINUX
#  define _GNU_SOURCE
#  include <utmpx.h>
#  include <sched.h>
#endif
#include <dash/dart/base/macro.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/base/locality.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/base/assert.h>

#include <dash/dart/base/internal/papi.h>
#include <dash/dart/base/internal/unit_locality.h>

#include <dash/dart/base/hwinfo.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_locality.h>

#include <inttypes.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

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

static const int BYTES_PER_MB = (1024 * 1024);

/* NOTE: The dart_hwinfo function must only return reliable information,
 *       typically obtained from system functions or libraries such as
 *       hwloc, PAPI, likwid etc.
 *       Hardware locality attributes that cannot be obtained or deduced
 *       are initialized with -1.
 *       Assumptions and approximations based on the common case
 *       are specific to the use case and must be decided in client code.
 *       Otherwise, users of this function would consider a possibly
 *       incorrect hwinfo attribute setting as actual system information.
 *
 *       For example, assuming
 *
 *         (numa node memory) = (system memory) / (num numa)
 *
 *       to derive default values for memory capacity is viable in most
 *       cases but harmful for compute nodes with accelerator components
 *       where this would lead to drastic host/target load imbalance.
 */

dart_ret_t dart_hwinfo(
  dart_hwinfo_t * hwinfo)
{
  DART_LOG_DEBUG("dart_hwinfo()");

  dart_hwinfo_t hw;

  hw.num_sockets         = -1;
  hw.num_numa            = -1;
  hw.numa_id             = -1;
  hw.num_cores           = -1;
  hw.cpu_id              = -1;
  hw.min_cpu_mhz         = -1;
  hw.max_cpu_mhz         = -1;
  hw.min_threads         = -1;
  hw.max_threads         = -1;
  hw.cache_sizes[0]      = -1;
  hw.cache_sizes[1]      = -1;
  hw.cache_sizes[2]      = -1;
  hw.cache_line_sizes[0] = -1;
  hw.cache_line_sizes[1] = -1;
  hw.cache_line_sizes[2] = -1;
  hw.cache_shared[0]     = -1;
  hw.cache_shared[1]     = -1;
  hw.cache_shared[2]     = -1;
  hw.max_shmem_mbps      = -1;
  hw.system_memory       = -1;
  hw.numa_memory         = -1;

  char * max_shmem_mbps_str = getenv("DASH_MAX_SHMEM_MBPS");
  if (NULL != max_shmem_mbps_str) {
    hw.max_shmem_mbps = (int)(atoi(max_shmem_mbps_str));
    DART_LOG_TRACE("dart_hwinfo: DASH_MAX_SHMEM_MBPS set: %d",
                   hw.max_shmem_mbps);
  } else {
    DART_LOG_TRACE("dart_hwinfo: DASH_MAX_SHMEM_MBPS not set");
  }
  if (hw.max_shmem_mbps <= 0) {
    /* TODO: Intermediate workaround for load balancing, use -1
     *       instead: */
    hw.max_shmem_mbps = 1235;
  }

#ifdef DART_ENABLE_LIKWID
  DART_LOG_TRACE("dart_hwinfo: using likwid");
  /*
   * see likwid API documentation:
   * https://rrze-hpc.github.io/likwid/Doxygen/C-likwidAPI-code.html
   */
  int likwid_ret = topology_init();
  if (likwid_ret < 0) {
    DART_LOG_ERROR("dart_hwinfo: "
                   "likwid: topology_init failed, returned %d", likwid_ret);
  } else {
    CpuInfo_t     info = get_cpuInfo();
    CpuTopology_t topo = get_cpuTopology();
    if (hw.min_cpu_mhz < 0 || hw.max_cpu_mhz < 0) {
      hw.min_cpu_mhz = info->clock;
      hw.max_cpu_mhz = info->clock;
    }
    if (hw.num_sockets < 0) {
      hw.num_sockets = topo->numSockets;
    }
    if (hw.num_numa < 0) {
      hw.num_numa    = hw.num_sockets;
    }
    if (hw.num_cores < 0) {
      hw.num_cores   = topo->numCoresPerSocket * hw.num_sockets;
    }
    topology_finalize();
    DART_LOG_TRACE("dart_hwinfo: likwid: "
                   "num_sockets: %d num_numa: %d num_cores: %d",
                   hw.num_sockets, hw.num_numa, hw.num_cores);
  }
#endif /* DART_ENABLE_LIKWID */

#ifdef DART_ENABLE_HWLOC
  DART_LOG_TRACE("dart_hwinfo: using hwloc");

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
      hw.cache_sizes[level]      = obj->attr->cache.size;
      hw.cache_line_sizes[level] = obj->attr->cache.linesize;
      ++level;
    }
  }
  if (hw.num_sockets < 0) {
    // Resolve number of sockets:
    int depth = hwloc_get_type_depth(topology, HWLOC_OBJ_SOCKET);
    if (depth != HWLOC_TYPE_DEPTH_UNKNOWN) {
      hw.num_sockets = hwloc_get_nbobjs_by_depth(topology, depth);
    }
  }
  if (hw.num_numa < 0) {
    /* NOTE:
     * Do not use HWLOC_OBJ_NUMANODE (requires >= hwloc-1.11.0).
     * HWLOC_OBJ_NODE is renamed to HWLOC_OBJ_NUMANODE in recent
     * distribution hwloc-1.11.0 but still supported for backward
     * compatibility and therefore works in all versions of hwloc.
     * When HWLOC_OBJ_NODE is marked as deprecated, add a condition
     * on hwloc version here.
     */
    int n_numa_nodes = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_NODE);
    if (n_numa_nodes > 0) {
      hw.num_numa = n_numa_nodes;
    }
  }
  if (hw.num_cores < 0) {
    int n_cores = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_CORE);
    if (n_cores > 0) {
      hw.num_cores = n_cores;
    }
	}
  if (hw.num_cores > 0 && hw.max_threads < 0) {
    int n_cpus     = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);
    hw.min_threads = 1;
    hw.max_threads = n_cpus / hw.num_cores;
  }

  if(hw.system_memory < 0) {
    hwloc_obj_t obj;
    obj = hwloc_get_obj_by_type(topology, HWLOC_OBJ_MACHINE, 0);
    hw.system_memory = obj->memory.total_memory / BYTES_PER_MB;
  }
  if(hw.numa_memory < 0) {
    hwloc_obj_t obj;
    /* NOTE:
     * Do not use HWLOC_OBJ_NUMANODE (requires >= hwloc-1.11.0).
     * HWLOC_OBJ_NODE is renamed to HWLOC_OBJ_NUMANODE in recent
     * distribution hwloc-1.11.0 but still supported for backward
     * compatibility and therefore works in all versions of hwloc.
     * When HWLOC_OBJ_NODE is marked as deprecated, add a condition
     * on hwloc version here.
     */
    obj = hwloc_get_obj_by_type(topology, HWLOC_OBJ_NODE, 0);
    if(obj != NULL) {
      hw.numa_memory = obj->memory.total_memory / BYTES_PER_MB;
    } else {
      /* No NUMA domain: */
      hw.numa_memory = hw.system_memory;
    }
  }
  hwloc_topology_destroy(topology);
  DART_LOG_TRACE("dart_hwinfo: hwloc: "
                 "num_sockets:%d num_numa:%d num_cores:%d",
                 hw.num_sockets, hw.num_numa, hw.num_cores);
#endif /* DART_ENABLE_HWLOC */

#ifdef DART_ENABLE_PAPI
  DART_LOG_TRACE("dart_hwinfo: using PAPI");

  const PAPI_hw_info_t * papi_hwinfo = NULL;
  if (dart__base__locality__papi_init(&papi_hwinfo) == DART_OK) {
    if (hw.num_sockets < 0) { hw.num_sockets = papi_hwinfo->sockets; }
    if (hw.num_numa    < 0) { hw.num_numa    = papi_hwinfo->nnodes;  }
    if (hw.num_cores   < 0) {
      int cores_per_socket = papi_hwinfo->cores;
      hw.num_cores   = hw.num_sockets * cores_per_socket;
    }
    if (hw.min_cpu_mhz < 0 || hw.max_cpu_mhz < 0) {
      hw.min_cpu_mhz = papi_hwinfo->cpu_min_mhz;
      hw.max_cpu_mhz = papi_hwinfo->cpu_max_mhz;
    }
    DART_LOG_TRACE("dart_hwinfo: PAPI: "
                   "num_sockets:%d num_numa:%d num_cores:%d",
                   hw.num_sockets, hw.num_numa, hw.num_cores);
  }
#endif /* DART_ENABLE_PAPI */

#ifdef DART__ARCH__IS_MIC
  /*
   * Hardware information for Intel MIC can be hard-coded as hardware
   * specs of MIC model variants are invariant:
   */
  DART_LOG_TRACE("dart_hwinfo: MIC architecture");

  if (hw.num_sockets < 0) { hw.num_sockets =  1; }
  if (hw.num_numa    < 0) { hw.num_numa    =  1; }
  if (hw.num_cores   < 0) { hw.num_cores   = 60; }
  if (hw.min_cpu_mhz < 0 || hw.max_cpu_mhz < 0) {
    hw.min_cpu_mhz = 1100;
    hw.max_cpu_mhz = 1100;
  }
  if (hw.min_threads < 0 || hw.max_threads < 0) {
    hw.min_threads = 4;
    hw.max_threads = 4;
  }
  if (hw.numa_id < 0) {
    hw.numa_id = 0;
  }
#endif

#ifdef DART__PLATFORM__POSIX
  if (hw.num_cores < 0) {
		/*
     * NOTE: includes hyperthreading
     */
    int posix_ret = sysconf(_SC_NPROCESSORS_ONLN);
    hw.num_cores = (posix_ret > 0) ? posix_ret : hw.num_cores;
		DART_LOG_TRACE(
      "dart_hwinfo: POSIX: hw.num_cores = %d",
      hw.num_cores);
  }

	if(hw.system_memory < 0) {
	  long pages     = sysconf(_SC_AVPHYS_PAGES);
	  long page_size = sysconf(_SC_PAGE_SIZE);
    if (pages > 0 && page_size > 0) {
      hw.system_memory = (int) ((pages * page_size) / BYTES_PER_MB);
    }
  }

#endif

#ifdef DART__PLATFORM__LINUX
  if (hw.cpu_id < 0) {
    hw.cpu_id = sched_getcpu();
  }
#else
  DART_LOG_ERROR("dart_hwinfo: "
                 "Linux platform required");
  return DART_ERR_OTHER;
#endif

#ifdef DART_ENABLE_NUMA
  DART_LOG_TRACE("dart_hwinfo: using numalib");
  if (hw.num_numa < 0) {
    hw.num_numa = numa_max_node() + 1;
  }
  if (hw.numa_id < 0 && hw.cpu_id >= 0) {
    hw.numa_id  = numa_node_of_cpu(hw.cpu_id);
  }
  DART_LOG_TRACE("dart_hwinfo: numalib: "
                 "num_sockets:%d num_numa:%d numa_id:%d num_cores:%d",
                 hw.num_sockets, hw.num_numa, hw.numa_id, hw.num_cores);
#endif

  DART_LOG_TRACE("dart_hwinfo: finished: "
                 "num_sockets:%d num_numa:%d "
                 "numa_id:%d cpu_id:%d, num_cores:%d "
                 "min_threads:%d max_threads:%d",
                 hw.num_sockets, hw.num_numa,
                 hw.numa_id, hw.cpu_id, hw.num_cores,
                 hw.min_threads, hw.max_threads);

  *hwinfo = hw;

  DART_LOG_DEBUG("dart_hwinfo >");
  return DART_OK;
}
