/**
 * \file dash/dart/mpi/dart_locality_priv.c
 *
 */

#include <dash/dart/mpi/dart_locality_priv.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_communication.h>
#include <dash/dart/if/dart_locality.h>
#include <dash/dart/if/dart_team_group.h>

#include <dash/dart/base/logging.h>
#include <dash/dart/base/locality.h>


dart_ret_t dart__mpi__locality_init()
{
  DART_LOG_DEBUG("dart__mpi__locality_init()");
  dart_ret_t ret;

  ret = dart__base__locality__init();
  if (ret != DART_OK) {
    DART_LOG_ERROR("dart__mpi__locality_init ! "
                   "dart__base__locality__init failed: %d", ret);
    return ret;
  }
  DART_LOG_DEBUG("dart__mpi__locality_init >");
  return DART_OK;
}

dart_ret_t dart__mpi__locality_finalize()
{
  DART_LOG_DEBUG("dart__mpi__locality_finalize()");
  dart_ret_t ret;

  ret = dart__base__locality__finalize();

  dart_barrier(DART_TEAM_ALL);

  if (ret != DART_OK) {
    DART_LOG_ERROR("dart__mpi__locality_finalize ! "
                   "dart__base__locality__finalize failed: %d", ret);
    return ret;
  }
  DART_LOG_DEBUG("dart__mpi__locality_finalize >");
  return DART_OK;
}

