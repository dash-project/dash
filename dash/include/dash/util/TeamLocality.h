#ifndef DASH__UTIL__TEAM_LOCALITY_H__INCLUDED
#define DASH__UTIL__TEAM_LOCALITY_H__INCLUDED

#include <dash/util/Locality.h>
#include <dash/util/LocalityDomain.h>
#include <dash/util/UnitLocality.h>

#include <dash/algorithm/internal/String.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_locality.h>

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
 * size_t num_module_domains = tloc.parts().size();
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

  TeamLocality(
    dash::Team & team,
    Scope_t      scope      = Scope_t::Global,
    std::string  domain_tag = ".")
  : _team(&team),
    _scope(scope)
  {
    dart_domain_locality_t * domain;
    DASH_ASSERT_RETURNS(
      dart_domain_team_locality(
        _team->dart_id(),
        domain_tag.c_str(),
        &domain),
      DART_OK);

    _domains.push_back(LocalityDomain_t(domain));

    if (_scope != Scope_t::Global) {
      split(_scope);
    }
  }

  TeamLocality()                           = default;
  TeamLocality(const self_t & other)       = default;

  self_t & operator=(const self_t & other) = default;

  inline std::vector<LocalityDomain_t> & parts()
  {
    return _domains;
  }

  inline const std::vector<LocalityDomain_t> & parts() const
  {
    return _domains;
  }

  inline dash::Team & team()
  {
    return (nullptr == _team) ? dash::Team::Null() : *_team;
  }

  inline const std::vector<dart_unit_t> & units() const
  {
    return _unit_ids;
  }

  self_t & split(Scope_t scope, int num_parts = 0)
  {
    DASH_LOG_DEBUG_VAR("TeamLocality.split()", num_parts);

    if (static_cast<int>(_scope) > static_cast<int>(scope)) {
      // Cannot split into higher scope
      DASH_THROW(
        dash::exception::InvalidArgument,
        "Cannot split LocalityDomain at scope " << _scope << " "
        "into a parent scope (got: " << scope << ")");
    }
    _scope = scope;
    _domains.clear();
    _unit_ids.clear();

    int     num_domains;
    char ** domain_tags;
    DASH_ASSERT_RETURNS(
      dart_scope_domains(
        _domain,
        static_cast<dart_locality_scope_t>(_scope),
        &num_domains,
        &domain_tags),
      DART_OK);
    free(domain_tags);

    if (num_parts < 1 || num_domains <= num_parts) {
      DASH_LOG_DEBUG("TeamLocality.split",
                     "split into single subdomains");

      dart_domain_locality_t * subdomains =
        new dart_domain_locality_t[num_domains];

      DASH_ASSERT_RETURNS(
        dart_domain_split(
          domain,
          static_cast<dart_locality_scope_t>(_scope),
          num_domains,
          subdomains),
        DART_OK);
      for (int sd = 0; sd < num_domains; ++sd) {
        DASH_LOG_TRACE_VAR("TeamLocality.split", subdomains[sd].domain_tag);
        _domains.push_back(LocalityDomain(&subdomains[sd]));
      }
    } else {
      DASH_LOG_DEBUG("TeamLocality.split",
                     "split into groups of subdomains");

      dart_domain_locality_t * split_domains =
        new dart_domain_locality_t[num_parts];

      DASH_ASSERT_RETURNS(
        dart_domain_split(
          domain,
          static_cast<dart_locality_scope_t>(_scope),
          num_parts,
          split_domains),
        DART_OK);

      for (int sd = 0; sd < num_parts; ++sd) {
        DASH_LOG_TRACE_VAR("TeamLocality.split",
                           split_domains[sd].domain_tag);
        _domains.push_back(LocalityDomain(&split_domains[sd]));
      }
    }

    for (auto & domain : _domains) {
      _unit_ids.insert(_unit_ids.end(),
                       domain.units().begin(),
                       domain.units().end());
    }

    DASH_LOG_DEBUG("TeamLocality.split >");

    return *this;
  }

  self_t & group(
    std::initializer_list<std::string> group_subdomain_tags)
  {
    for (auto domain : _domains) {
      domain.group(group_domain_tags);
    }
    return *this;
  }

  self_t & select(
    std::initializer_list<std::string> domain_tags)
  {
    for (auto domain : _domains) {
      domain.select(domain_tags);
    }
    return *this;
  }

  self_t & exclude(
    std::initializer_list<std::string> domain_tags)
  {
    for (auto domain : _domains) {
      domain.exclude(domain_tags);
    }
    return *this;
  }

private:
  dash::Team                        * _team          = nullptr;
  /// Parent scope of the team locality domain hierarchy.
  Scope_t                             _scope         = Scope_t::Undefined;
  /// Domains in the team locality, one domain for every split group.
  std::vector<LocalityDomain_t>       _domains;
  /// Units in the domain.
  std::vector<dart_unit_t>            _unit_ids;

}; // class TeamLocality

}  // namespace util
}  // namespace dash

#endif // DASH__UTIL__TEAM_LOCALITY_H__INCLUDED
