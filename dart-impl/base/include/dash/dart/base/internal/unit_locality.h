/**
 * \file dash/dart/base/internal/unit_locality.h
 */
#ifndef DART__BASE__INTERNAL__UNIT_LOCALITY_H__
#define DART__BASE__INTERNAL__UNIT_LOCALITY_H__

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

#include <unistd.h>
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

#if 0
/* Global array, one unit locality descriptor per unit. */
dart_gptr_t _dart__base__unit_locality__map;
dart_gptr_t _dart__base__unit_locality__map_local;
#else
dart_unit_locality_t * _dart__base__unit_locality__map;
#endif

dart_ret_t dart__base__unit_locality__init()
{
  dart_ret_t  ret;
  dart_unit_t myid;
  size_t      nunits;

  ret = dart_myid(&myid);
  ret = dart_size(&nunits);

  /* get local unit's locality information: */
  dart_unit_locality_t * uloc;
  ret = dart_locality_unit(myid, &uloc);

  /* all-to-all exchange of locality data across all units:
   * (send, recv, nbytes, team) */
  size_t     nbytes = nunits * sizeof(dart_unit_locality_t);
  _dart__base__unit_locality__map = (dart_unit_locality_t *)(
                                       malloc(nbytes));
  ret = dart_allgather(uloc, _dart__base__unit_locality__map, nbytes,
                       DART_TEAM_ALL);

#if 0
  /* using global memory to exchange locality information across all units
   * is only beneficial if locality information could change in
   * non-collective operations.
   * Using allgather for now.
   */
  ret = dart_team_memalloc_aligned(
          dart_team_all,
          sizeof(dart_unit_locality_t),
          &_dart__base__unit_locality__map);
  _dart__base__unit_locality__map_local = _dart__base__unit_locality__map;
  /* global pointer to local unit's shared locality descriptor: */
  dart_gptr_setunit(&_dart__base__unit_locality__map_local, myid);
#endif

  return DART_OK;
}

dart_ret_t dart__base__unit_locality__finalize()
{
  dart_ret_t ret;

  free(_dart__base__unit_locality__map);

#if 0
  ret = dart_team_memfree(
          dart_team_all,
          _dart__base__unit_locality__map);
#endif

  return DART_OK;
}

#endif /* DART__BASE__INTERNAL__UNIT_LOCALITY_H__ */
