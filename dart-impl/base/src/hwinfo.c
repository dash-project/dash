#include <dash/dart/base/config.h>
#ifdef DART__PLATFORM__LINUX
/* _GNU_SOURCE required for sched_getcpu() */
#  define _GNU_SOURCE
#  include <sched.h>
#endif

#ifdef DART__PLATFORM__OSX
#include <cpuid.h>
#include <stdint.h>

#define CPUID(INFO, LEAF, SUBLEAF) __cpuid_count(LEAF, SUBLEAF, INFO[0], INFO[1], INFO[2], INFO[3])

static int osx_sched_getcpu() {                              
  uint32_t CPUInfo[4]; 
  int cpuid;                          
  CPUID(CPUInfo, 1, 0);                          
  /* CPUInfo[1] is EBX, bits 24-31 are APIC ID */ 
  if ( (CPUInfo[3] & (1 << 9)) == 0) {           
    cpuid = -1;  /* no APIC on chip */             
  }                                              
  else {                                         
    cpuid = (unsigned)CPUInfo[1] >> 24;                    
  }                                              
  if (cpuid < 0) cpuid = 0;
  return cpuid;                          
}
#endif

#include <dash/dart/base/macro.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/base/locality.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/base/assert.h>

#include <dash/dart/base/internal/papi.h>
#include <dash/dart/base/internal/hwloc.h>
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

dart_ret_t dart_hwinfo_init(
  dart_hwinfo_t * hw)
{
  hw->num_sockets         = -1;
  hw->num_numa            = -1;
  hw->numa_id             = -1;
  hw->num_cores           = -1;
  hw->core_id             = -1;
  hw->cpu_id              = -1;
  hw->min_cpu_mhz         = -1;
  hw->max_cpu_mhz         = -1;
  hw->min_threads         = -1;
  hw->max_threads         = -1;
  hw->cache_ids[0]        = -1;
  hw->cache_ids[1]        = -1;
  hw->cache_ids[2]        = -1;
  hw->cache_sizes[0]      = -1;
  hw->cache_sizes[1]      = -1;
  hw->cache_sizes[2]      = -1;
  hw->cache_line_sizes[0] = -1;
  hw->cache_line_sizes[1] = -1;
  hw->cache_line_sizes[2] = -1;
  hw->max_shmem_mbps      = -1;
  hw->system_memory_bytes = -1;
  hw->numa_memory_bytes   = -1;
  hw->num_scopes          = -1;

  dart_locality_scope_pos_t undef_scope;
  undef_scope.scope = DART_LOCALITY_SCOPE_UNDEFINED;
  undef_scope.index = -1;
  for (int s = 0; s < DART_LOCALITY_MAX_DOMAIN_SCOPES; s++) {
    hw->scopes[s] = undef_scope;
  }

  return DART_OK;
}

