
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


dash::util::LocalityDomain::LocalityDomain(
  const dash::util::LocalityDomain & parent,
  dart_domain_locality_t           * domain
) : _domain(domain),
    _is_owner(false)
{
  DASH_LOG_TRACE("LocalityDomain(parent,sd)()",
                 "parent domain:", parent.domain_tag(),
                 "subdomain:",     domain->domain_tag);

  _subdomains = new std::unordered_map<int, self_t>();

  init(_domain);

  DASH_LOG_TRACE("LocalityDomain(parent,sd) >");
}

dash::util::LocalityDomain::LocalityDomain(
  dart_domain_locality_t           * domain
) : _domain(domain),
    _is_owner(false)
{
  DASH_LOG_TRACE("LocalityDomain(d)()",
                 "domain:", domain->domain_tag);

  DASH_ASSERT_MSG(
    _domain != NULL &&
    _domain != nullptr,
    "Failed to load locality domain with tag " << domain->domain_tag);

  _subdomains = new std::unordered_map<int, self_t>();

  init(_domain);

  DASH_LOG_TRACE("LocalityDomain(d) >");
}

dash::util::LocalityDomain::LocalityDomain(
  const dash::util::LocalityDomain & parent,
  std::string                        subdomain_tag
) : _domain(nullptr),
    _is_owner(true)
{
  DASH_LOG_TRACE("LocalityDomain(parent,sdt)()",
                 "parent domain:", parent.domain_tag(),
                 "subdomain:",     subdomain_tag);

  DASH_ASSERT_RETURNS(
    dart_domain_locality(
      &(parent.dart_type()),
      subdomain_tag.c_str(),
      &_domain),
    DART_OK);

  DASH_ASSERT_MSG(
    _domain != NULL &&
    _domain != nullptr,
    "Failed to load locality domain with tag " << subdomain_tag);

  _subdomains = new std::unordered_map<int, self_t>();

  init(_domain);

  DASH_LOG_TRACE("LocalityDomain(parent,sdt) >");
}

dash::util::LocalityDomain::~LocalityDomain()
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

dash::util::LocalityDomain::LocalityDomain(
  const dash::util::LocalityDomain & other
) : _domain_tag(other._domain_tag),
    _unit_ids(other._unit_ids)
{
  DASH_LOG_TRACE("LocalityDomain(other)()");

  _subdomains = new std::unordered_map<int, self_t>();
  _is_owner   = other._is_owner;

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
  DASH_LOG_TRACE("LocalityDomain(other) > ");
}

dash::util::LocalityDomain &
dash::util::LocalityDomain::operator=(
  const dash::util::LocalityDomain & other)
{
  DASH_LOG_TRACE("LocalityDomain.=(other)");

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

  DASH_LOG_TRACE("LocalityDomain.= >");
  return *this;
}

dash::util::LocalityDomain &
dash::util::LocalityDomain::LocalityDomain::select(
  std::initializer_list<std::string> subdomain_tags)
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

  DASH_LOG_TRACE("LocalityDomain.select >");
  return *this;
}

dash::util::LocalityDomain &
dash::util::LocalityDomain::exclude(
  std::initializer_list<std::string> subdomain_tags)
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

  DASH_LOG_TRACE("LocalityDomain.exclude >");
  return *this;
}

dash::util::LocalityDomain &
dash::util::LocalityDomain::group(
  std::initializer_list<std::string> group_subdomain_tags)
{
  DASH_LOG_TRACE("LocalityDomain.group(subdomains[])");

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

  DASH_LOG_TRACE("LocalityDomain.group >",
                 "group domain:", _groups.back().domain_tag());
  return _groups.back();
}

