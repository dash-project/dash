#ifndef DASH__UTIL__TEAM_LOCALITY_H__INCLUDED
#define DASH__UTIL__TEAM_LOCALITY_H__INCLUDED

#include <dash/util/Locality.h>
#include <dash/util/LocalityDomain.h>
#include <dash/util/UnitLocality.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_locality.h>

#include <dash/algorithm/internal/String.h>

#include <dash/Exception.h>
#include <dash/Team.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
#include <iterator>
#include <algorithm>


namespace dash {
namespace util {

/**
 * Hierarchical locality domains of a specified team.
 *
 * Usage examples:
 *
 * \code
 * dash::Team & team = dash::Team::All();
 *
 * dash::util::TeamLocality tloc(team);
 *
 * // Team locality at first node, split at module scope:
 * tloc.select(".0")
 *     .split(dash::util::Locality::Scope::Module);
 *
 * size_t num_module_split_domains = tloc.parts().size();
 * for (dash::util::LocalityDomain domain : tloc.parts()) {
 *   int module_index            = domain.relative_index();
 *   int domain_max_core_mhz     = domain.hwinfo().max_cpu_mhz;
 *   int domain_min_core_threads = domain.hwinfo().min_threads;
 *   int domain_core_perf        = domain_max_core_mhz *
 *                                 domain_min_core_threads;
 *
 *   size_t num_module_units = domain.units().size();
 *   for (dart_unit_t module_unit_id : domain.unit_ids()) {
 *     dash::util::UnitLocality uloc(team, module_unit_id);
 *
 *     std::string unit_host = uloc.host();
 *     int unit_numa_id      = uloc.hwinfo().numa_id;
 *     int unit_num_cores    = uloc.hwinfo().num_cores;
 *     int unit_num_threads  = uloc.hwinfo().max_threads * unit_num_cores;
 *   }
 * }
 * \endcode
 */
class TeamLocality
{
private:
  typedef TeamLocality                    self_t;
  typedef dash::util::Locality::Scope     Scope_t;
  typedef dash::util::LocalityDomain      LocalityDomain_t;

public:
  /**
   * Constructor. Creates new instance of \c dash::util::TeamLocality for
   * a specified team.
   */
  TeamLocality(
    dash::Team & team,
    Scope_t      scope      = Scope_t::Global,
    std::string  domain_tag = ".");

  /**
   * Default constructor.
   */
  TeamLocality()                           = default;

  /**
   * Copy constructor.
   */
  TeamLocality(const self_t & other)       = default;

  /**
   * Assignment operator.
   */
  self_t & operator=(const self_t & other) = default;

  self_t & split(Scope_t scope, int num_parts = 0);

  inline std::vector<LocalityDomain_t> & parts()
  {
    return _split_domains;
  }

  inline const std::vector<LocalityDomain_t> & parts() const
  {
    return _split_domains;
  }

  inline dash::Team & team()
  {
    return (nullptr == _team) ? dash::Team::Null() : *_team;
  }

  inline const std::vector<dart_unit_t> & units() const
  {
    return _unit_ids;
  }

  inline self_t & group(
    std::initializer_list<std::string> group_subdomain_tags)
  {
    for (auto domain : _split_domains) {
      domain.group(group_subdomain_tags);
    }
    return *this;
  }

  inline self_t & select(
    std::initializer_list<std::string> domain_tags)
  {
    for (auto domain : _split_domains) {
      domain.select(domain_tags);
    }
    return *this;
  }

  inline self_t & exclude(
    std::initializer_list<std::string> domain_tags)
  {
    for (auto domain : _split_domains) {
      domain.exclude(domain_tags);
    }
    return *this;
  }

private:
  dash::Team                        * _team          = nullptr;
  /// Parent scope of the team locality domain hierarchy.
  Scope_t                             _scope         = Scope_t::Undefined;
  /// Split domains in the team locality, one domain for every split group.
  std::vector<LocalityDomain_t>       _split_domains;
  /// Locality domain of the team.
  LocalityDomain_t                    _domain;
  /// Units in the domain.
  std::vector<dart_unit_t>            _unit_ids;

}; // class TeamLocality

}  // namespace util
}  // namespace dash

#endif // DASH__UTIL__TEAM_LOCALITY_H__INCLUDED
