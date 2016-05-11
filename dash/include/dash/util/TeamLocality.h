#ifndef DASH__UTIL__TEAM_LOCALITY_H__INCLUDED
#define DASH__UTIL__TEAM_LOCALITY_H__INCLUDED

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_locality.h>

#include <dash/util/Locality.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
#include <iterator>


namespace dash {
namespace util {

/**
 * Wrapper of a single \c dart_domain_locality_t object.
 */
class LocalityDomain
{
private:
  typedef LocalityDomain                           self_t;
  typedef dash::util::Locality::Scope     LocalityScope_t;

public:
  /**
   * Iterator on subdomains of the locality domain.
   */
  class iterator
  : public std::iterator< std::random_access_iterator_tag, LocalityDomain >
  {
  private:
    typedef iterator                           self_t;

  public:
    typedef iterator                        self_type;
    typedef int                       difference_type;

    typedef       LocalityDomain           value_type;
    typedef       LocalityDomain *            pointer;
    typedef const LocalityDomain *      const_pointer;
    typedef       LocalityDomain &          reference;
    typedef const LocalityDomain &    const_reference;

  public:
    iterator(
      const LocalityDomain & domain,
      int                    subdomain_idx = 0)
    : _domain(&domain),
      _idx(subdomain_idx)
    { }

    iterator() = default;

    bool operator==(const self_type & rhs) const {
      return *_domain == *(rhs._domain) && _idx == rhs._idx;
    }

    bool operator!=(const self_type & rhs) const {
      return !(*this == rhs);
    }

    const_reference operator[](int i)
    {
      int subdomain_idx = _idx + i;
      return _domain->at(subdomain_idx);
    }

    const_reference operator*() {
      return _domain->at(_idx);
    }

    const_pointer operator->() {
      return &(_domain->at(_idx));
    }

    self_t & operator++()      { _idx++;   return *this; }
    self_t & operator--()      { _idx--;   return *this; }
    self_t & operator+=(int i) { _idx+= i; return *this; }
    self_t & operator-=(int i) { _idx-= i; return *this; }
    self_t   operator+(int i)  { self_t ret = *this; ret += i; return ret; }
    self_t   operator-(int i)  { self_t ret = *this; ret -= i; return ret; }
    self_t   operator++(int)   { self_t ret = *this; _idx++;   return ret; }
    self_t   operator--(int)   { self_t ret = *this; _idx--;   return ret; }

  private:
    const LocalityDomain * _domain = nullptr;
    int                    _idx    = 0;

  }; // class LocalityDomain::iterator

public:

  LocalityDomain()
  : _team(nullptr)
  { }

  LocalityDomain(
    dash::Team  & team,
    std::string   domain_tag)
  : _team(&team)
  {
    DASH_ASSERT_RETURNS(
      dart_domain_locality(
        team.dart_id(),
        domain_tag.c_str(),
        &_domain_locality),
      DART_OK);
    DASH_ASSERT_MSG(
      _domain_locality != NULL &&
      _domain_locality != nullptr,
      "Failed to load locality domain with tag " << domain_tag);

    if (_domain_locality->num_units > 0) {
      _unit_ids = std::vector<dart_unit_t>(
                    _domain_locality->unit_ids,
                    _domain_locality->unit_ids + _domain_locality->num_units);
    }

    _begin = iterator(*this, 0);
    _end   = iterator(*this, _domain_locality->num_domains);

    _domains = new std::unordered_map<int, self_t>();
  }

  ~LocalityDomain()
  {
    delete _domains;
  }

  inline bool operator==(const self_t & rhs) const
  {
    return *_team                       == *rhs._team &&
           _domain_locality->team       == rhs._domain_locality->team &&
           _domain_locality->domain_tag == rhs._domain_locality->domain_tag;
  }

  inline bool operator!=(const self_t & rhs) const
  {
    return !(*this == rhs);
  }

  inline dash::Team & team()
  {
    return *_team;
  }

  inline std::string domain_tag() const
  {
    return _domain_locality->domain_tag;
  }

  inline std::string host() const
  {
    return _domain_locality->host;
  }

  inline iterator begin() const
  {
    return _begin;
  }

  inline iterator end() const
  {
    return _end;
  }

  inline const std::vector<dart_unit_t> & units() const
  {
    return _unit_ids;
  }

  inline const dart_hwinfo_t & hwinfo() const
  {
    return _domain_locality->hwinfo;
  }

  inline int level() const
  {
    return _domain_locality->level;
  }

  inline LocalityScope_t scope() const
  {
    return static_cast<LocalityScope_t>(_domain_locality->scope);
  }

  inline int node_id() const
  {
    return _domain_locality->node_id;
  }

  inline int num_nodes() const
  {
    return _domain_locality->num_nodes;
  }

  inline int relative_index() const
  {
    return _domain_locality->relative_index;
  }

