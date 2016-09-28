#ifndef DART_PMEM_H_INCLUDED
#define DART_PMEM_H_INCLUDED

#ifdef DASH_ENABLE_PMEM

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_globmem.h>


#define DART_INTERFACE_ON

/**
 * \file dart_pmem.h
 *
 * Routines for allocation and reclamation of persistent memory regions
 * in global address space
 */

/**
 * \defgroup  DartPersistentMemory    Persistent memory semantics
 * \ingroup   DartInterface
 */

#define DART_PMEM_FILE_CREATE (1 << 0)
#define DART_PMEM_FILE_EXCL   (1 << 1)

static dart_pmem_oid_t const DART_PMEM_OID_NULL = {{0, 0}};

/* ======================================================================== *
 * Open and Close                                                           *
 * ======================================================================== */

dart_pmem_pool_t * dart__pmem__pool_open(
  dart_team_t         team,
  char const     *    name,
  int                 flags,
  mode_t              mode);

dart_ret_t dart__pmem__pool_close(
  dart_pmem_pool_t ** pool);

/* ======================================================================== *
 * Persistent Memory Allocation                                             *
 * ======================================================================== */

dart_pmem_oid_t dart__pmem__alloc(
  dart_pmem_pool_t  const * pool,
  size_t              nbytes);

dart_ret_t  dart__pmem__free(
  dart_pmem_pool_t  const *     pool,
  dart_pmem_oid_t               poid);


dart_ret_t dart__pmem__fetch_all(
    dart_pmem_pool_t const * pool,
    dart_pmem_oid_t * buf
);

dart_ret_t dart__pmem__get_addr(
  dart_pmem_oid_t oid,
  void ** addr);

dart_ret_t dart__pmem__persist_addr(
  dart_pmem_pool_t const * pool,
  void * addr,
  size_t nbytes);

dart_ret_t dart__pmem__sizeof_oid(
  dart_pmem_pool_t const * pool,
  dart_pmem_oid_t oid,
  size_t * size
);

/* ======================================================================== *
 * Other functions                                                          *
 * ======================================================================== */

dart_ret_t dart__pmem__pool_stat(
  dart_pmem_pool_t const * pool,
  struct dart_pmem_pool_stat * stat
);


#define DART_INTERFACE_OFF

#ifdef __cplusplus
}
#endif

#endif

#endif /* DART_PMEM_H_INCLUDED */
