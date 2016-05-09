#ifndef DASH__UTIL__LOCALITY_H__
#define DASH__UTIL__LOCALITY_H__

#include <dash/Init.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_locality.h>

#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <cstring>


std::ostream & operator<<(
  std::ostream & os,
  const dart_domain_locality_t & domain_loc);

std::ostream & operator<<(
  std::ostream & os,
  const dart_unit_locality_t & unit_loc);


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
    int  unit;
    char host[40];
    char domain[20];
    int  cpu_id;
    int  num_cores;
    int  numa_id;
    int  num_threads;
  } UnitPinning;

  typedef enum {
    Undefined = DART_LOCALITY_SCOPE_UNDEFINED,
    Global    = DART_LOCALITY_SCOPE_GLOBAL,
    Network   = DART_LOCALITY_SCOPE_NETWORK,
    Node      = DART_LOCALITY_SCOPE_NODE,
    Module    = DART_LOCALITY_SCOPE_MODULE,
    NUMA      = DART_LOCALITY_SCOPE_NUMA,
    Unit      = DART_LOCALITY_SCOPE_UNIT,
    Package   = DART_LOCALITY_SCOPE_PACKAGE,
    Uncore    = DART_LOCALITY_SCOPE_UNCORE,
    Core      = DART_LOCALITY_SCOPE_CORE,
    CPU       = DART_LOCALITY_SCOPE_CPU
  } Scope;

public:

  static inline int NumNodes() {
    return (_domain_loc == nullptr) ? -1 : _domain_loc->num_nodes;
  }

  static inline int NumSockets() {
    return (_domain_loc == nullptr) ? -1 : _domain_loc->hwinfo.num_sockets;
  }

  static inline int NumNUMANodes() {
    return (_domain_loc == nullptr) ? -1 : _domain_loc->hwinfo.num_numa;
  }

  static inline int NumCPUs() {
    return (_domain_loc == nullptr) ? -1 : _domain_loc->hwinfo.num_cores;
  }

  static inline void SetNumNodes(int n) {
    _domain_loc->num_nodes = n;
  }

  static inline void SetNumSockets(int n) {
    if (_unit_loc == nullptr) {
      return;
    }
    _domain_loc->hwinfo.num_sockets = n;
  }

  static inline void SetNumNUMANodes(int n) {
    if (_unit_loc == nullptr) {
      return;
    }
    _domain_loc->hwinfo.num_numa = n;
  }

  static inline void SetNumCPUs(int n) {
    _domain_loc->hwinfo.num_cores = n;
  }

  static int UnitNUMAId() {
    return _domain_loc->hwinfo.numa_id;
  }

  static int UnitCPUId() {
    return _domain_loc->hwinfo.cpu_id;
  }

  static inline int CPUMaxMhz() {
    return (_unit_loc == nullptr) ? -1 : _unit_loc->hwinfo.max_cpu_mhz;
  }

  static inline int CPUMinMhz() {
    return (_unit_loc == nullptr) ? -1 : _unit_loc->hwinfo.min_cpu_mhz;
  }

  static inline std::string Hostname() {
    return (_domain_loc == nullptr) ? "" : _domain_loc->host;
  }

  static inline std::string Hostname(dart_unit_t unit) {
    dart_unit_locality_t * ul;
    dart_unit_locality(DART_TEAM_ALL, unit, &ul);
    return ul->host;
  }

  static const UnitPinning Pinning(dart_unit_t unit) {
    dart_unit_locality_t * ul;
    dart_unit_locality(DART_TEAM_ALL, unit, &ul);
    UnitPinning pinning;
    pinning.unit        = ul->unit;
    pinning.num_cores   = ul->hwinfo.num_cores;
    pinning.cpu_id      = ul->hwinfo.cpu_id;
    pinning.numa_id     = ul->hwinfo.numa_id;
    pinning.num_threads = ul->hwinfo.max_threads;
    strncpy(pinning.host,   ul->host,       40);
    strncpy(pinning.domain, ul->domain_tag, 20);
    return pinning;
  }

  static const std::array<int, 3> & CacheSizes() {
    return _cache_sizes;
  }

  static const std::array<int, 3> & CacheLineSizes() {
    return _cache_line_sizes;
  }

private:
  static void init();

private:
  static dart_unit_locality_t     * _unit_loc;
  static dart_domain_locality_t   * _domain_loc;

  static std::array<int, 3>         _cache_sizes;
  static std::array<int, 3>         _cache_line_sizes;
};

std::ostream & operator<<(
  std::ostream & os,
  const typename dash::util::Locality::UnitPinning & upi);

} // namespace util
} // namespace dash

#endif // DASH__UTIL__LOCALITY_H__
