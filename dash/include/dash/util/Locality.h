#ifndef DASH__UTIL__LOCALITY_H__
#define DASH__UTIL__LOCALITY_H__

#include <dash/Init.h>
#include <dash/util/Config.h>

#include <dylocxx.h>

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
    Undefined = DYLOC_LOCALITY_SCOPE_UNDEFINED,
    Global    = DYLOC_LOCALITY_SCOPE_GLOBAL,
    Group     = DYLOC_LOCALITY_SCOPE_GROUP,
    Network   = DYLOC_LOCALITY_SCOPE_NETWORK,
    Node      = DYLOC_LOCALITY_SCOPE_NODE,
    Module    = DYLOC_LOCALITY_SCOPE_MODULE,
    NUMA      = DYLOC_LOCALITY_SCOPE_NUMA,
    Unit      = DYLOC_LOCALITY_SCOPE_UNIT,
    Package   = DYLOC_LOCALITY_SCOPE_PACKAGE,
    Uncore    = DYLOC_LOCALITY_SCOPE_UNCORE,
    Cache     = DYLOC_LOCALITY_SCOPE_CACHE,
    Core      = DYLOC_LOCALITY_SCOPE_CORE,
    CPU       = DYLOC_LOCALITY_SCOPE_CPU
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
