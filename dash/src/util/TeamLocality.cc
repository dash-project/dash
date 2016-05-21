
#include <dash/util/TeamLocality.h>
#include <dash/util/Locality.h>

#include <dash/Exception.h>

#include <dash/internal/Logging.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_locality.h>

#include <vector>
#include <string>


dash::util::TeamLocality::TeamLocality(
  dash::Team                  & team,
  dash::util::Locality::Scope   scope,
  std::string                   domain_tag)
: _team(&team),
  _scope(scope)
{
  DASH_LOG_DEBUG("TeamLocality(t,s,dt)",
                 "team:",       team.dart_id(),
                 "scope:",      scope,
                 "domain tag:", domain_tag);

  dart_domain_locality_t * domain;
  DASH_ASSERT_RETURNS(
    dart_domain_team_locality(
      _team->dart_id(),
      domain_tag.c_str(),
      &domain),
    DART_OK);

  _domain = dash::util::LocalityDomain(domain);

  if (_scope != Scope_t::Global) {
    split(_scope);
  }

  DASH_LOG_DEBUG("TeamLocality >");
}

dash::util::TeamLocality::TeamLocality(
  dash::Team                  & team,
  dash::util::LocalityDomain  & domain)
: _team(&team),
  _scope(domain.scope()),
  _domain(domain)
{
  DASH_LOG_DEBUG("TeamLocality(t,d)",
                 "team:",   team.dart_id(),
                 "domain:", domain.domain_tag());

  DASH_LOG_DEBUG("TeamLocality >");
}

dash::util::TeamLocality &
dash::util::TeamLocality::split(
  dash::util::Locality::Scope   scope,
  int                           num_split_parts)
{
  DASH_LOG_DEBUG_VAR("TeamLocality.split()", num_split_parts);

  if (static_cast<int>(_scope) > static_cast<int>(scope)) {
    // Cannot split into higher scope
    DASH_THROW(
      dash::exception::InvalidArgument,
      "Cannot split LocalityDomain at scope " << _scope << " "
      "into a parent scope (got: " << scope << ")");
  }
  _scope = scope;

  _parts.clear();

  // Actual number of subdomains created in the split:
  int     num_parts = num_split_parts;
  // Number of domains at specified scope:
  int     num_scope_parts;
  // Tags of domains at specified scope:
  char ** domain_tags;
  DASH_ASSERT_RETURNS(
    dart_domain_scope_tags(
      &(_domain.dart_type()),
      static_cast<dart_locality_scope_t>(_scope),
      &num_scope_parts,
      &domain_tags),
    DART_OK);
  free(domain_tags);

  if (num_split_parts < 1 || num_scope_parts <= num_split_parts) {
    DASH_LOG_DEBUG("TeamLocality.split", "split into single subdomains");
    num_parts = num_scope_parts;
  } else {
    DASH_LOG_DEBUG("TeamLocality.split", "split into groups of subdomains");
    num_parts = num_split_parts;
  }

  dart_domain_locality_t * subdomains =
    new dart_domain_locality_t[num_parts];

  DASH_ASSERT_RETURNS(
    dart_domain_split(
      &(_domain.dart_type()),
      static_cast<dart_locality_scope_t>(_scope),
      num_parts,
      subdomains),
    DART_OK);

  for (int sd = 0; sd < num_parts; ++sd) {
    DASH_LOG_TRACE_VAR("TeamLocality.split",
                       subdomains[sd].domain_tag);

    dash::util::LocalityDomain locality_domain(
      _domain,        // parent
      &subdomains[sd] // subdomain
    );
    _parts.push_back(
      dash::util::TeamLocality(
        *_team, locality_domain)
    );
  }

  DASH_LOG_DEBUG("TeamLocality.split >");

  return *this;
}

