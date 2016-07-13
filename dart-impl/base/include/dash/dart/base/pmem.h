/**
 * \file dash/dart/base/pmem.h
 */
#ifndef DART__BASE__PMEM_H__
#define DART__BASE__PMEM_H__

#include <dash/dart/if/dart_types.h>
#include <libpmemobj.h>

typedef PMEMobjpool dart_pmem_pool_handle_t;

/* ======================================================================== *
 * Init / Finalize                                                          *
 * ======================================================================== */

dart_ret_t dart__base__pmem__init();

dart_ret_t dart__base__pmem__finalize();

/* ======================================================================== *
 * Create / Delete                                                          *
 * ======================================================================== */

dart_pmem_pool_t * dart__base__pmem__pool_create(
  dart_team_t   team
  const char  * path,
  const char  * layout,
  size_t        size);

dart_pmem_pool_t * dart__base__pmem__open(
  dart_team_t   team,
  const char  * path,
  const char  * layout);

dart_ret_t dart__base__pmem__close(
  dart_pmem_pool_t pool);

/* ======================================================================== *
 * Persistent Memory Allocation                                             *
 * ======================================================================== */

dart_ret_t  dart__base__pmem__alloc(
  dart_pmem_pool_t  * pool,
  size_t              nel,
  size_t              elsize,
  dart_gptr_t       * gptr)
  
dart_ret_t  dart__base__pmem__free(
  dart_gptr_t       * gptr)

#endif /* DART__BASE__PMEM_H__ */