dart_ret_t dart_hwinfo(
  dart_hwinfo_t * hwinfo)
{
  DART_LOG_DEBUG("dart_hwinfo()");

  dart_hwinfo_t hw;
  dart_hwinfo_init(&hw);

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

  if(gethostname(hw.host, DART_LOCALITY_HOST_MAX_SIZE) != 0) {
    hw.host[DART_LOCALITY_HOST_MAX_SIZE-1] = '\0';
  }

#ifdef DART_ENABLE_HWLOC
  DART_LOG_TRACE("dart_hwinfo: using hwloc");

  hwloc_topology_t topology;
  hwloc_topology_init(&topology);
  hwloc_topology_set_flags(topology,
#if HWLOC_API_VERSION < 0x00020000
                             HWLOC_TOPOLOGY_FLAG_IO_DEVICES
                           | HWLOC_TOPOLOGY_FLAG_IO_BRIDGES
  /*                       | HWLOC_TOPOLOGY_FLAG_WHOLE_IO  */
#else
                             HWLOC_TOPOLOGY_FLAG_WHOLE_SYSTEM
#endif
                          );
  hwloc_topology_load(topology);

  /* hwloc can resolve the physical index (os_index) of the active unit,
   * not the logical index.
   * Queries in the topology hierarchy require the logical index, however.
   * As a workaround, units scan the topology for the logical index of the
   * CPU object that has a matching physical index.
   */

  /* Get PU of active thread: */
  hwloc_cpuset_t cpuset = hwloc_bitmap_alloc();
  int flags     = 0; // HWLOC_CPUBIND_PROCESS;
  int cpu_os_id = -1;
  int ret       = hwloc_get_last_cpu_location(topology, cpuset, flags);
  if (!ret) {
    cpu_os_id = hwloc_bitmap_first(cpuset);
  }
  hwloc_bitmap_free(cpuset);

  hwloc_obj_t cpu_obj;
  for (cpu_obj =
         hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, 0);
       cpu_obj;
       cpu_obj = cpu_obj->next_cousin) {
    if ((int)cpu_obj->os_index == cpu_os_id) {
      hw.cpu_id = cpu_obj->logical_index;
      break;
    }
  }

  DART_LOG_TRACE("dart_hwinfo: hwloc : cpu_id phys:%d logical:%d",
                 cpu_os_id, hw.cpu_id);

  /* PU (cpu_id) to CORE (core_id) object: */
  hwloc_obj_t core_obj;
  for (core_obj = hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, hw.cpu_id);
       core_obj;
       core_obj = core_obj->parent) {
    if (core_obj->type == HWLOC_OBJ_CORE) { break; }
  }
  if (core_obj) {
    hw.core_id = core_obj->logical_index;

    DART_LOG_TRACE("dart_hwinfo: hwloc : core logical index: %d",
                   hw.core_id);

    hw.num_scopes      = 0;
    int cache_level    = 0;
    /* Use CORE object to resolve caches: */
    hwloc_obj_t obj;
    for (obj = hwloc_get_obj_by_type(topology, HWLOC_OBJ_CORE, hw.core_id);
         obj;
         obj = obj->parent) {

      if (obj->type == HWLOC_OBJ_MACHINE) { break; }
      hw.scopes[hw.num_scopes].scope =
        dart__base__hwloc__obj_type_to_dart_scope(obj->type);
      hw.scopes[hw.num_scopes].index = obj->logical_index;
      DART_LOG_TRACE("dart_hwinfo: hwloc: parent[%d] (scope:%d index:%d)",
                     hw.num_scopes,
                     hw.scopes[hw.num_scopes].scope,
                     hw.scopes[hw.num_scopes].index);
      hw.num_scopes++;

#if HWLOC_API_VERSION < 0x00020000
      if (obj->type == HWLOC_OBJ_CACHE) {
        hw.cache_sizes[cache_level]      = obj->attr->cache.size;
        hw.cache_line_sizes[cache_level] = obj->attr->cache.linesize;
        hw.cache_ids[cache_level]        = obj->logical_index;
        cache_level++;
      }
#else
      if (obj->type == HWLOC_OBJ_L1CACHE) {
        hw.cache_sizes[0]              = obj->attr->cache.size;
        hw.cache_line_sizes[0]         = obj->attr->cache.linesize;
        hw.cache_ids[0]                = obj->logical_index;
        cache_level++;
      }
      else if (obj->type == HWLOC_OBJ_L2CACHE) {
        hw.cache_sizes[1]              = obj->attr->cache.size;
        hw.cache_line_sizes[1]         = obj->attr->cache.linesize;
        hw.cache_ids[1]                = obj->logical_index;
        cache_level++;
      }
      else if (obj->type == HWLOC_OBJ_L3CACHE) {
        hw.cache_sizes[2]              = obj->attr->cache.size;
        hw.cache_line_sizes[2]         = obj->attr->cache.linesize;
        hw.cache_ids[2]                = obj->logical_index;
        cache_level++;
      }
#endif
      else if (obj->type == HWLOC_OBJ_NODE) {
        hw.numa_id = obj->logical_index;
      } else if (obj->type ==
#if HWLOC_API_VERSION > 0x00011000
                 HWLOC_OBJ_PACKAGE
#else
                 HWLOC_OBJ_SOCKET
#endif
                ) {
      }
    }
  }

  if (hw.numa_id < 0) {
    hwloc_obj_t numa_obj;
    for (numa_obj =
           hwloc_get_obj_by_type(topology, HWLOC_OBJ_PU, hw.cpu_id);
         numa_obj;
         numa_obj = numa_obj->parent) {
      if (numa_obj->type == HWLOC_OBJ_NODE) {
        hw.numa_id = numa_obj->logical_index;
        break;
      }
    }
  }
  if (hw.num_numa < 0) {
    int n_numa_nodes = hwloc_get_nbobjs_by_type(
                         topology, DART__HWLOC_OBJ_NUMANODE);
    if (n_numa_nodes > 0) {
      hw.num_numa = n_numa_nodes;
    }
  }
  if (hw.num_sockets < 0) {
    int n_sockets = hwloc_get_nbobjs_by_type(topology, 
#if HWLOC_API_VERSION > 0x00011000
                 HWLOC_OBJ_PACKAGE
#else
                 HWLOC_OBJ_SOCKET
#endif
    );
    if (n_sockets > 0) {
      hw.num_sockets = n_sockets;
    }
	}
  if (hw.num_cores < 0) {
    int n_cores = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_CORE);
    if (n_cores > 0) {
      hw.num_cores = n_cores;
    }
	}
  if (hw.min_threads < 0 && hw.max_threads < 0 &&
      hw.num_cores > 0 && hw.max_threads < 0) {
    int n_cpus     = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_PU);
    hw.min_threads = 1;
    hw.max_threads = n_cpus / hw.num_cores;
  }

  if(hw.system_memory_bytes < 0) {
    hwloc_obj_t obj;
    obj = hwloc_get_obj_by_type(topology, HWLOC_OBJ_MACHINE, 0);
#if HWLOC_API_VERSION < 0x00020000
    hw.system_memory_bytes = obj->memory.total_memory / BYTES_PER_MB;
#else
    hw.system_memory_bytes = obj->total_memory / BYTES_PER_MB;
#endif
  }
  if(hw.numa_memory_bytes < 0) {
    hwloc_obj_t obj;
    obj = hwloc_get_obj_by_type(topology, DART__HWLOC_OBJ_NUMANODE, 0);
    if(obj != NULL) {
#if HWLOC_API_VERSION < 0x00020000
      hw.numa_memory_bytes = obj->memory.total_memory / BYTES_PER_MB;
#else
      hw.numa_memory_bytes = obj->total_memory / BYTES_PER_MB;
#endif
    } else {
      /* No NUMA domain: */
      hw.numa_memory_bytes = hw.system_memory_bytes;
    }
  }

  hwloc_topology_destroy(topology);
  DART_LOG_TRACE("dart_hwinfo: hwloc: "
                 "num_numa:%d numa_id:%d "
                 "num_cores:%d core_id:%d cpu_id:%d",
                 hw.num_numa, hw.numa_id,
                 hw.num_cores, hw.core_id, hw.cpu_id);
