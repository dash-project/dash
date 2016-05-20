#ifndef DASH__UTIL__LOCALITY_DOMAIN_H__INCLUDED
#define DASH__UTIL__LOCALITY_DOMAIN_H__INCLUDED

#include <dash/util/Locality.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_locality.h>

#include <dash/algorithm/internal/String.h>

#include <dash/Exception.h>

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
  : _domain(nullptr),
    _subdomains(nullptr)
  {
    clear();
  }

  LocalityDomain(
    const self_t & parent,
    std::string    domain_tag)
  : _domain(nullptr),
    _is_owner(false)
  {
    DASH_LOG_TRACE("LocalityDomain(domain_tag)",
                   "domain tag:", domain_tag);

    DASH_ASSERT_RETURNS(
      dart_domain_locality(
        &(parent.dart_type()),
        domain_tag.c_str(),
        &_domain),
      DART_OK);

    DASH_ASSERT_MSG(
      _domain != NULL &&
      _domain != nullptr,
      "Failed to load locality domain with tag " << domain_tag);

    _subdomains = new std::unordered_map<int, self_t>();

    init(_domain);

    DASH_LOG_TRACE("LocalityDomain(t,dom) >");
  }

  LocalityDomain(
    dart_domain_locality_t * domain)
  : _domain(domain),
    _is_owner(false)
  {
    DASH_LOG_TRACE("LocalityDomain(const dom)",
                   "domain:", domain->domain_tag);
    DASH_ASSERT_MSG(
      _domain != NULL &&
      _domain != nullptr,
      "Failed to load locality domain with tag " << domain->domain_tag);

    _subdomains = new std::unordered_map<int, self_t>();

    init(_domain);

    DASH_LOG_TRACE("LocalityDomain(t,dom) >");
  }

  ~LocalityDomain()
  {
    if (_subdomains != nullptr) {
      delete _subdomains;
      _subdomains = nullptr;
    }
    if (_is_owner && _domain != nullptr) {
      dart_domain_delete(_domain);
      _domain = nullptr;
    }
  }

  LocalityDomain(const self_t & other)
  : _domain_tag(other._domain_tag),
    _unit_ids(other._unit_ids)
  {
    _subdomains = new std::unordered_map<int, self_t>();

    _is_owner = other._is_owner;
    if (_is_owner) {
      _domain = new dart_domain_locality_t();
      DASH_ASSERT_RETURNS(
        dart_domain_copy(
          _domain,
          other._domain),
        DART_OK);
    } else {
      _domain = other._domain;
    }
    _begin = iterator(*this, 0);
    if (_domain != nullptr) {
      _end = iterator(*this, _domain->num_domains);
    } else {
      _end = iterator(*this, 0);
    }
  }

  self_t & operator=(const self_t & other)
  {
    _unit_ids   = other._unit_ids;
    _is_owner   = other._is_owner;

    _subdomains = new std::unordered_map<int, self_t>();

    if (_is_owner) {
      _domain = new dart_domain_locality_t();
      DASH_ASSERT_RETURNS(
        dart_domain_copy(
          _domain,
          other._domain),
        DART_OK);
    } else {
      _domain = other._domain;
    }
    _begin = iterator(*this, 0);
    if (_domain != nullptr) {
      _end = iterator(*this, _domain->num_domains);
    } else {
      _end = iterator(*this, 0);
    }

    return *this;
  }

  inline bool operator==(const self_t & rhs) const
  {
    return ( (_domain == rhs._domain)
             ||
             ( (_domain     != nullptr &&
                rhs._domain != nullptr)
                &&
               (_domain->team
                  == rhs._domain->team &&
                _domain->domain_tag
                  == rhs._domain->domain_tag) ) );
  }

  inline bool operator!=(const self_t & rhs) const
  {
    return !(*this == rhs);
  }

  self_t & select(
    std::initializer_list<std::string> subdomain_tags)
  {
    std::vector<const char *> subdomain_tags_cstr;
    for (auto domain_tag : subdomain_tags) {
      subdomain_tags_cstr.push_back(domain_tag.c_str());
    }

    DASH_ASSERT_RETURNS(
      dart_domain_select(
        _domain,
        subdomain_tags_cstr.size(),
        subdomain_tags_cstr.data()),
      DART_OK);

    init(_domain);
    return *this;
  }

  self_t & exclude(
    std::initializer_list<std::string> subdomain_tags)
  {
    std::vector<const char *> subdomain_tags_cstr;
    for (auto domain_tag : subdomain_tags) {
      subdomain_tags_cstr.push_back(domain_tag.c_str());
    }

    DASH_ASSERT_RETURNS(
      dart_domain_exclude(
        _domain,
        subdomain_tags_cstr.size(),
        subdomain_tags_cstr.data()),
      DART_OK);

    init(_domain);
    return *this;
  }

  self_t & group(
    std::initializer_list<std::string> group_subdomain_tags)
  {
    std::vector<const char *> subdomain_tags_cstr;
    for (auto domain_tag : group_subdomain_tags) {
      subdomain_tags_cstr.push_back(domain_tag.c_str());
    }

    char group_domain_tag[DART_LOCALITY_DOMAIN_TAG_MAX_SIZE];

    DASH_ASSERT_RETURNS(
      dart_group_domains(
        _domain,                    // dart_t       * domain
        subdomain_tags_cstr.size(), // int            num_group_subdomains
        subdomain_tags_cstr.data(), // const char  ** group_subdomain_tags
        group_domain_tag),          // char        ** group_domain_tag_out
      DART_OK);

    // Clear cached subdomain instances:
    _subdomains->clear();

    dart_domain_locality_t * group_domain;
    DASH_ASSERT_RETURNS(
      dart_domain_locality(
        _domain,
        group_domain_tag,
        &group_domain),
      DART_OK);

    _groups.push_back(self_t(group_domain));

    return _groups.back();
  }

  inline void clear()
  {
    _unit_ids.clear();
    _groups.clear();
    if (nullptr != _subdomains) {
      _subdomains->clear();
    }
    _begin = iterator(*this, 0);
    _end   = iterator(*this, 0);
  }

  /**
   * Lazy-load instance of \c LocalityDomain for subdomain.
   */
  inline self_t & at(int relative_index) const
  {
    DASH_ASSERT(_subdomains != nullptr);
    DASH_ASSERT(_domain  != nullptr);
    if (_subdomains->find(relative_index) == _subdomains->end()) {
      // LocalityDomain instance for subdomain not cached yet:
      self_t subdomain(&(_domain->domains[relative_index]));
      _subdomains->insert(std::make_pair(relative_index, subdomain));
    }
    return (*_subdomains)[relative_index];
  }

  inline dart_team_t dart_team()
  {
    if (nullptr == _domain) {
      return DART_TEAM_NULL;
    }
    return _domain->team;
  }

  inline const std::string & domain_tag() const
  {
    return _domain_tag;
  }

  inline const dart_domain_locality_t & dart_type() const
  {
    DASH_ASSERT(_domain != nullptr);
    return *_domain;
  }

  inline std::string host() const
  {
    DASH_ASSERT(_domain != nullptr);
    return _domain->host;
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
    return _domain == nullptr
           ? 0
           : _domain->num_domains;
  }

  inline const std::vector<dart_unit_t> & units() const
  {
    return _unit_ids;
  }

  inline const dart_hwinfo_t & hwinfo() const
  {
    DASH_ASSERT(_domain != nullptr);
    return _domain->hwinfo;
  }

  inline int level() const
  {
    DASH_ASSERT(_domain != nullptr);
    return _domain->level;
  }

  inline Scope_t scope() const
  {
    return _domain == nullptr
           ? Scope_t::Undefined
           : static_cast<Scope_t>(_domain->scope);
  }

  inline int node_id() const
  {
    DASH_ASSERT(_domain != nullptr);
    return _domain->node_id;
  }

  inline int num_nodes() const
  {
    DASH_ASSERT(_domain != nullptr);
    return _domain->num_nodes;
  }

  inline int relative_index() const
  {
    DASH_ASSERT(_domain != nullptr);
    return _domain->relative_index;
  }

