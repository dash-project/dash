/**
 * \file dash/dart/base/internal/unit_locality.c
 */

/*
 * Include config and utmpx.h first to prevent previous include of utmpx.h
 * without _GNU_SOURCE in included headers:
 */
#include <dash/dart/base/config.h>
#ifdef DART__PLATFORM__LINUX
#  define _GNU_SOURCE
#  include <utmpx.h>
#endif
#include <dash/dart/base/macro.h>
#include <dash/dart/base/locality.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_locality.h>
#include <dash/dart/if/dart_communication.h>

#include <unistd.h>
#include <inttypes.h>
#include <stdio.h>
#include <sched.h>
#include <string.h>

#ifdef DART_ENABLE_LIKWID
#  include <likwid.h>
#endif

#ifdef DART_ENABLE_HWLOC
#  include <hwloc.h>
#  include <hwloc/helper.h>
#endif

#ifdef DART_ENABLE_PAPI
#  include <papi.h>
#endif

#ifdef DART_ENABLE_NUMA
#  include <utmpx.h>
#  include <numa.h>
#endif

#include <dash/dart/base/logging.h>
#include <dash/dart/base/assert.h>
#include <dash/dart/base/hwinfo.h>

#include <dash/dart/base/internal/unit_locality.h>
#include <dash/dart/base/internal/host_topology.h>

/**
 * Functions for exchanging and collecting locality information of units
 * in a team.
 */


/* ======================================================================== *
 * Private Functions                                                        *
 * ======================================================================== */

dart_ret_t dart__base__unit_locality__init(
  dart_unit_locality_t  * loc);

dart_ret_t dart__base__unit_locality__local_unit_new(
  dart_team_t             team,
  dart_unit_locality_t  * loc);

/* ======================================================================== *
 * Init / Finalize                                                          *
 * ======================================================================== */

/**
 * Exchange and collect all locality information of all units in the
 * specified team in an array of \c dart_unit_mapping_t objects.
 *
 * Note that locality information does not contain the units' locality
 * domain tags.
 *
 * \note
 * This is a collective N-to-N (allgather) operation.
 */
dart_ret_t dart__base__unit_locality__create(
  dart_team_t             team,
  dart_unit_mapping_t  ** unit_mapping)
{
  dart_ret_t  ret;
  dart_team_unit_t myid = DART_UNDEFINED_TEAM_UNIT_ID;
  size_t      nunits = 0;
  *unit_mapping      = NULL;
  DART_LOG_DEBUG("dart__base__unit_locality__create()");

  DART_ASSERT_RETURNS(dart_team_myid(team, &myid),   DART_OK);
  DART_ASSERT_RETURNS(dart_team_size(team, &nunits), DART_OK);

  dart_unit_mapping_t * mapping = malloc(sizeof(dart_unit_mapping_t));
  mapping->num_units            = nunits;
  mapping->team                 = team;

  size_t nbytes = sizeof(dart_unit_locality_t);

  /* get local unit's locality information: */
  dart_unit_locality_t * uloc;
  uloc = (dart_unit_locality_t *)(malloc(sizeof(dart_unit_locality_t)));
  ret  = dart__base__unit_locality__local_unit_new(team, uloc);
  if (ret != DART_OK) {
    DART_LOG_ERROR("dart__base__unit_locality__create ! "
                   "dart__base__unit_locality__local_unit_new failed: %d",
                   ret);
    return ret;
  }
  DART_LOG_TRACE("dart__base__unit_locality__create: unit %d of %ld: "
                 "sending %ld bytes: "
                 "host:'%s' core_id:%d numa_id:%d nthreads:%d",
                 myid.id, nunits, nbytes,
                 uloc->hwinfo.host,
                 uloc->hwinfo.cpu_id, uloc->hwinfo.numa_id,
                 uloc->hwinfo.max_threads);

  mapping->unit_localities = (dart_unit_locality_t *)(
                                malloc(nunits * nbytes));
  dart_barrier(team);

  /* all-to-all exchange of locality data across all units:
   * (send, recv, nbytes, team) */
  DART_LOG_DEBUG("dart__base__unit_locality__create: dart_allgather");
  ret = dart_allgather(uloc,
                       mapping->unit_localities,
                       nbytes,
                       DART_TYPE_BYTE,
                       team);

  dart_barrier(team);
  free(uloc);

  if (ret != DART_OK) {
    DART_LOG_ERROR("dart__base__unit_locality__create ! "
                   "dart_allgather failed: %d", ret);
    return ret;
  }
#ifdef DART_ENABLE_LOGGING
  for (size_t u = 0; u < nunits; ++u) {
    dart_unit_locality_t * ulm_u = &mapping->unit_localities[u];
    DART_LOG_TRACE("dart__base__unit_locality__create: unit[%d]: "
                   "unit:%d host:'%s' "
                   "num_cores:%d core_id:%d cpu_id:%d "
                   "num_numa:%d numa_id:%d "
                   "nthreads:%d",
                   (int)(u), ulm_u->unit,
                   ulm_u->hwinfo.host,
                   ulm_u->hwinfo.num_cores, ulm_u->hwinfo.core_id,
                   ulm_u->hwinfo.cpu_id,
                   ulm_u->hwinfo.num_numa, ulm_u->hwinfo.numa_id,
                   ulm_u->hwinfo.max_threads);
  }
#endif

  *unit_mapping = mapping;

  DART_LOG_DEBUG("dart__base__unit_locality__create >");
  return DART_OK;
}

