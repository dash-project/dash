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

  static inline int NumNumaNodes() {
#ifdef DASH_ENABLE_NUMA
    return numa_max_node() + 1;
#else
    return 1;
#endif
  }

  static inline int NumCPUs() {
#ifdef DASH_ENABLE_NUMA
    // Number of physical cores on this system, divided by 2 to
    // eliminate HT cores:
    return numa_num_configured_cpus() / 2;
#else
#ifdef DASH__PLATFORM__POSIX
    int ret = sysconf(_SC_NPROCESSORS_ONLN);
    return (ret > 0) ? ret : 1;
#else
    DASH_THROW(
      dash::exception::NotImplemented,
      "dash::util::Locality::NumCPUs required libnuma or POSIX platform");
#endif
#endif
  }

  static inline int NumNodes() {
    return std::max<int>(dash::size() / NumCPUs(), 1);
  }

  static inline int UnitNUMANode() {
    int numa_node = 0;
#ifdef DASH_ENABLE_NUMA
    int cpu;
    cpu       = sched_getcpu();
    numa_node = numa_node_of_cpu(cpu);
#endif
    return numa_node;
  }

  static inline int UnitCPU() {
#ifdef DASH__PLATFORM__LINUX
    return sched_getcpu();
#else
    DASH_THROW(
      dash::exception::NotImplemented,
      "dash::util::Locality::UnitCPU is only available on Linux platforms");
#endif
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
