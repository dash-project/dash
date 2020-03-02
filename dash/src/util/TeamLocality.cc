
#include <dash/util/TeamLocality.h>
#include <dash/util/Locality.h>

#include <dash/Exception.h>

#include <dash/internal/Logging.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_locality.h>

#include <vector>
#include <string>


dash::util::TeamLocality::TeamLocality(
  const dash::Team            & team,
  dash::util::Locality::Scope   scope,
  std::string                   domain_tag)
: _team(&team),
  _scope(scope)
{
  DASH_LOG_DEBUG("TeamLocality(t,s,dt)()",
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
    DASH_LOG_DEBUG("TeamLocality(t,s,dt)", "split team domain");
    split(_scope);
  }

  DASH_LOG_DEBUG("TeamLocality(t,s,dt) >");
}

dash::util::TeamLocality::TeamLocality(
  const dash::Team            & team,
  dash::util::LocalityDomain  & domain)
: _team(&team),
  _scope(domain.scope()),
  _domain(domain)
{
  DASH_LOG_DEBUG("TeamLocality(t,d)()",
                 "team:",   team.dart_id(),
                 "domain:", domain.domain_tag());

  DASH_LOG_DEBUG("TeamLocality(t,d) >");
}