private:

  void init(dart_domain_locality_t * domain)
  {
    clear();

    _domain = domain;

    if (_domain->num_units > 0) {
      _unit_ids = std::vector<dart_unit_t>(
                    _domain->unit_ids,
                    _domain->unit_ids + _domain->num_units);
    }

    _begin = iterator(*this, 0);
    _end   = iterator(*this, _domain->num_domains);
  }

private:
  /// Underlying \c dart_domain_locality_t object.
  dart_domain_locality_t                   * _domain    = nullptr;
  /// Copy of _domain->domain_tag to avoid string copying.
  std::string                                _domain_tag;
  /// Cache of lazy-loaded subdomains, mapped by subdomain relative index.
  /// Must be heap-allocated as type is incomplete due to type definition
  /// cycle.
  mutable std::unordered_map<int, self_t>  * _subdomains = nullptr;
  /// Units in the domain.
  std::vector<dart_unit_t>                   _unit_ids;
  /// Iterator to the first subdomain.
  iterator                                   _begin;
  /// Iterator past the last subdomain.
  iterator                                   _end;
  /// Whether this instance is owner of _domain.
  bool                                       _is_owner   = false;
  /// Domain tags of groups in the locality domain.
  std::vector<self_t>                        _groups;

}; // class LocalityDomain

} // namespace util
} // namespace dash

#endif // DASH__UTIL__LOCALITY_DOMAIN_H__INCLUDED