  /**
   * Lazy-load instance of \c LocalityDomain for subdomain.
   */
  inline self_t & at(int relative_index) const
  {
    if (_domains->find(relative_index) == _domains->end()) {
      // LocalityDomain instance for subdomain not cached yet:
      std::string subdomain_tag(
                    _domain_locality->domains[relative_index].domain_tag);
      self_t      subdomain_loc(*_team, subdomain_tag);
      _domains->insert(std::make_pair(relative_index, subdomain_loc));
    }
    return (*_domains)[relative_index];
  }

private:
  dash::Team                               * _team;
  dart_domain_locality_t                   * _domain_locality = nullptr;

  iterator                                   _begin;
  iterator                                   _end;

  /// Units in the domain.
  std::vector<dart_unit_t>                   _unit_ids;
  /// Cache of lazy-loaded subdomains, mapped by subdomain relative index.
  mutable std::unordered_map<int, self_t>  * _domains;

}; // class LocalityDomain

/**
 * Wrapper of a single \c dart_unit_locality_t object.
 */
class UnitLocality
{
private:
  typedef UnitLocality  self_t;

public:

  UnitLocality(
    dash::Team  & team,
    dart_unit_t   unit)
  : _team(team)
  {
    DASH_ASSERT_RETURNS(
      dart_unit_locality(
        team.dart_id(), unit, &_unit_locality),
      DART_OK);
  }

  UnitLocality() = delete;

  inline const dart_hwinfo_t & hwinfo() const
  {
    return _unit_locality->hwinfo;
  }

  inline dash::Team & team()
  {
    return _team;
  }

  inline dart_unit_t unit_id() const
  {
    return _unit_locality->unit;
  }

  inline std::string domain_tag() const
  {
    return _unit_locality->domain_tag;
  }

  inline std::string host() const
  {
    return _unit_locality->host;
  }

private:
  dash::Team             & _team;
  dart_unit_locality_t   * _unit_locality = nullptr;

}; // class UnitLocality

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
 * size_t num_module_domains = tloc.domains().size();
 * for (dash::util::LocalityDomain domain : tloc.domains()) {
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
  typedef dash::util::Locality::Scope     LocalityScope_t;

public:

  TeamLocality(
    dash::Team      & team,
    LocalityScope_t   scope      = LocalityScope_t::Global,
    std::string       domain_tag = ".")
  : _team(team),
    _scope(scope),
    _domain_tag(domain_tag)
  {
    if (_domain_tag != ".") {
      select(_domain_tag);
    }
    if (_scope == LocalityScope_t::Global) {
      // Load the team's global locality domain hierarchy:
      _domains.push_back(LocalityDomain(_team, _domain_tag));
    } else {
      split(_scope);
    }
  }

  TeamLocality() = delete;

  self_t & select(std::string domain_tag)
  {
    if (_domain_tag == domain_tag) {
      return *this;
    }
    if (domain_tag.find(_domain_tag) != 0) {
      // Current domain tag is not parent of domain tag to select:
      DASH_THROW(
        dash::exception::InvalidArgument,
        "Selected domain "     << domain_tag << " is not a subdomain of "
        "TeamLocality domain " << _domain_tag);
    }
    _domain_tag = domain_tag;
    _domains.clear();
    _unit_ids.clear();

    _domains.push_back(LocalityDomain(_team, _domain_tag));

    for (auto & domain : _domains) {
      _unit_ids.insert(_unit_ids.end(),
                       domain.units().begin(),
                       domain.units().end());
    }

    return *this;
  }

  self_t & split(LocalityScope_t scope)
  {
    if (_scope == scope) {
      // TODO: Scope could be ambiguous, needs additional check of level
      return *this;
    }
    if (static_cast<int>(_scope) < static_cast<int>(scope)) {
      // Cannot split into higher scope
      DASH_THROW(
        dash::exception::InvalidArgument,
        "Cannot split TeamLocality at scope " << _scope << " "
        "into a parent scope (got: " << scope << ")");
    }
    _scope = scope;
    _domains.clear();
    _unit_ids.clear();

    int     num_domains;
    char ** domain_tags;
    dart_scope_domains(
      _team.dart_id(),
      _domain_tag.c_str(),
      static_cast<dart_locality_scope_t>(_scope),
      &num_domains,
      &domain_tags);
    for (int d = 0; d < num_domains; ++d) {
      _domains.push_back(LocalityDomain(_team, domain_tags[d]));
    }
    for (auto & domain : _domains) {
      _unit_ids.insert(_unit_ids.end(),
                       domain.units().begin(),
                       domain.units().end());
    }

    return *this;
  }

  inline const std::vector<LocalityDomain> & domains() const
  {
    return _domains;
  }

  inline dash::Team & team()
  {
    return _team;
  }

  inline const std::vector<dart_unit_t> & units() const
  {
    return _unit_ids;
  }

private:
  dash::Team                        & _team;
  LocalityScope_t                     _scope      = LocalityScope_t::Global;
  int                                 _level      = 0;
  std::string                         _domain_tag;
  std::vector<LocalityDomain>         _domains;
  std::vector<dart_unit_t>            _unit_ids;

}; // class TeamLocality

}  // namespace util
}  // namespace dash

#endif // DASH__UTIL__TEAM_LOCALITY_H__INCLUDED
