/**
 * \file dart_locality.c
 *
 */
#include <dash/dart/base/config.h>
#include <dash/dart/base/macro.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/base/locality.h>
#include <dash/dart/base/internal/unit_locality.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_locality.h>

#include <mpi.h>

#include <unistd.h>
#include <stdio.h>
#include <sched.h>

/* ======================================================================== *
 * Domain Locality                                                          *
 * ======================================================================== */

dart_ret_t dart_domain_locality(
  dart_team_t               team,
  const char              * domain_tag,
  dart_domain_locality_t ** locality)
{
  DART_LOG_DEBUG("dart_domain_locality() team(%d) domain(%s) -> %p",
                 team, domain_tag, *locality);
  dart_ret_t ret;

  dart_domain_locality_t * dloc;
  ret = dart__base__locality__domain(team, domain_tag, &dloc);
  if (ret != DART_OK) {
    DART_LOG_ERROR("dart_domain_locality: dart__base__locality__domain "
                   "failed (%d)", ret);
    return ret;
  }
  *locality = dloc;

  DART_LOG_DEBUG("dart_domain_locality > team(%d) domain(%s) -> %p",
                 team, domain_tag, *locality);
  return DART_OK;
}

dart_ret_t dart_domain_split(
  dart_team_t               team,
  const char              * domain_tag,
  dart_locality_scope_t     scope,
  int                       num_parts,
  int                    ** group_sizes_out,
  char                 **** group_domain_tags_out)
{
  DART_LOG_DEBUG("dart_domain_split() team(%d) domain(%s) "
                 "into %d parts at scope %d",
                 team, domain_tag, num_parts, scope);
  return dart__base__locality__domain_split(
           team, domain_tag, scope, num_parts,
           group_sizes_out, group_domain_tags_out);
  DART_LOG_DEBUG("dart_domain_split >");
  return DART_OK;
}

dart_ret_t dart_scope_domains(
  dart_team_t               team,
  const char              * domain_tag,
  dart_locality_scope_t     scope,
  int                     * num_domains_out,
  char                  *** domain_tags_out)
{
  *num_domains_out = 0;
  *domain_tags_out = NULL;

  return dart__base__locality__scope_domains(
           team,
           domain_tag,
           scope,
           num_domains_out,
           domain_tags_out);
}

/* ======================================================================== *
 * Unit Locality                                                            *
 * ======================================================================== */

dart_ret_t dart_unit_locality(
  dart_team_t             team,
  dart_unit_t             unit,
  dart_unit_locality_t ** locality)
{
  DART_LOG_DEBUG("dart_unit_locality() team(%d) unit(%d)", team, unit);
  *locality = NULL;

  dart_unit_locality_t * uloc;
  dart_ret_t ret = dart__base__locality__unit(team, unit, &uloc);
  if (ret != DART_OK) {
    DART_LOG_ERROR("dart_unit_locality: "
                   "dart__base__unit_locality__get(unit:%d) failed (%d)",
                   unit, ret);
    return ret;
  }
  *locality = uloc;

  DART_LOG_DEBUG("dart_unit_locality > team(%d) unit(%d) -> %p",
                 team, unit, *locality);
  return DART_OK;
}

