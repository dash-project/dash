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

dart_ret_t dart__base__unit_locality__unit_locality_init(
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
  dart_unit_t myid   = DART_UNDEFINED_UNIT_ID;
  size_t      nunits = 0;
  *unit_mapping      = NULL;
  DART_LOG_DEBUG("dart__base__unit_locality__create()");

  DART_ASSERT_RETURNS(dart_team_myid(team, &myid),   DART_OK);
  DART_ASSERT_RETURNS(dart_team_size(team, &nunits), DART_OK);

  dart_unit_mapping_t * mapping = malloc(sizeof(dart_unit_mapping_t));
  mapping->num_units            = nunits;

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
  DART_LOG_TRACE("dart__base__unit_locality__create: unit %d of %"PRIu64": "
                 "sending %"PRIu64" bytes: "
                 "host:%s domain:%s core_id:%d numa_id:%d nthreads:%d",
                 myid, nunits, nbytes,
                 uloc->host, uloc->domain_tag, uloc->hwinfo.cpu_id,
                 uloc->hwinfo.numa_id, uloc->hwinfo.max_threads);

  mapping->unit_localities = (dart_unit_locality_t *)(
                                malloc(nunits * nbytes));
  dart_barrier(team);

  /* all-to-all exchange of locality data across all units:
   * (send, recv, nbytes, team) */
  DART_LOG_DEBUG("dart__base__unit_locality__create: dart_allgather");
  ret = dart_allgather(uloc, mapping->unit_localities, nbytes,
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
                   "unit:%d host:%s domain:%s "
                   "num_cores:%d cpu_id:%d "
                   "num_numa:%d numa_id:%d "
                   "nthreads:%d",
                   u, ulm_u->unit, ulm_u->host, ulm_u->domain_tag,
                   ulm_u->hwinfo.num_cores, ulm_u->hwinfo.cpu_id,
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

  free(unit_mapping->unit_localities);

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
  dart_unit_t             unit,
  dart_unit_locality_t ** loc)
{
  if ((size_t)(unit) >= unit_mapping->num_units) {
    DART_LOG_ERROR("dart__base__unit_locality__get ! "
                   "unit id %d out of bounds, team size: %"PRIu64"",
                   unit, unit_mapping->num_units);
    return DART_ERR_INVAL;
  }
  *loc = (unit_mapping->unit_localities + unit);
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
  dart_unit_locality_t  * loc)
{
  DART_LOG_DEBUG("dart__base__unit_locality__local_unit_new() loc(%p)", loc);
  if (loc == NULL) {
    DART_LOG_ERROR("dart__base__unit_locality__local_unit_new ! null");
    return DART_ERR_INVAL;
  }
  dart_unit_t myid = DART_UNDEFINED_UNIT_ID;

  DART_ASSERT_RETURNS(
    dart__base__unit_locality__unit_locality_init(loc),
    DART_OK);
  DART_ASSERT_RETURNS(
    dart_team_myid(team, &myid),
    DART_OK);

  dart_hwinfo_t * hwinfo = malloc(sizeof(dart_hwinfo_t));
  DART_ASSERT_RETURNS(dart_hwinfo(hwinfo), DART_OK);

  /* Domain tag is unknown at this point, initialize with global domain: */
  strncpy(loc->domain_tag, ".", 1);
  loc->domain_tag[1] = '\0';

  dart_domain_locality_t * dloc;
  DART_ASSERT_RETURNS(
    dart_domain_team_locality(team, ".", &dloc),
    DART_OK);

  loc->unit   = myid;
  loc->team   = team;
  loc->hwinfo = *hwinfo;

  char hostname[DART_LOCALITY_HOST_MAX_SIZE];
  gethostname(hostname, DART_LOCALITY_HOST_MAX_SIZE);
  strncpy(loc->host, hostname, DART_LOCALITY_HOST_MAX_SIZE);

#if defined(DART__BASE__LOCALITY__SIMULATE_MICS)
  /* Assigns every second unit to a MIC host name.
   * Useful for simulating a heterogeneous node-level topology
   * for debugging.
   */
  if (myid % 2 == 1) {
    strncpy(loc->host + strlen(hostname), "-mic0", 5);
  }
#endif

  DART_LOG_DEBUG("dart__base__unit_locality__local_unit_new > loc(%p)", loc);
  return DART_OK;
}

/**
 * Default constructor of dart_unit_locality_t.
 */
dart_ret_t dart__base__unit_locality__unit_locality_init(
  dart_unit_locality_t  * loc)
{
  DART_LOG_TRACE("dart__base__unit_locality__unit_locality_init() "
                 "loc: %p", loc);
  if (loc == NULL) {
    DART_LOG_ERROR("dart__base__unit_locality__unit_locality_init ! null");
    return DART_ERR_INVAL;
  }
  loc->unit                  = DART_UNDEFINED_UNIT_ID;
  loc->team                  = DART_UNDEFINED_TEAM_ID;
  loc->domain_tag[0]         = '\0';
  loc->host[0]               = '\0';
  loc->hwinfo.numa_id        = -1;
  loc->hwinfo.cpu_id         = -1;
  loc->hwinfo.num_cores      = -1;
  loc->hwinfo.min_threads    = -1;
  loc->hwinfo.max_threads    = -1;
  loc->hwinfo.max_cpu_mhz    = -1;
  loc->hwinfo.min_cpu_mhz    = -1;
  loc->hwinfo.max_shmem_mbps = -1;
  DART_LOG_TRACE("dart__base__unit_locality__unit_locality_init >");
  return DART_OK;
}

