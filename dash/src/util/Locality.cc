#include <dash/util/Locality.h>

#include <dash/Array.h>
#include <dash/algorithm/Copy.h>

#include <iostream>
#include <iomanip>
#include <sstream>
#include <vector>
#include <string>
#include <cstring>

#ifdef DASH_ENABLE_HWLOC
#include <hwloc.h>
#include <hwloc/helper.h>
#endif

namespace dash {
namespace util {

void Locality::init()
{
  _cache_sizes[0]      = -1;
  _cache_sizes[1]      = -1;
  _cache_sizes[2]      = -1;
  _cache_line_sizes[0] = -1;
  _cache_line_sizes[1] = -1;
  _cache_line_sizes[2] = -1;
  _num_nodes           = -1;
  _num_sockets         = -1;
  _num_numa            = -1;
  _num_cpus            = -1;
#ifdef DASH_ENABLE_HWLOC
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
      _cache_sizes[level]      = obj->attr->cache.size;
      _cache_line_sizes[level] = obj->attr->cache.linesize;
      ++level;
    }
  }
  // Resolve number of sockets:
  int depth = hwloc_get_type_depth(topology, HWLOC_OBJ_SOCKET);
  if (depth != HWLOC_TYPE_DEPTH_UNKNOWN) {
    _num_sockets = hwloc_get_nbobjs_by_depth(topology, depth);
  }
	// Resolve number of NUMA nodes:
  int n_numa_nodes = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_NODE);
  if (n_numa_nodes > 0) {
    _num_numa = n_numa_nodes;
  }
	// Resolve number of cores per numa:
	int n_cores = hwloc_get_nbobjs_by_type(topology, HWLOC_OBJ_CORE);
	if (n_cores > 0){
		_num_cpus = n_cores;
	}
  hwloc_topology_destroy(topology);
#endif
#ifdef DASH_ENABLE_PAPI
  int retval = PAPI_library_init(PAPI_VER_CURRENT);
  if (retval != PAPI_VER_CURRENT && retval > 0) {
    DASH_LOG_ERROR(
      "dash::util::Locality::init(): PAPI version mismatch");
  }
  else if (retval < 0) {
    DASH_LOG_ERROR(
      "dash::util::Locality::init(): PAPI init failed");
  } else {
    const PAPI_hw_info_t * hwinfo = PAPI_get_hardware_info();
    if (hwinfo == NULL) {
      DASH_LOG_ERROR(
        "dash::util::Locality::init(): PAPI get hardware info failed");
    } else {
      if (_num_sockets < 0) {
        _num_sockets = hwinfo->sockets;
      }
      if (_num_numa < 0) {
        _num_numa = hwinfo->nnodes;
      }
      if (_num_cpus < 0) {
        auto cores_per_socket = hwinfo->cores;
        _num_cpus = _num_sockets * cores_per_socket;
				DASH_LOG_DEBUG("_num_cpus first got by PAPI", _num_cpus);
      }
    }
  }
#endif
#ifdef DASH__PLATFORM__POSIX
  if (_num_cpus < 0) {
		// be careful: includes hyperthreading
    int ret = sysconf(_SC_NPROCESSORS_ONLN);
    _num_cpus = (ret > 0) ? ret : _num_cpus;
		DASH_LOG_DEBUG("_num_cpus first got by DASH__PLATFORM_POSIX", _num_cpus);
  }
#endif
#ifdef DASH_ENABLE_NUMA
  if (_num_numa < 0) {
    _num_numa = numa_max_node() + 1;
  }
#endif
  if (_num_nodes < 0 && _num_cpus > 0) {
    _num_nodes = std::max<int>(dash::size() / _num_cpus, 1);
  }
  if (_num_nodes   < 0) { _num_nodes   = 1; }
  if (_num_sockets < 0) { _num_sockets = 1; }
  if (_num_numa    < 0) { _num_numa    = 1; }
  if (_num_cpus    < 0) { _num_cpus    = 1; }

  // Collect process pinning information:
  dash::Array<UnitPinning> pinning(dash::size());

  int cpu       = dash::util::Locality::MyCPU();
  int numa_node = dash::util::Locality::MyNUMANode();

  UnitPinning my_pin_info;
  my_pin_info.rank      = dash::myid();
  my_pin_info.cpu       = cpu;
  my_pin_info.numa_node = numa_node;
  gethostname(my_pin_info.host, 100);

  pinning[dash::myid()] = my_pin_info;

  // Ensure pinning data is ready:
  dash::barrier();

  // Create local copy of pinning info:

  // TODO:
  // Change to directly copying to local_vector.begin()
  // when dash::copy is available for iterator output
  // ranges.

  // Copy into temporary array:
  UnitPinning * local_copy_tmp = new UnitPinning[pinning.size()];

  auto copy_end = dash::copy(pinning.begin(), pinning.end(),
                             local_copy_tmp);
  auto n_copied = copy_end - local_copy_tmp;
  DASH_LOG_TRACE_VAR("dash::util::Locality::init", n_copied);
  // Copy from temporary array to local vector:
  _unit_pinning.insert(_unit_pinning.end(),
                       local_copy_tmp, local_copy_tmp + n_copied);
  // Free temporary array:
  delete[] local_copy_tmp;

  // Wait for completion of the other units' copy operations:
  dash::barrier();
}

std::ostream & operator<<(
  std::ostream        & os,
  const typename Locality::UnitPinning & upi)
{
  std::ostringstream ss;
  ss << "dash::util::Locality::UnitPinning("
     << "rank:" << upi.rank << " "
     << "host:" << upi.host << " "
     << "cpu:"  << upi.cpu  << " "
     << "numa:" << upi.numa_node << ")";
  return operator<<(os, ss.str());
}

int Locality::_num_nodes   = -1;
int Locality::_num_sockets = -1;
int Locality::_num_numa    = -1;
int Locality::_num_cpus    = -1;
std::vector<Locality::UnitPinning> Locality::_unit_pinning;
std::array<int, 3>                 Locality::_cache_sizes;
std::array<int, 3>                 Locality::_cache_line_sizes;

} // namespace util
} // namespace dash
