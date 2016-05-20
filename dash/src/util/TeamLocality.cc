
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
  dart_domain_locality_t * domain;
  DASH_ASSERT_RETURNS(
    dart_domain_team_locality(
      _team->dart_id(),
      domain_tag.c_str(),
      &domain),
    DART_OK);

  _split_domains.push_back(dash::util::LocalityDomain(domain));

  if (_scope != Scope_t::Global) {
    split(_scope);
  }
}

dash::util::TeamLocality &
dash::util::TeamLocality::split(
  dash::util::Locality::Scope   scope,
  int                           num_parts)
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
  _split_domains.clear();
  _unit_ids.clear();

  int     num_split_domains;
  char ** domain_tags;
  DASH_ASSERT_RETURNS(
    dart_scope_domains(
      &(_domain.dart_type()),
      static_cast<dart_locality_scope_t>(_scope),
      &num_split_domains,
      &domain_tags),
    DART_OK);
  free(domain_tags);

  if (num_parts < 1 || num_split_domains <= num_parts) {
    DASH_LOG_DEBUG("TeamLocality.split",
                   "split into single subdomains");

    dart_domain_locality_t * subdomains =
      new dart_domain_locality_t[num_split_domains];

    DASH_ASSERT_RETURNS(
      dart_domain_split(
        &(_domain.dart_type()),
        static_cast<dart_locality_scope_t>(_scope),
        num_split_domains,
        subdomains),
      DART_OK);
    for (int sd = 0; sd < num_split_domains; ++sd) {
      DASH_LOG_TRACE_VAR("TeamLocality.split", subdomains[sd].domain_tag);
      _split_domains.push_back(
          dash::util::LocalityDomain(&subdomains[sd]));
    }
  } else {
    DASH_LOG_DEBUG("TeamLocality.split",
                   "split into groups of subdomains");

    dart_domain_locality_t * split_split_domains =
      new dart_domain_locality_t[num_parts];

    DASH_ASSERT_RETURNS(
      dart_domain_split(
        &(_domain.dart_type()),
        static_cast<dart_locality_scope_t>(_scope),
        num_parts,
        split_split_domains),
      DART_OK);

    for (int sd = 0; sd < num_parts; ++sd) {
      DASH_LOG_TRACE_VAR("TeamLocality.split",
                         split_split_domains[sd].domain_tag);
      _split_domains.push_back(
          dash::util::LocalityDomain(&split_split_domains[sd]));
    }
  }

  for (auto & domain : _split_domains) {
    _unit_ids.insert(_unit_ids.end(),
                     domain.units().begin(),
                     domain.units().end());
  }

  DASH_LOG_DEBUG("TeamLocality.split >");

  return *this;
}

