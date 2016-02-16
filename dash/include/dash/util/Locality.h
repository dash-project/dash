#ifndef DASH__UTIL__LOCALITY_H__
#define DASH__UTIL__LOCALITY_H__

#include <libdash.h>
#include <unistd.h>

#ifdef DASH_ENABLE_NUMA
#include <utmpx.h>
#include <numa.h>
#endif

// #include <hwloc.h>
// #include <hwloc/helper.h>

namespace dash {
namespace util {

class Locality
{
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
      "dash::util::Locality::NumCPUs required libnuma or POSIX platform")
#endif
#endif
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
      "dash::util::Locality::UnitCPU is only available on Linux platforms")
#endif
  }

  static inline std::string Hostname() {
    char host_cstr[100];
    gethostname(host_cstr, 100);
    return std::string(host_cstr);
  }

};

} // namespace util
} // namespace dash

#endif // DASH__UTIL__LOCALITY_H__
