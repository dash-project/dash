/**
 * \file dash/dart/mpi/dart_locality_priv.c
 *
 */

#include <dash/dart/mpi/dart_locality_priv.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_locality.h>

#include <dash/dart/base/domain_map.h>


dart_ret_t dart__mpi__locality_init()
{
  dart_ret_t ret;

  ret = dart__base__domain_map__init();
  if (ret != DART_OK) {
    return ret;
  }

  return DART_OK;
}

dart_ret_t dart__mpi__locality_finalize()
{
  dart_ret_t ret;

  ret = dart__base__domain_map__finalize();
  if (ret != DART_OK) {
    return ret;
  }

  return DART_OK;
}