dart_ret_t dart__base__unit_locality__destruct(
  dart_unit_mapping_t   * unit_mapping)
{
  DART_LOG_DEBUG("dart__base__unit_locality__destruct() team: %d",
                 unit_mapping->team);

  if (NULL != unit_mapping) {
    if (NULL != unit_mapping->unit_localities) {
      free(unit_mapping->unit_localities);
      unit_mapping->unit_localities = NULL;
    }
    free(unit_mapping);
  }

  DART_LOG_DEBUG("dart__base__unit_locality__destruct >");
  return DART_OK;
}

/* ======================================================================== *
 * Lookup                                                                   *
 * ======================================================================== */

/**
 * Get the specified unit's locality information from a set of unit
 * mappings.
 */
dart_ret_t dart__base__unit_locality__at(
  dart_unit_mapping_t   * unit_mapping,
  dart_team_unit_t        unit,
  dart_unit_locality_t ** loc)
{
  if ((size_t)(unit.id) >= unit_mapping->num_units) {
    DART_LOG_ERROR("dart__base__unit_locality__get ! "
                   "unit id %d out of bounds, team size: %u",
                   unit.id, unit_mapping->num_units);
    return DART_ERR_INVAL;
  }
  *loc = &(unit_mapping->unit_localities[unit.id]);
  return DART_OK;
}

/* ======================================================================== *
 * Private Functions                                                        *
 * ======================================================================== */

/**
 * Initialize unit locality information from HW locality.
 */
dart_ret_t dart__base__unit_locality__local_unit_new(
  dart_team_t             team,
  dart_unit_locality_t  * uloc)
{
  DART_LOG_DEBUG("dart__base__unit_locality__local_unit_new() loc(%p)",
                 (void *)uloc);
  if (uloc == NULL) {
    DART_LOG_ERROR("dart__base__unit_locality__local_unit_new ! null");
    return DART_ERR_INVAL;
  }
  dart_team_unit_t myid;

  DART_ASSERT_RETURNS(
    dart__base__unit_locality__init(uloc),
    DART_OK);
  DART_ASSERT_RETURNS(
    dart_team_myid(team, &myid),
    DART_OK);

  dart_hwinfo_t hwinfo;
  DART_ASSERT_RETURNS(dart_hwinfo(&hwinfo), DART_OK);

#if 1 || __TODO__CLARIFY__
  /* Is this call required? */
  dart_domain_locality_t * tloc;
  DART_ASSERT_RETURNS(
    dart_domain_team_locality(team, ".", &tloc),
    DART_OK);
#endif

  uloc->unit   = myid;
  uloc->team   = team;
  uloc->hwinfo = hwinfo;

#if defined(DART__BASE__LOCALITY__SIMULATE_MICS)
  /* Assigns every second unit to a MIC host name.
   * Useful for simulating a heterogeneous node-level topology
   * for debugging.
   */
  if (myid % 3 == 1) {
    strncat(uloc->hwinfo.host, "-mic0", 5);
  }
#endif

  DART_LOG_DEBUG("dart__base__unit_locality__local_unit_new > loc(%p)",
                 (void *)uloc);
  return DART_OK;
}

/**
 * Default constructor of dart_unit_locality_t.
 */
dart_ret_t dart__base__unit_locality__init(
  dart_unit_locality_t  * loc)
{
  DART_LOG_TRACE("dart__base__unit_locality__init() loc: %p",
                 (void *)loc);
  if (loc == NULL) {
    DART_LOG_ERROR("dart__base__unit_locality__init ! null");
    return DART_ERR_INVAL;
  }

  loc->unit = DART_UNDEFINED_TEAM_UNIT_ID;
  loc->team = DART_UNDEFINED_TEAM_ID;

  dart_hwinfo_init(&loc->hwinfo);

  /*
  DART_ASSERT_RETURNS(
    dart__base__locality__domain__init(&loc->domain),
    DART_OK);
  */
  loc->domain_tag[0] = '\0';

  DART_LOG_TRACE("dart__base__unit_locality__init >");
  return DART_OK;
}

