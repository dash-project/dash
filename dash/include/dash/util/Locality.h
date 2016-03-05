#ifndef DASH__UTIL__LOCALITY_H__
#define DASH__UTIL__LOCALITY_H__

#include <dash/internal/Config.h>
#include <dash/Exception.h>
#include <dash/Init.h>

#include <iostream>
#include <unistd.h>
#include <string>
#include <vector>
#include <array>

#ifdef DASH_ENABLE_NUMA
#include <utmpx.h>
#include <numa.h>
#endif

#ifdef DASH_ENABLE_HWLOC
#include <hwloc.h>
#include <hwloc/helper.h>
#endif

#ifdef DASH_ENABLE_PAPI
#include <papi.h>
#endif

namespace dash {
namespace util {

template<
  typename ElementType,
  typename IndexType,
  class PatternType>
class Array;

class Locality
{
public:
  friend void dash::init(int *argc, char ***argv);

public:
  typedef struct {
    int  rank;
    char host[100];
    int  cpu;
    int  numa_node;
  } UnitPinning;

public:

  static inline int NumNodes() {
    return std::max<int>(dash::size() / NumCPUs(), 1);
  }

  static inline int NumSockets() {
    return _num_sockets;
  }

  static inline int NumNumaNodes() {
    return _num_numa;
  }

  static inline int NumCPUs() {
    return _num_cpus;
  }

  static inline void SetNumNodes(int n) {
    _num_nodes = n;
  }

  static inline void SetNumSockets(int n) {
    _num_sockets = n;
  }

  static inline void SetNumNumaNodes(int n) {
    _num_numa = n;
  }

  static inline void SetNumCPUs(int n) {
    _num_cpus = n;
  }

  static inline int MyNUMANode() {
    int numa_node = -1;
#ifdef DASH_ENABLE_NUMA
    int cpu;
    cpu       = sched_getcpu();
    numa_node = numa_node_of_cpu(cpu);
#endif
    return numa_node;
  }

  static inline int MyCPU() {
#ifdef DASH__PLATFORM__LINUX
    return sched_getcpu();
#endif
    DASH_THROW(
      dash::exception::NotImplemented,
      "dash::util::Locality::UnitCPU is only available on Linux platforms");
  }

  static inline int CPUMaxMhz() {
#ifdef DASH_ENABLE_PAPI
    const PAPI_hw_info_t * hwinfo = PAPI_get_hardware_info();
    if (hwinfo == NULL) {
      DASH_THROW(
        dash::exception::RuntimeError,
        "PAPI get hardware info failed");
    }
    return hwinfo->cpu_max_mhz;
#endif
    return -1;
  }

  static inline int CPUMinMhz() {
#ifdef DASH_ENABLE_PAPI
    const PAPI_hw_info_t * hwinfo = PAPI_get_hardware_info();
    if (hwinfo == NULL) {
      DASH_THROW(
        dash::exception::RuntimeError,
        "PAPI get hardware info failed");
    }
    return hwinfo->cpu_min_mhz;
#endif
    return -1;
  }

  static inline std::string Hostname() {
    return Hostname(dash::myid());
  }

  static inline std::string Hostname(dart_unit_t unit_id) {
    return _unit_pinning[unit_id].host;
  }

  static const std::vector<UnitPinning> & Pinning() {
    return _unit_pinning;
  }

  static const std::array<size_t, 3> & CacheSizes() {
    return _cache_sizes;
  }

  static const std::array<size_t, 3> & CacheLineSizes() {
    return _cache_line_sizes;
  }

private:
  static void init();

private:
  static int                      _num_nodes;
  static int                      _num_sockets;
  static int                      _num_numa;
  static int                      _num_cpus;
  static std::vector<UnitPinning> _unit_pinning;
  static std::array<size_t, 3>    _cache_sizes;
  static std::array<size_t, 3>    _cache_line_sizes;
};

std::ostream & operator<<(
  std::ostream        & os,
  const typename dash::util::Locality::UnitPinning & upi);

} // namespace util
} // namespace dash

#endif // DASH__UTIL__LOCALITY_H__
