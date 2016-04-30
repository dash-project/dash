/**
 * \file dart_locality.c
 *
 * Implementation of all the related global pointer operations
 *
 * All the following functions are implemented with the underlying *MPI-3*
 * one-sided runtime system.
 */

/*
 * Include config and utmpx.h first to prevent previous include of utmpx.h
 * without _GNU_SOURCE in included headers:
 */
#include <dash/dart/base/config.h>
#include <dash/dart/base/macro.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/base/locality.h>
#include <dash/dart/base/domain_map.h>

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
  DART_LOG_DEBUG("dart_domain_locality() domain: %s", domain_tag);

  dart_domain_locality_t * loc;
  dart_ret_t ret = dart__base__domain_map__find(domain_tag, &loc);
  if (ret != DART_OK) {
    return ret;
  }
  *locality = loc;

  DART_LOG_DEBUG("dart_domain_locality > domain: %s -> %p",
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
  DART_LOG_DEBUG("dart_team_locality() team: %d", team);

  dart_domain_locality_t * loc =
    (dart_domain_locality_t *)(malloc(sizeof(dart_domain_locality_t)));

  /* TODO: resolve domain of given team */
  char domain_tag[DART_LOCALITY_DOMAIN_TAG_MAX_SIZE];
  sprintf(domain_tag, "%d", (int)team);

  strncpy(loc->domain_tag, domain_tag, DART_LOCALITY_DOMAIN_TAG_MAX_SIZE);

  /* ensure null-termination of string fields: */
  loc->domain_tag[DART_LOCALITY_DOMAIN_TAG_MAX_SIZE-1] = '\0';

  *locality = loc;

  DART_LOG_DEBUG("dart_team_locality > team: %d -> %p", team, *locality);
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
  dart__unused(unit);
  /*
   * TODO: Should be cached, locality information for a global unit id is
   *       invariant.
   */
  DART_LOG_DEBUG("dart_unit_locality() unit:%d", unit);
  *locality = NULL;

  /* TODO: Temporary implementation, using locality information for local
   *       unit, should be lookup of locality for specified unit.
   */
  dart_unit_locality_t * loc;
  dart_ret_t ret = dart__base__locality__local_unit(&loc);
  if (ret != DART_OK) {
    return ret;
  }
  *locality = loc;

  DART_LOG_DEBUG("dart_unit_locality > unit: %d -> %p", unit, *locality);
  return DART_OK;
}

dart_ret_t dart_set_unit_locality(
  dart_unit_locality_t * loc)
{
  dart__unused(loc);
  return DART_OK;
}
