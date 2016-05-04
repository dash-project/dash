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
  const char              * domain_tag,
  dart_domain_locality_t ** locality)
{
  DART_LOG_DEBUG("dart_domain_locality() domain(%s) -> %p",
                 domain_tag, *locality);
  dart_ret_t ret;

  dart_domain_locality_t * dloc;
  ret = dart__base__locality__domain(domain_tag, &dloc);
  if (ret != DART_OK) {
    DART_LOG_ERROR("dart_domain_locality: dart__base__locality__domain "
                   "failed (%d)", ret);
    return ret;
  }
  *locality = dloc;

  DART_LOG_DEBUG("dart_domain_locality > domain(%s) -> %p",
                 domain_tag, *locality);
  return DART_OK;
}

dart_ret_t dart_set_domain_locality(
  dart_domain_locality_t  * loc)
{
  dart__unused(loc);
  return DART_OK;
}

/* ======================================================================== *
 * Team Locality                                                            *
 * ======================================================================== */

dart_ret_t dart_team_locality(
  dart_team_t               team,
  dart_domain_locality_t ** locality)
{
  /*
   * TODO: Should be cached, locality information for a global team id is
   *       invariant.
   */
  DART_LOG_DEBUG("dart_team_locality() team(%d)", team);

  dart_domain_locality_t * loc =
    (dart_domain_locality_t *)(malloc(sizeof(dart_domain_locality_t)));

  /* TODO: resolve domain of given team */
  char domain_tag[DART_LOCALITY_DOMAIN_TAG_MAX_SIZE];
  sprintf(domain_tag, "t.%d", (int)team);

  strncpy(loc->domain_tag, domain_tag, DART_LOCALITY_DOMAIN_TAG_MAX_SIZE);

  /* ensure null-termination of string fields: */
  loc->domain_tag[DART_LOCALITY_DOMAIN_TAG_MAX_SIZE-1] = '\0';

  *locality = loc;

  DART_LOG_DEBUG("dart_team_locality > team(%d) -> %p", team, *locality);
  return DART_OK;
}

dart_ret_t dart_set_team_locality(
  dart_domain_locality_t  * loc)
{
  dart__unused(loc);
  return DART_OK;
}

/* ======================================================================== *
 * Unit Locality                                                            *
 * ======================================================================== */

dart_ret_t dart_unit_locality(
  dart_unit_t             unit,
  dart_unit_locality_t ** locality)
{
  DART_LOG_DEBUG("dart_unit_locality() unit(%d)", unit);
  *locality = NULL;

  dart_unit_locality_t * uloc;
  dart_ret_t ret = dart__base__locality__unit(unit, &uloc);
  if (ret != DART_OK) {
    DART_LOG_ERROR("dart_unit_locality: "
                   "dart__base__unit_locality__get(unit:%d) failed (%d)",
                   unit, ret);
    return ret;
  }
  *locality = uloc;

  DART_LOG_DEBUG("dart_unit_locality > unit(%d) -> %p", unit, *locality);
  return DART_OK;
}

dart_ret_t dart_set_unit_locality(
  dart_unit_locality_t * loc)
{
  dart__unused(loc);
  return DART_OK;
}

