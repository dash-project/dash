/**
 * \file dash/dart/mpi/dart_locality_priv.c
 *
 */

#include <dash/dart/mpi/dart_locality_priv.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_locality.h>

#include <dash/dart/base/logging.h>
#include <dash/dart/base/locality.h>
// #include <dash/dart/base/domain_map.h>


dart_ret_t dart__mpi__locality_init()
{
  DART_LOG_DEBUG("dart__mpi__locality_init()");
  dart_ret_t ret;

  ret = dart__base__locality__init();
  if (ret != DART_OK) {
    DART_LOG_ERROR("dart__mpi__locality_init: "
                   "dart__base__locality__init failed: %d", ret);
    return ret;
  }
#if 0
  /* get pointer to locality field of global domain entry in domain map: */
  dart_domain_locality_t * domain_map_g_dloc;
  ret = dart__base__domain_map__find(".", &domain_map_g_dloc);
  if (ret != DART_OK) {
    DART_LOG_ERROR("dart__mpi__locality_init: "
                   "dart__base__domain_map__find failed: %d", ret);
    return ret;
  }
  /* load locality descriptor for global domain into locality field of global
   * domain entry in domain map: */
  ret = dart__base__locality__global_domain_new(domain_map_g_dloc);
  if (ret != DART_OK) {
    DART_LOG_ERROR("dart__mpi__locality_init: "
                   "dart__base__locality__global_domain_new failed: %d", ret);
    return ret;
  }
#endif
  /* initialize locality descriptor for local unit: */
  dart_unit_locality_t l_uloc;
  ret = dart__base__locality__local_unit_new(&l_uloc);
  if (ret != DART_OK) {
    DART_LOG_ERROR("dart__mpi__locality_init: "
                   "dart__base__locality__local_unit_new failed: %d", ret);
    return ret;
  }
  DART_LOG_DEBUG("dart__mpi__locality_init >");
  return DART_OK;
}

dart_ret_t dart__mpi__locality_finalize()
{
  dart_ret_t ret;

// ret = dart__base__domain_map__finalize();
  ret = dart__base__locality__finalize();
  if (ret != DART_OK) {
    return ret;
  }

  return DART_OK;
}

