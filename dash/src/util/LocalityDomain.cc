
#include <dash/util/LocalityDomain.h>
#include <dash/util/Locality.h>

#include <dash/Exception.h>

#include <dash/internal/Logging.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_locality.h>

#include <vector>
#include <string>
#include <unordered_map>
#include <utility>
#include <iterator>
#include <algorithm>
#include <functional>


namespace dash {
namespace util {

std::ostream & operator<<(
  std::ostream                     & os,
  const dash::util::LocalityDomain & domain_loc)
{
  return operator<<(os, domain_loc.dart_type());
}

} // namespace util
} // namespace dash

// -------------------------------------------------------------------------
// Public Constructors
// -------------------------------------------------------------------------


dash::util::LocalityDomain::LocalityDomain()
: _domain(nullptr),
  _subdomains(nullptr)
{ }

dash::util::LocalityDomain::LocalityDomain(
  const dart_domain_locality_t     & domain
) : _is_owner(true)
{
  DASH_LOG_TRACE("LocalityDomain(const & d)()",
                 "domain:", domain.domain_tag);

  // Create deep copy of the domain object:
  DASH_ASSERT_RETURNS(
    dart_domain_clone(
      &domain,
      &_domain),
    DART_OK);

  init(_domain);

  DASH_LOG_TRACE("LocalityDomain(const & d) >");
}

dash::util::LocalityDomain::LocalityDomain(
  dart_domain_locality_t           * domain
) : _is_owner(false)
{
  DASH_ASSERT_MSG(
    domain != nullptr,
    "Failed to load locality domain: domain pointer is null");

  DASH_LOG_TRACE("LocalityDomain(d)()",
                 "domain:", domain->domain_tag);
  init(domain);

  DASH_LOG_TRACE("LocalityDomain(d) >", "domain:", _domain->domain_tag);
}

// -------------------------------------------------------------------------
// Destructor
// -------------------------------------------------------------------------

dash::util::LocalityDomain::~LocalityDomain()
{
  DASH_LOG_TRACE_VAR("LocalityDomain.~()", _domain->domain_tag);

  if (_subdomains != nullptr) {
    delete _subdomains;
    _subdomains = nullptr;
  }
  if (_is_owner && _domain != nullptr) {
    dart_domain_destruct(_domain);
  }
  _domain = nullptr;

  DASH_LOG_TRACE("LocalityDomain.~ >");
}

// -------------------------------------------------------------------------
// Copy / Assignment
// -------------------------------------------------------------------------

dash::util::LocalityDomain::LocalityDomain(
  const dash::util::LocalityDomain & other
) : _domain_tag(other._domain_tag),
    _unit_ids(other._unit_ids),
