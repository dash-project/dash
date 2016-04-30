#ifndef DASH__UTIL__LOCALITY_H__
#define DASH__UTIL__LOCALITY_H__

#include <dash/Init.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_locality.h>

#include <iostream>
#include <string>
#include <vector>
#include <array>


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
    int  rank;
    char host[100];
    int  cpu;
    int  numa_node;
  } UnitPinning;

  typedef enum {
    Undefined = DART_LOCALITY_SCOPE_UNDEFINED,
    Global    = DART_LOCALITY_SCOPE_GLOBAL,
    Node      = DART_LOCALITY_SCOPE_NODE,
    Module    = DART_LOCALITY_SCOPE_MODULE,
    NUMA      = DART_LOCALITY_SCOPE_NUMA,
    Unit      = DART_LOCALITY_SCOPE_UNIT,
    Core      = DART_LOCALITY_SCOPE_CORE
  } Scope;

public:

  static inline int NumNodes() {
    return (_domain_loc == nullptr) ? -1 : _domain_loc->num_nodes;
  }

  static inline int NumSockets() {
    return (_domain_loc == nullptr) ? -1 : _domain_loc->num_sockets;
  }

  static inline int NumNUMANodes() {
    return (_domain_loc == nullptr) ? -1 : _domain_loc->num_numa;
  }

  static inline int NumCPUs() {
    return (_domain_loc == nullptr) ? -1 : _domain_loc->num_cores;
  }

  static inline void SetNumNodes(int n) {
    _domain_loc->num_nodes = n;
    dart_set_domain_locality(_domain_loc);
  }

  static inline void SetNumSockets(int n) {
    if (_unit_loc == nullptr) {
      return;
    }
    _domain_loc->num_sockets = n;
    dart_set_domain_locality(_domain_loc);
  }

  static inline void SetNumNUMANodes(int n) {
    if (_unit_loc == nullptr) {
      return;
    }
    _domain_loc->num_numa = n;
    dart_set_domain_locality(_domain_loc);
  }

  static inline void SetNumCPUs(int n) {
    _domain_loc->num_cores = n;
    dart_set_domain_locality(_domain_loc);
  }

  static std::vector<int> UnitNUMANodes() {
    std::vector<int> numa_ids;
    int num_numa = NumNUMANodes();
    for (int i = 0; i < num_numa; ++i) {
      numa_ids.push_back(_domain_loc->numa_ids[i]);
    }
    return numa_ids;
  }

  static std::vector<int> UnitCPUIds() {
    std::vector<int> cpu_ids;
    int num_cores = NumCPUs();
    for (int i = 0; i < num_cores; ++i) {
      cpu_ids.push_back(_unit_loc->cpu_ids[i]);
    }
    return cpu_ids;
  }

  static inline int CPUMaxMhz() {
    return (_unit_loc == nullptr) ? -1 : _unit_loc->max_cpu_mhz;
  }

  static inline int CPUMinMhz() {
    return (_unit_loc == nullptr) ? -1 : _unit_loc->min_cpu_mhz;
  }

  static inline std::string Hostname() {
    return (_domain_loc == nullptr) ? "" : _domain_loc->host;
  }

  static inline std::string Hostname(dart_unit_t unit_id) {
    return _unit_pinning[unit_id].host;
  }

  static const std::vector<UnitPinning> & Pinning() {
    return _unit_pinning;
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

  static std::vector<UnitPinning>   _unit_pinning;
};

std::ostream & operator<<(
  std::ostream & os,
  const typename dash::util::Locality::UnitPinning & upi);

} // namespace util
} // namespace dash

#endif // DASH__UTIL__LOCALITY_H__