#endif /* DART_ENABLE_HWLOC */

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
    if (hw.num_numa < 0) {
      hw.num_numa    = likwid_getNumberOfNodes();
    }
    if (hw.num_sockets < 0) {
      hw.num_sockets = topo->numSockets;
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

#ifdef DART_ENABLE_PAPI
  DART_LOG_TRACE("dart_hwinfo: using PAPI");

  const PAPI_hw_info_t * papi_hwinfo = NULL;
  if (dart__base__locality__papi_init(&papi_hwinfo) == DART_OK) {
    if (hw.num_numa < 0) {
      hw.num_numa    = papi_hwinfo->nnodes;
    }
    if (hw.num_cores < 0) {
      int num_sockets      = papi_hwinfo->sockets;
      int cores_per_socket = papi_hwinfo->cores;
      hw.num_cores   = num_sockets * cores_per_socket;
    }
    if (hw.min_cpu_mhz < 0 || hw.max_cpu_mhz < 0) {
      hw.min_cpu_mhz = papi_hwinfo->cpu_min_mhz;
      hw.max_cpu_mhz = papi_hwinfo->cpu_max_mhz;
    }
    DART_LOG_TRACE("dart_hwinfo: PAPI: num_numa:%d num_cores:%d",
                   hw.num_numa, hw.num_cores);
  }
#endif /* DART_ENABLE_PAPI */

if (hw.cpu_id < 0) {
  #ifdef DART__PLATFORM__LINUX
    hw.cpu_id = sched_getcpu();
  #elif defined(DART__PLATFORM__OSX)
    hw.cpu_id = osx_sched_getcpu();
  #else
    DART_LOG_ERROR("dart_hwinfo: "
                "HWLOC or PAPI required if not running on a Linux or OSX platform");
    return DART_ERR_OTHER;
  #endif
}

#ifdef DART__ARCH__IS_MIC
  /*
   * Hardware information for Intel MIC can be hard-coded as hardware
   * specs of MIC model variants are invariant:
   */
  DART_LOG_TRACE("dart_hwinfo: MIC architecture");

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

	if(hw.system_memory_bytes < 0) {
	  long pages     = sysconf(_SC_AVPHYS_PAGES);
	  long page_size = sysconf(_SC_PAGE_SIZE);
    if (pages > 0 && page_size > 0) {
      hw.system_memory_bytes = (int) ((pages * page_size) / BYTES_PER_MB);
    }
  }

#endif

#ifdef DART_ENABLE_NUMA
  DART_LOG_TRACE("dart_hwinfo: using numalib");
  if (hw.num_numa < 0) {
    hw.num_numa = numa_max_node() + 1;
  }
  if (hw.numa_id < 0 && hw.cpu_id >= 0) {
    hw.numa_id = numa_node_of_cpu(hw.cpu_id);
  }
#endif

  if (hw.num_scopes < 1) {
    /* No domain hierarchy could be resolved.
     * Use flat topology, with all units assigned to domains in CORE scope: */
    hw.num_scopes = 1;
    hw.scopes[0].scope = DART_LOCALITY_SCOPE_CORE;
    hw.scopes[0].index = (hw.core_id >= 0) ? hw.core_id : hw.cpu_id;
  }

  DART_LOG_TRACE("dart_hwinfo: finished: "
                 "num_numa:%d numa_id:%d cpu_id:%d, num_cores:%d "
                 "min_threads:%d max_threads:%d",
                 hw.num_numa, hw.numa_id, hw.cpu_id, hw.num_cores,
                 hw.min_threads, hw.max_threads);

  *hwinfo = hw;

  DART_LOG_DEBUG("dart_hwinfo >");
  return DART_OK;
}
