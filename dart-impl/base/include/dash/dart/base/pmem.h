/**
 * \file dash/dart/base/pmem.h
 */
#ifndef DART__BASE__PMEM_H__INCLUDED
#define DART__BASE__PMEM_H__INCLUDED

#ifdef DASH_ENABLE_PMEM

#include <dash/dart/if/dart_types.h>
#include <libpmemobj.h>


/* ======================================================================== *
 * Init / Finalize                                                          *
 * ======================================================================== */

dart_ret_t dart__base__pmem_init(
  void * (*malloc_func)(size_t size),
  void (*free_func)(void * ptr),
  void * (*realloc_func)(void * ptr, size_t size),
  char * (*strdup_func)(const char * s));

dart_ret_t dart__base__pmem_finalize(void);

/* ======================================================================== *
 * Open / Close                                                             *
 * ======================================================================== */


dart_pmem_pool_t * dart__base__pmem__pool_open(
  dart_team_t         team,
  char const     *    name,
  int                 flags,
  mode_t              mode);

dart_ret_t dart__base__pmem__pool_close(
  dart_pmem_pool_t ** pool);

/* ======================================================================== *
 * Persistent Memory Allocation                                             *
 * ======================================================================== */

dart_pmem_oid_t dart__base__pmem__alloc(
  dart_pmem_pool_t  const * pool,
  size_t              nbytes);

dart_ret_t  dart__base__pmem__free(
  dart_pmem_pool_t  const *     pool,
  dart_pmem_oid_t         poid);


dart_ret_t dart__base__pmem__fetch_all(
    dart_pmem_pool_t const * pool,
    dart_pmem_oid_t * buf);

dart_ret_t dart__base__pmem__get_addr(
  dart_pmem_oid_t oid,
  void ** addr);

dart_ret_t dart__base__pmem__persist_addr(
  dart_pmem_pool_t const * pool,
  void * addr,
  size_t nbytes);

dart_ret_t dart__base__pmem__sizeof_oid(
  dart_pmem_pool_t const * pool,
  dart_pmem_oid_t oid,
  size_t * size);

/* ======================================================================== *
 * Other functions                                                          *
 * ======================================================================== */

dart_ret_t dart__base__pmem__pool_stat(
  dart_pmem_pool_t const * pool,
  struct dart_pmem_pool_stat * stat);

#endif

#endif /* DART__BASE__PMEM_H__ */