//  _unit_localities(other._unit_localities),
    _group_domain_tags(other._group_domain_tags)
{
  DASH_LOG_TRACE_VAR("LocalityDomain(other)()", other._domain_tag);

  _subdomains  = new std::unordered_map<int, self_t>(
                       *(other._subdomains));
  _is_owner    = other._is_owner;

  if (_is_owner) {
    DASH_ASSERT_RETURNS(
      dart_domain_clone(
        other._domain,
        &_domain),
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

  collect_groups(_group_domain_tags);

  DASH_LOG_TRACE_VAR("LocalityDomain(other) >", other._domain_tag);
}

dash::util::LocalityDomain &
dash::util::LocalityDomain::operator=(
  const dash::util::LocalityDomain & other)
{
  DASH_LOG_TRACE("LocalityDomain.=(other)");

  if (nullptr == _subdomains) {
    if (nullptr != other._subdomains) {
      _subdomains = new std::unordered_map<int, self_t>(
                          *(other._subdomains));
    }
  } else {
    if (nullptr == other._subdomains) {
      delete _subdomains;
    } else {
      *_subdomains = *other._subdomains;
    }
  }

  _is_owner          = other._is_owner;
  _unit_ids          = other._unit_ids;
  //_unit_localities   = other._unit_localities;
  _domain_tag        = other._domain_tag;
  _group_domain_tags = other._group_domain_tags;

  if (_is_owner) {
    DASH_ASSERT_RETURNS(
      dart_domain_clone(
        other._domain,
        &_domain),
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

  collect_groups(_group_domain_tags);

  DASH_LOG_TRACE("LocalityDomain.= >");
  return *this;
}

// -------------------------------------------------------------------------
// Subdomains Filter Methods
// -------------------------------------------------------------------------

dash::util::LocalityDomain &
dash::util::LocalityDomain::LocalityDomain::select(
  const std::vector<std::string> & subdomain_tags)
{
  DASH_LOG_TRACE("LocalityDomain.select(subdomains[])");

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

  for (auto part : _parts) {
    part.select(subdomain_tags);
  }

  DASH_LOG_TRACE("LocalityDomain.select >");
  return *this;
}

dash::util::LocalityDomain &
dash::util::LocalityDomain::exclude(
  const std::vector<std::string> & subdomain_tags)
{
  DASH_LOG_TRACE("LocalityDomain.exclude(subdomains[])");

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

  for (auto part : _parts) {
    part.exclude(subdomain_tags);
  }

  DASH_LOG_TRACE("LocalityDomain.exclude >");
  return *this;
}

dash::util::LocalityDomain &
dash::util::LocalityDomain::group(
  const std::vector<std::string> & group_subdomain_tags)
{
  DASH_LOG_TRACE("LocalityDomain.group(subdomains[])");

  std::vector<const char *> subdomain_tags_cstr;
  for (auto domain_tag : group_subdomain_tags) {
    subdomain_tags_cstr.push_back(domain_tag.c_str());
  }

  char group_domain_tag[DART_LOCALITY_DOMAIN_TAG_MAX_SIZE];

  DASH_ASSERT_RETURNS(
    dart_domain_group(
      _domain,                    // dart_t       * domain
      subdomain_tags_cstr.size(), // int            num_group_subdomains
      subdomain_tags_cstr.data(), // const char  ** group_subdomain_tags
      group_domain_tag),          // char        ** group_domain_tag_out
    DART_OK);

  // Clear cached subdomain instances:
  _subdomains->clear();
  // Clear cached group references:
  _groups.clear();

  _group_domain_tags.push_back(group_domain_tag);
  collect_groups(_group_domain_tags);

  for (auto part : _parts) {
    part.group(group_subdomain_tags);
  }

  DASH_LOG_TRACE("LocalityDomain.group >",
                 "group domain:", _groups.back()->domain_tag());
  return *(_groups.back());
}

dash::util::LocalityDomain &
dash::util::LocalityDomain::split(
  dash::util::Locality::Scope scope,
  int                         num_split_parts)
{
  DASH_LOG_DEBUG("LocalityDomain.split()",
                 "domain:", _domain->domain_tag,
                 "scope:",  scope,
                 "parts:",  num_split_parts);

  // Actual number of subdomains created in the split:
  int                       num_parts = num_split_parts;
  // Number of domains at specified scope:
  int                       num_scope_parts;
  // Tags of domains at specified scope:
  dart_domain_locality_t ** scope_domains;
  DASH_ASSERT_RETURNS(
    dart_domain_scope_domains(
      _domain,
      static_cast<dart_locality_scope_t>(scope),
      &num_scope_parts,
      &scope_domains),
    DART_OK);
  for (int sd = 0; sd < num_scope_parts; ++sd) {
    DASH_LOG_DEBUG("LocalityDomain.split", "scope domain:",
                   scope_domains[sd]->domain_tag);
  }
  free(scope_domains);

  if (num_split_parts < 1 || num_scope_parts <= num_split_parts) {
    DASH_LOG_DEBUG("LocalityDomain.split",
                   "split into single subdomains");
    num_parts = num_scope_parts;
  } else {
    DASH_LOG_DEBUG("LocalityDomain.split",
                   "split into groups of subdomains");
    num_parts = num_split_parts;
  }

  std::vector<dart_domain_locality_t> subdomains;
  subdomains.resize(num_parts);

  DASH_ASSERT_RETURNS(
    dart_domain_split(
      _domain,
      static_cast<dart_locality_scope_t>(scope),
      num_parts,
      subdomains.data()),
    DART_OK);

  _parts.clear();
  for (int sd = 0; sd < num_parts; ++sd) {
    DASH_LOG_TRACE_VAR("LocalityDomain.split",
                       subdomains[sd].domain_tag);
    _parts.push_back(
        dash::util::LocalityDomain(
          subdomains[sd]
        ));
  }

  DASH_LOG_DEBUG("LocalityDomain.split >");

  return *this;
}

dash::util::LocalityDomain &
dash::util::LocalityDomain::split_groups()
{
  DASH_LOG_DEBUG_VAR("LocalityDomain.split_groups()", _group_domain_tags);
  _parts.clear();
  for (auto group_domain_tag : _group_domain_tags) {
    DASH_LOG_TRACE_VAR("LocalityDomain.split_groups",
                       group_domain_tag);
    // Copy the base domain:
    dart_domain_locality_t * group;
    DASH_ASSERT_RETURNS(
      dart_domain_clone(
        _domain,
        &group),
      DART_OK);
    // Remove all subdomains from copied base domain except for the group:
    const char * group_domain_tag_cstr = group_domain_tag.c_str();
    DASH_ASSERT_RETURNS(
      dart_domain_select(
        group, 1, &group_domain_tag_cstr),
      DART_OK);

    _parts.push_back(
        dash::util::LocalityDomain(
          *group
        ));

    dart_domain_destruct(group);
  }
  DASH_LOG_DEBUG("LocalityDomain.split_groups >");

  return *this;
}

// -------------------------------------------------------------------------
// Lookup
// -------------------------------------------------------------------------

const dash::util::LocalityDomain &
dash::util::LocalityDomain::at(
  int relative_index) const
{
  DASH_LOG_DEBUG_VAR("LocalityDomain.at()", relative_index);

  DASH_ASSERT(_subdomains != nullptr);
  DASH_ASSERT(_domain     != nullptr);
  DASH_ASSERT_RANGE(
    0, relative_index, _domain->num_domains,
    "Relative index out of bounds");

  auto subdomain_it = _subdomains->find(relative_index);

  if (subdomain_it == _subdomains->end()) {
    // LocalityDomain instance for subdomain not cached yet:

    DASH_LOG_DEBUG("LocalityDomain.at", " --> creating subdomain instance");
    auto insertion = _subdomains->insert(
                       std::make_pair(
                         relative_index,
                         dash::util::LocalityDomain(
                           *this,
                           &(_domain->domains[relative_index]))
                       ));
    DASH_LOG_DEBUG("LocalityDomain.at", " <-- created subdomain instance");
    DASH_LOG_DEBUG("LocalityDomain.at >",
                   "cached domains[", relative_index, "]");
    return (*(insertion.first)).second;
  }
  DASH_LOG_DEBUG("LocalityDomain.at >");
  return (*subdomain_it).second;
}

dash::util::LocalityDomain &
dash::util::LocalityDomain::at(
  int relative_index)
{
  return const_cast<dash::util::LocalityDomain &>(
           static_cast<const self_t *>(this)->at(relative_index));
}

dash::util::LocalityDomain::const_iterator
dash::util::LocalityDomain::find(
  const std::string & find_tag) const
{
  DASH_LOG_DEBUG("LocalityDomain.find()",
                 "find tag",  find_tag,
                 "in domain", _domain->domain_tag);

  for (auto subdomain_it = begin(); subdomain_it != end(); ++subdomain_it) {
    const std::string & subdomain_tag = subdomain_it->domain_tag();

    // Domain found:
    if (subdomain_tag == find_tag) {
      DASH_LOG_DEBUG_VAR("LocalityDomain.find >", subdomain_tag);
      return subdomain_it;
    }

    // Recurse into subdomains with domain tag that is a prefix of the
    // specified tag:
    auto match_range  = std::mismatch(subdomain_tag.begin(),
                                      subdomain_tag.end(),
                                      find_tag.begin());
    bool prefix_match = match_range.first == subdomain_tag.end();
    if (prefix_match) {
      auto subdomain_res = subdomain_it->find(find_tag);
      if (subdomain_res != subdomain_it->end()) {
        return subdomain_res;
      }
    }
  }
  DASH_LOG_DEBUG("LocalityDomain.find >",
                 "subdomain", find_tag, "not found",
                 "in domain", _domain->domain_tag);
  return end();
}

dash::util::LocalityDomain::iterator
dash::util::LocalityDomain::find(
  const std::string & find_tag)
{
  return iterator(
           static_cast<const self_t *>(this)->find(find_tag));

}

// -------------------------------------------------------------------------
// Private Methods
// -------------------------------------------------------------------------

dash::util::LocalityDomain::LocalityDomain(
  const dash::util::LocalityDomain & parent,
  dart_domain_locality_t           * domain
) : _domain(domain),
    _is_owner(false)
{
  DASH_LOG_TRACE("LocalityDomain(parent,sd)()",
                 "parent domain:", parent.domain_tag(),
                 "subdomain:",     domain->domain_tag);
  init(_domain);

  DASH_LOG_TRACE("LocalityDomain(parent,sd) >");
}

void
dash::util::LocalityDomain::init(
  dart_domain_locality_t * domain)
{
  DASH_ASSERT(domain != nullptr);

  DASH_LOG_DEBUG("LocalityDomain.init()",
                 "domain:", domain->domain_tag);

  _domain     = domain;
  _domain_tag = _domain->domain_tag;

  if (nullptr == _subdomains) {
    _subdomains = new std::unordered_map<int, self_t>();
  } else {
    _subdomains->clear();
  }

  _unit_ids.clear();
#if 0
  _unit_localities.clear();
#endif

  DASH_LOG_TRACE("LocalityDomain.init",
                 "num_units:", _domain->num_units);
  if (_domain->num_units > 0) {
    // The underlying domain might be modified, but we
    // have to store the unit IDs in the original domain
    // configuration. So we save the initial unit IDs here.
    _unit_ids.insert(_unit_ids.end(),
                     _domain->unit_ids,
                     _domain->unit_ids + _domain->num_units);
  }

#if 0
  if (nullptr == _parent) {
    DASH_LOG_TRACE("LocalityDomain.init",
                   "root domain, get unit locality descriptors");
    for (auto unit : _unit_ids) {
      _unit_localities.insert(
        std::make_pair(
          unit,
          UnitLocality_t(
            dash::Team::Get(_domain->team),
            unit)
        )
      );
    }
  }
#endif

  _begin = iterator(*this, 0);
  _end   = iterator(*this, _domain->num_domains);

  collect_groups(_group_domain_tags);

  DASH_LOG_DEBUG("LocalityDomain.init >",
                 "domain:", domain->domain_tag);
}
