#ifndef DASH__UTIL__LOCALITY_H__
#define DASH__UTIL__LOCALITY_H__

#include <dash/Init.h>

#include <dash/util/Config.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_locality.h>

#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <cstring>


std::ostream & operator<<(
  std::ostream                 & os,
  const dart_domain_locality_t & domain_loc);

std::ostream & operator<<(
  std::ostream                 & os,
  const dart_unit_locality_t   & unit_loc);

namespace dash {

namespace util {

class Locality
{
public:
  friend void dash::init(int *argc, char ***argv);

public:

  typedef enum
  {
    Undefined = DART_LOCALITY_SCOPE_UNDEFINED,
    Global    = DART_LOCALITY_SCOPE_GLOBAL,
    Group     = DART_LOCALITY_SCOPE_GROUP,
    Network   = DART_LOCALITY_SCOPE_NETWORK,
    Node      = DART_LOCALITY_SCOPE_NODE,
    Module    = DART_LOCALITY_SCOPE_MODULE,
    NUMA      = DART_LOCALITY_SCOPE_NUMA,
    Unit      = DART_LOCALITY_SCOPE_UNIT,
    Package   = DART_LOCALITY_SCOPE_PACKAGE,
    Uncore    = DART_LOCALITY_SCOPE_UNCORE,
    Cache     = DART_LOCALITY_SCOPE_CACHE,
    Core      = DART_LOCALITY_SCOPE_CORE,
    CPU       = DART_LOCALITY_SCOPE_CPU
  }
  Scope;

public:

  static inline int NumNodes()
  {
    return (_team_loc == nullptr)
//         ? -1 : std::max<int>(_team_loc->num_nodes, 1);
           ? -1 : std::max<int>(_team_loc->num_domains, 1);
  }


private:
  static void init();

private:
  static dart_unit_locality_t     * _unit_loc;
  static dart_domain_locality_t   * _team_loc;

};

} // namespace util
} // namespace dash

std::ostream & operator<<(
  std::ostream                 & os,
  dash::util::Locality::Scope    scope);

std::ostream & operator<<(
  std::ostream                 & os,
  dart_locality_scope_t          scope);

#endif // DASH__UTIL__LOCALITY_H__
