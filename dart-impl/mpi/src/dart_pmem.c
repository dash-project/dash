/**
 * \file dart_pmem.c
 *
 * Implementation of dart persistent memory
 */
#include <stdio.h>
#include <string.h>
#include <linux/limits.h>
#include <sys/stat.h>

#include <mpi.h>

#include <dash/dart/if/dart_team_group.h>
#include <dash/dart/if/dart_pmem.h>

#include <dash/dart/base/pmem.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/base/assert.h>

#ifdef DASH_ENABLE_PMEM

void * alloc_mem(size_t size);
void free_mem(void * ptr);


void * alloc_mem(size_t size)
{
  char * baseptr;

  DART_ASSERT_RETURNS(MPI_Alloc_mem(size, MPI_INFO_NULL, &baseptr), MPI_SUCCESS);
  return baseptr;
}

void free_mem(void * ptr)
{
  DART_ASSERT_RETURNS(MPI_Free_mem(ptr), MPI_SUCCESS);
}

dart_ret_t dart__mpi__pmem_init(void)
{
  return dart__base__pmem_init(alloc_mem, free_mem, NULL, NULL);
}

dart_ret_t dart__mpi__pmem_finalize(void)
{
  return dart__base__pmem_finalize();
}

/* ----------------------------------------------------------------------- *
 * Implementation of DART PMEM Interface                                   *
 * ----------------------------------------------------------------------- */

dart_pmem_pool_t * dart__pmem__pool_open(
  dart_team_t   team,
  const char  * name,
  int           flags,
  mode_t        mode)
{
  return dart__base__pmem__pool_open(team, name, flags, mode);
}

dart_pmem_oid_t dart__pmem__alloc(
  dart_pmem_pool_t  const * pool,
  size_t                    nbytes)
{
  return dart__base__pmem__alloc(pool, nbytes);
}

dart_ret_t  dart__pmem__free(
  dart_pmem_pool_t  const *     pool,
  dart_pmem_oid_t               poid)
{
  return dart__base__pmem__free(pool, poid);
}

dart_ret_t dart__pmem__get_addr(
  dart_pmem_oid_t oid,
  void ** addr)
{
  return dart__base__pmem__get_addr(oid, addr);
}

dart_ret_t dart__pmem__persist_addr(
  dart_pmem_pool_t const * pool,
  void * addr,
  size_t nbytes)
{
  return dart__base__pmem__persist_addr(pool, addr, nbytes);
}

dart_ret_t dart__pmem__pool_stat(
  dart_pmem_pool_t const * pool,
  struct dart_pmem_pool_stat * stat)
{

  return dart__base__pmem__pool_stat(pool, stat);
}

dart_ret_t dart__pmem__fetch_all(
  dart_pmem_pool_t const * pool,
  dart_pmem_oid_t * buf)
{
  return dart__base__pmem__fetch_all(pool, buf);
}

dart_ret_t dart__pmem__sizeof_oid(
  dart_pmem_pool_t const * pool,
  dart_pmem_oid_t oid,
  size_t * size)
{
  return  dart__base__pmem__sizeof_oid(pool, oid, size);
}

dart_ret_t dart__pmem__pool_close(
  dart_pmem_pool_t ** pool)
{
  return dart__base__pmem__pool_close(pool);
}
#endif
