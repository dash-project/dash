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
#include <dash/dart/base/logging.h>
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
#include <dash/dart/base/internal/unit_locality.h>


dart_unit_locality_t * _dart__base__unit_locality__map;
size_t                 _dart__base__unit_locality__team_size_all;



dart_ret_t dart__base__unit_locality__data(
  dart_unit_locality_t ** loc)
{
  *loc = _dart__base__unit_locality__map;
  return DART_OK;
}

dart_ret_t dart__base__unit_locality__at(
  dart_unit_t             unit,
  dart_unit_locality_t ** loc)
{
  if ((size_t)(unit) >= _dart__base__unit_locality__team_size_all) {
    DART_LOG_ERROR("dart__base__unit_locality__get ! "
                   "unit id %d out of bounds, team size: %"PRIu64"",
                   unit, _dart__base__unit_locality__team_size_all);
    return DART_ERR_INVAL;
  }
  *loc = _dart__base__unit_locality__map + unit;
  return DART_OK;
}

dart_ret_t dart__base__unit_locality__init()
{
  dart_ret_t  ret;
  dart_unit_t myid   = DART_UNDEFINED_UNIT_ID;
  size_t      nunits = 0;
  DART_LOG_DEBUG("dart__base__unit_locality__init()");

  DART_ASSERT_RETURNS(dart_myid(&myid),   DART_OK);
  DART_ASSERT_RETURNS(dart_size(&nunits), DART_OK);

  _dart__base__unit_locality__team_size_all = nunits;

  size_t nbytes = sizeof(dart_unit_locality_t);

  /* get local unit's locality information: */
  dart_unit_locality_t * uloc;
  uloc = (dart_unit_locality_t *)(malloc(sizeof(dart_unit_locality_t)));
  ret  = dart__base__locality__local_unit_new(uloc);
  if (ret != DART_OK) {
    DART_LOG_ERROR("dart__base__unit_locality__init ! "
                   "dart__base__locality__local_unit_new failed: %d", ret);
    return ret;
  }
  DART_LOG_TRACE("dart__base__unit_locality__init: unit %d of %"PRIu64": "
                 "sending %"PRIu64" bytes: "
                 "host:%s domain:%s core_id:%d numa_id:%d nthreads:%d",
                 myid, nunits, nbytes,
                 uloc->host, uloc->domain_tag, uloc->hwinfo.cpu_id,
                 uloc->hwinfo.numa_id, uloc->hwinfo.max_threads);

  _dart__base__unit_locality__map = (dart_unit_locality_t *)(
                                       malloc(nunits * nbytes));
  dart_barrier(DART_TEAM_ALL);

  /* all-to-all exchange of locality data across all units:
   * (send, recv, nbytes, team) */
  DART_LOG_DEBUG("dart__base__unit_locality__init: dart_allgather");
  ret = dart_allgather(uloc, _dart__base__unit_locality__map, nbytes,
                       DART_TEAM_ALL);

  dart_barrier(DART_TEAM_ALL);
  free(uloc);

  if (ret != DART_OK) {
    DART_LOG_ERROR("dart__base__unit_locality__init ! "
                   "dart_allgather failed: %d", ret);
    return ret;
  }
#ifdef DART_ENABLE_LOGGING
  for (size_t u = 0; u < nunits; ++u) {
    dart_unit_locality_t * ulm_u = &_dart__base__unit_locality__map[u];
    DART_LOG_TRACE("dart__base__unit_locality__init: unit[%d]: "
                   "unit:%d host:%s domain:%s num_cores:%d core_id:%d "
                   "numa_id:%d nthreads:%d",
                   u, ulm_u->unit, ulm_u->host, ulm_u->domain_tag,
                   ulm_u->hwinfo.num_cores, ulm_u->hwinfo.cpu_id,
                   ulm_u->hwinfo.numa_id, ulm_u->hwinfo.max_threads);
  }
#endif

  DART_LOG_DEBUG("dart__base__unit_locality__init >");
  return DART_OK;
}

dart_ret_t dart__base__unit_locality__finalize()
{
  DART_LOG_DEBUG("dart__base__unit_locality__finalize()");

  dart_barrier(DART_TEAM_ALL);
  free(_dart__base__unit_locality__map);

  DART_LOG_DEBUG("dart__base__unit_locality__finalize >");
  return DART_OK;
}

