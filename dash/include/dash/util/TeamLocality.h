#ifndef DASH__UTIL__TEAM_LOCALITY_H__INCLUDED
#define DASH__UTIL__TEAM_LOCALITY_H__INCLUDED

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_locality.h>

#include <dash/util/Locality.h>

#include <dash/algorithm/internal/String.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <utility>
#include <iterator>
#include <algorithm>


namespace dash {
namespace util {

/**
 * Wrapper of a single \c dart_domain_locality_t object.
 */
class LocalityDomain
{
private:
  typedef LocalityDomain                       self_t;
  typedef dash::util::Locality::Scope         Scope_t;

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
      DASH_ASSERT(_domain != nullptr);
      int subdomain_idx = _idx + i;
      return _domain->at(subdomain_idx);
    }

    const_reference operator*()
    {
      DASH_ASSERT(_domain != nullptr);
      return _domain->at(_idx);
    }

    const_pointer operator->()
    {
      DASH_ASSERT(_domain != nullptr);
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
  : _team(&dash::Team::Null()),
    _domain_locality(nullptr),
    _domains(nullptr)
  {
    _begin = iterator(*this, 0);
    _end   = iterator(*this, 0);
  }

  LocalityDomain(
    dash::Team  & team,
    std::string   domain_tag)
  : _team(&team)
  {
    DASH_LOG_TRACE("LocalityDomain(t,dom)",
                   "team:", team.dart_id(), "domain:", domain_tag);
    DASH_ASSERT_RETURNS(
      dart_domain_locality(
        _team->dart_id(),
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

    DASH_LOG_TRACE("LocalityDomain(t,dom) >");
  }

  LocalityDomain(
    dash::Team          & team,
    std::string           domain_tag,
    std::vector<self_t> & domains)
  : _team(&team),
    _domain_locality(nullptr),
    _is_owner(true)
  {
    DASH_LOG_TRACE("LocalityDomain(t,dom,subdoms)",
                   "team:", team.dart_id(), "domain:", domain_tag,
                   "n.domains:", domains.size());
    _begin = iterator(*this, 0);
    _end   = iterator(*this, domains.size());

    _domains = new std::unordered_map<int, self_t>();

    dart_domain_locality_t * parent_domain;
    DASH_ASSERT_RETURNS(
      dart_domain_locality(
        _team->dart_id(),
        domain_tag.c_str(),
        &parent_domain),
      DART_OK);

    int n_domains = domains.size();

    _domain_locality                 = new dart_domain_locality_t();
    _domain_locality->scope          = parent_domain->scope;
    _domain_locality->level          = 0;
    _domain_locality->relative_index = 0;
    _domain_locality->team           = _team->dart_id();
    _domain_locality->parent         = NULL;
    _domain_locality->num_domains    = n_domains;
    _domain_locality->domains        = new dart_domain_locality_t[n_domains];
    _domain_locality->hwinfo         = parent_domain->hwinfo;
    _domain_locality->node_id        = parent_domain->node_id;
    _domain_locality->num_nodes      = parent_domain->num_nodes;

    int n_units       = 0;
    int subdomain_idx = 0;
    for (auto & domain : domains) {
      n_units += domain.units().size();

      _unit_ids.insert(_unit_ids.end(),
                       domain.units().begin(),
                       domain.units().end());
      _domains->insert(std::make_pair(subdomain_idx, domain));

      _domain_locality->domains[subdomain_idx] =
        domain.dart_type();

      subdomain_idx++;
    }

    _domain_locality->num_units = n_units;
    _domain_locality->unit_ids  = new dart_unit_t[n_units];

    std::copy(_unit_ids.begin(), _unit_ids.end(),
              _domain_locality->unit_ids);
    std::strcpy(_domain_locality->domain_tag, parent_domain->domain_tag);
    std::strcpy(_domain_locality->host,       parent_domain->host);

    DASH_LOG_TRACE("LocalityDomain(t,dom,subdoms) >");
  }

  ~LocalityDomain()
  {
    if (_domains != nullptr) {
      delete _domains;
      _domains = nullptr;
    }
    if (_is_owner && _domain_locality != nullptr) {
      delete _domain_locality->domains;
      delete _domain_locality->unit_ids;
      delete _domain_locality;
      _domain_locality = nullptr;
    }
  }

  LocalityDomain(const self_t & other)
  : _team(other._team),
    _unit_ids(other._unit_ids)
  {
    std::string domain_tag = "";
    if (other._domain_locality != nullptr) {
      domain_tag = other._domain_locality->domain_tag;
    }
    _is_owner = other._is_owner;
    if (_is_owner) {
      _domain_locality  = new dart_domain_locality_t();
      *_domain_locality = *(other._domain_locality);
    } else {
      _domain_locality = other._domain_locality;
    }
    _begin = iterator(*this, 0);
    if (_domain_locality != nullptr && domain_tag != "") {
      _end = iterator(*this, _domain_locality->num_domains);
    } else {
      _end = iterator(*this, 0);
    }
    _domains  = new std::unordered_map<int, self_t>();
    *_domains = *(other._domains);
  }

  self_t & operator=(const self_t & other)
  {
    _team     = other._team;
    _unit_ids = other._unit_ids;
    _is_owner = other._is_owner;

    std::string domain_tag = "";
    if (other._domain_locality != nullptr) {
      domain_tag = other._domain_locality->domain_tag;
    }
    _is_owner = other._is_owner;
    if (_is_owner) {
      _domain_locality  = new dart_domain_locality_t();
      *_domain_locality = *(other._domain_locality);
    } else {
      _domain_locality = other._domain_locality;
    }
    _begin = iterator(*this, 0);
    if (_domain_locality != nullptr && domain_tag != "") {
      _end = iterator(*this, _domain_locality->num_domains);
    } else {
      _end = iterator(*this, 0);
    }
    _domains  = new std::unordered_map<int, self_t>();
    *_domains = *(other._domains);

    return *this;
  }

  inline bool operator==(const self_t & rhs) const
  {
    return ( (_team == rhs._team)
             ||
             ( (_team != nullptr && rhs._team != nullptr)
               &&
               (*_team == *rhs._team) ) )
           &&
           ( (_domain_locality == rhs._domain_locality)
             ||
             ( (_domain_locality     != nullptr &&
                rhs._domain_locality != nullptr)
                &&
               (_domain_locality->team
                  == rhs._domain_locality->team &&
                _domain_locality->domain_tag
                  == rhs._domain_locality->domain_tag) ) );
  }

  inline bool operator!=(const self_t & rhs) const
  {
    return !(*this == rhs);
  }

  inline dash::Team & team()
  {
    DASH_ASSERT(_domain_locality != nullptr);
    return *_team;
  }

  inline std::string domain_tag() const
  {
    DASH_ASSERT(_domain_locality != nullptr);
    return _domain_locality->domain_tag;
  }

  inline const dart_domain_locality_t & dart_type() const
  {
    DASH_ASSERT(_domain_locality != nullptr);
    return *_domain_locality;
  }

  inline std::string host() const
  {
    DASH_ASSERT(_domain_locality != nullptr);
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

  inline size_t size() const
  {
    return _domain_locality == nullptr
           ? 0
           : _domain_locality->num_domains;
  }

  inline const std::vector<dart_unit_t> & units() const
  {
    return _unit_ids;
  }

  inline const dart_hwinfo_t & hwinfo() const
  {
    DASH_ASSERT(_domain_locality != nullptr);
    return _domain_locality->hwinfo;
  }

  inline int level() const
  {
    DASH_ASSERT(_domain_locality != nullptr);
    return _domain_locality->level;
  }

  inline Scope_t scope() const
  {
    return _domain_locality == nullptr
           ? Scope_t::Undefined
           : static_cast<Scope_t>(_domain_locality->scope);
  }

  inline int node_id() const
  {
    DASH_ASSERT(_domain_locality != nullptr);
    return _domain_locality->node_id;
  }

  inline int num_nodes() const
  {
    DASH_ASSERT(_domain_locality != nullptr);
    return _domain_locality->num_nodes;
  }

  inline int relative_index() const
  {
    DASH_ASSERT(_domain_locality != nullptr);
    return _domain_locality->relative_index;
  }

  /**
   * Lazy-load instance of \c LocalityDomain for subdomain.
   */
  inline self_t & at(int relative_index) const
  {
    DASH_ASSERT(_domains         != nullptr);
    DASH_ASSERT(_domain_locality != nullptr);
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
  dash::Team                               * _team = &dash::Team::Null();
  /// Underlying \c dart_domain_locality_t object.
  dart_domain_locality_t                   * _domain_locality = nullptr;
  /// Cache of lazy-loaded subdomains, mapped by subdomain relative index.
  /// Must be heap-allocated as type is incomplete due to type definition
  /// cycle.
  mutable std::unordered_map<int, self_t>  * _domains         = nullptr;
  /// Units in the domain.
  std::vector<dart_unit_t>                   _unit_ids;
  /// Iterator to the first subdomain.
  iterator                                   _begin;
  /// Iterator past the last subdomain.
  iterator                                   _end;
  /// Whether this instance is owner of _domain_locality.
  bool                                       _is_owner        = false;

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
    DASH_ASSERT(_unit_locality != nullptr);
    return _unit_locality->hwinfo;
  }

  inline dash::Team & team()
  {
    DASH_ASSERT(_unit_locality != nullptr);
    return _team;
  }

  inline dart_unit_t unit_id() const
  {
    return _unit_locality == nullptr
           ? DART_UNDEFINED_UNIT_ID
           : _unit_locality->unit;
  }

  inline std::string domain_tag() const
  {
    DASH_ASSERT(_unit_locality != nullptr);
    return _unit_locality->domain_tag;
  }

  inline std::string host() const
  {
    DASH_ASSERT(_unit_locality != nullptr);
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
  typedef dash::util::Locality::Scope     Scope_t;

public:

  TeamLocality(
    dash::Team & team,
    Scope_t      scope      = Scope_t::Global,
    std::string  domain_tag = ".")
  : _team(team),
    _scope(scope),
    _domain_tag(domain_tag)
  {
    if (_domain_tag != ".") {
      select(_domain_tag);
    }
    if (_scope == Scope_t::Global) {
      // Load the team's global locality domain hierarchy:
      _domains.push_back(LocalityDomain(_team, _domain_tag));
      for (auto & domain : _domains) {
        _unit_ids.insert(_unit_ids.end(),
                         domain.units().begin(),
                         domain.units().end());
      }
    } else {
      _scope = Scope_t::Undefined;
      split(scope);
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

  self_t & split(Scope_t scope, int num_parts = 0)
  {
    DASH_LOG_DEBUG_VAR("TeamLocality.split()", num_parts);

    if (static_cast<int>(_scope) > static_cast<int>(scope)) {
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
    DASH_ASSERT_RETURNS(
      dart_scope_domains(
        _team.dart_id(),
        _domain_tag.c_str(),
        static_cast<dart_locality_scope_t>(_scope),
        &num_domains,
        &domain_tags),
      DART_OK);

    if (num_parts < 1 || num_domains <= num_parts) {
      DASH_LOG_DEBUG("TeamLocality.split", "split into single subdomains");
      /* Split into single subdomains: */
      for (int d = 0; d < num_domains; ++d) {
        DASH_LOG_TRACE_VAR("TeamLocality.split", domain_tags[d]);
        _domains.push_back(LocalityDomain(_team, domain_tags[d]));
      }
    } else {
      DASH_LOG_DEBUG("TeamLocality.split", "split into groups of subdomains");
      /* Split into single subdomains: */
      /* Split into groups of subdomains: */
      int    * group_sizes;
      char *** group_domain_tags;
      DASH_ASSERT_RETURNS(
        dart_domain_split(
          _team.dart_id(),
          _domain_tag.c_str(),
          static_cast<dart_locality_scope_t>(_scope),
          num_parts,
          &group_sizes,
          &group_domain_tags),
        DART_OK);
      // Find longest common prefix of split domain tags:
      std::string root_domain_tag = dash::internal::common_prefix(
                                      std::vector<std::string>(
                                        domain_tags,
                                        domain_tags + num_domains));
      // Remove trailing '.':
      root_domain_tag.erase(root_domain_tag.find_last_of("."));

      std::vector<std::string> group_tags;
      for (int g = 0; g < num_parts; ++g) {
        std::string group_tag = dash::internal::common_prefix(
                                  std::vector<std::string>(
                                    group_domain_tags[g],
                                    group_domain_tags[g] + group_sizes[g]));
        // Remove trailing '.':
        group_tag.erase(group_tag.find_last_of("."));
        DASH_LOG_TRACE_VAR("TeamLocality.split", group_tag);
        group_tags.push_back(group_tag);
        while (group_tag == root_domain_tag) {
          // Root domain tag collides with group domain tag.
          // Remove trailing domain parts from root domain tag until it is
          // a valid parent tag of the group.
          size_t last_dot_pos = root_domain_tag.find_last_of(".");
          root_domain_tag.erase(last_dot_pos);
        }
      }
      DASH_LOG_TRACE_VAR("TeamLocality.split", root_domain_tag);
      // Group subdomains:
      for (int g = 0; g < num_parts; ++g) {
        int n_group_domains     = group_sizes[g];
        std::string & group_tag = group_tags[g];
        DASH_LOG_TRACE("TeamLocality.split", "-- group[", g, "]:",
                       "group tag:", group_tag,
                       "domains:",   n_group_domains);
        std::vector<LocalityDomain> subdomains;
        for (int d = 0; d < n_group_domains; ++d) {
          std::string & subdomain_tag = group_domain_tags[g][d];
          subdomains.push_back(
            LocalityDomain(_team, subdomain_tag));
          DASH_LOG_TRACE("TeamLocality.split", "   group[", g, "][", d, "]:",
                         "tag:",     subdomains.back().domain_tag(),
                         "units:",   subdomains.back().units(),
                         "domains:", subdomains.back().size());
        }
        _domains.push_back(
          LocalityDomain(_team, group_tag, subdomains));
        DASH_LOG_TRACE("TeamLocality.split", "-> group[", g, "]:",
                       "tag:",     _domains.back().domain_tag(),
                       "units:",   _domains.back().units(),
                       "domains:", _domains.back().size());
      }
    }

    for (auto & domain : _domains) {
      _unit_ids.insert(_unit_ids.end(),
                       domain.units().begin(),
                       domain.units().end());
    }

    DASH_LOG_DEBUG("TeamLocality.split >");
    free(domain_tags);

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
  Scope_t                             _scope       = Scope_t::Undefined;
  int                                 _level       = 0;
  std::string                         _domain_tag;
  std::vector<LocalityDomain>         _domains;
  std::vector<dart_unit_t>            _unit_ids;

}; // class TeamLocality

}  // namespace util
}  // namespace dash

#endif // DASH__UTIL__TEAM_LOCALITY_H__INCLUDED
