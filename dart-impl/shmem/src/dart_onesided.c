
#include <string.h>

#include <dash/dart/base/logging.h>
#include <dash/dart/if/dart.h>
#include <dash/dart/if/dart_types.h>
#include <dash/dart/shmem/dart_mempool.h>
#include <dash/dart/shmem/dart_memarea.h>
#include <dash/dart/shmem/dart_malloc.h>

dart_ret_t dart_get(
  void *dest,
  dart_gptr_t ptr,
  size_t nelem,
  dart_datatype_t dtype) DART_NOTHROW
{
  return dart_get_blocking(dest, ptr, nelem, dtype);
}

dart_ret_t dart_put(
  dart_gptr_t  ptr,
  const void * src,
  size_t       nelem,
  dart_datatype_t dtype)
{
  return dart_put_blocking(ptr, src, nelem, dtype);
}

/*
 * dart_accumulate_* implementation, to be defined as macro
 * DART_SHMEM_DEFINE_ACCUMULATE(type)
 */

int dart_shmem_reduce_int(int a, int b, dart_operation_t op) {
  switch (op) {
    case DART_OP_MIN  : return (a < b) ? a : b;
    case DART_OP_MAX  : return (a > b) ? a : b;
    case DART_OP_SUM  : return a + b;
    case DART_OP_PROD : return a * b;
    case DART_OP_BAND : return a & b;
    case DART_OP_LAND : return a && b;
    case DART_OP_BOR  : return a | b;
    case DART_OP_LOR  : return a || b;
    case DART_OP_BXOR : return a ^ b;
    case DART_OP_LXOR : return (!a) ? b : !b;
    default           : DART_LOG_ERROR("Unknown reduce operation (%d)",
                                       (int)op);
                        return 0;
  }
}

dart_ret_t dart_accumulate(
  dart_gptr_t      ptr_dest,
  const void     * values,
  size_t           nvalues,
  dart_datatype_t  dtype,
  dart_operation_t op) DART_NOTHROW
{
  int                * addr;
  int                  poolid;
  dart_global_unit_t   myid;
  dart_mempoolptr      pool;

  if (dtype != DART_TYPE_INT) {
    DART_LOG_ERROR("dart_accumulate: "
                   "only datatype DART_TYPE_INT supported");
    return DART_ERR_INVAL;
  }

  dart_myid(&myid);
  poolid = ptr_dest.segid;
  pool   = dart_memarea_get_mempool_by_id(poolid);
  if(!pool) {
    return DART_ERR_OTHER;
  }
  addr   = ((int*)(pool->localbase_addr)) +               /* pool base addr */
           ((ptr_dest.unitid - myid.id) * (pool->localsz)) + /* unit offset    */
           ptr_dest.addr_or_offs.offset;                  /* element offset */
  DART_LOG_DEBUG("ACC  - t:%d o:%d, pool:%d lbase:%p lsz:%d offs:%d",
                  ptr_dest.unitid,
                  myid,
                  poolid,
                  (int*)(pool->localbase_addr),
                  pool->localsz,
                  ptr_dest.addr_or_offs.offset);
  DART_LOG_DEBUG("ACC  - %d elements, addr: %p", nvalues, addr);
  for (size_t i = 0; i < nvalues; i++) {
    const int * ptr_src  = values+i;
    int * ptr_dest = &addr[i];
    int exp_value  = *(ptr_dest);
    int new_value;
    DART_LOG_DEBUG("ACC  - CAS on element %d (%p) to %d",
                   i, ptr_dest, new_value);
    int old_value;
    /* Compare-and-Swap single elements */
    for(;;) {
      new_value = dart_shmem_reduce_int(exp_value, *(ptr_src), op);
      old_value = __sync_val_compare_and_swap(
        ptr_dest,
        exp_value,
        new_value);
      if (old_value == exp_value) {
        /* Assume success, disregarding ABA for now */
        DART_LOG_DEBUG("ACC  - CAS succeeded on element %d ", i);
        break;
      }
      exp_value = old_value;
    }
  }
  return DART_OK;
}

dart_ret_t dart_get_handle(
  void *dest,
  dart_gptr_t ptr,
	size_t nelem,
  dart_datatype_t dtype,
  dart_handle_t *handle)
{
  return DART_ERR_OTHER;
}

dart_ret_t dart_put_handle(
  dart_gptr_t     ptr,
  const void    * src,
	size_t          nelem,
  dart_datatype_t dtype,
  dart_handle_t * handle)
{
  return DART_ERR_OTHER;
}

dart_ret_t dart_flush(
  dart_gptr_t gptr)
{
  // No flush needed for SHMEM
  return DART_OK;
}

dart_ret_t dart_flush_all(
  dart_gptr_t gptr)
{
  // No flush needed for SHMEM
  return DART_OK;
}

dart_ret_t dart_flush_local(
  dart_gptr_t gptr)
{
  // No flush needed for SHMEM
  return DART_OK;
}

dart_ret_t dart_flush_local_all(
  dart_gptr_t gptr)
{
  // No flush needed for SHMEM
  return DART_OK;
}

dart_ret_t dart_wait(
  dart_handle_t handle)
{
  // TODO: Not implemented
  return DART_ERR_OTHER;
}

dart_ret_t dart_test(
  dart_handle_t handle)
{
  // TODO: Not implemented
  return DART_ERR_OTHER;
}

dart_ret_t dart_waitall_local(
  dart_handle_t * handle,
  size_t          num_handles)
{
  return dart_waitall(handle, num_handles);
}

dart_ret_t dart_waitall(
  dart_handle_t *handle,
  size_t n)
{
  return DART_ERR_OTHER;
}

dart_ret_t dart_testall(
  dart_handle_t *handle,
  size_t n)
{
  // TODO: Not implemented
  return DART_ERR_OTHER;
}

dart_ret_t dart_get_blocking(
  void *dest,
	dart_gptr_t ptr,
  size_t nelem,
  dart_datatype_t dtype) DART_NOTHROW
{
  char *addr;
  int poolid;
  dart_global_unit_t myid;
  dart_mempoolptr pool;
  size_t nbytes = nelem * dart__shmem__datatype_sizeof(dtype);

  poolid = ptr.segid;
  pool = dart_memarea_get_mempool_by_id(poolid);

  if(!pool)
    return DART_ERR_OTHER;

  dart_myid(&myid);

  addr = ((char*)pool->localbase_addr) +
    ((ptr.unitid-myid.id)*(pool->localsz)) +
    ptr.addr_or_offs.offset;

  memcpy(dest, addr, nbytes);
  return DART_OK;
}

dart_ret_t dart_put_blocking(
  dart_gptr_t  ptr,
  const void * src,
  size_t       nelem,
  dart_datatype_t dtype)
{
  char *addr;
  int poolid;
  dart_global_unit_t myid;
  dart_mempoolptr pool;
  size_t nbytes = nelem * dart__shmem__datatype_sizeof(dtype);

  poolid = ptr.segid;
  pool = dart_memarea_get_mempool_by_id(poolid);

  if(!pool)
    return DART_ERR_OTHER;

  dart_myid(&myid);

  addr = ((char*)pool->localbase_addr) +
    ((ptr.unitid-myid.id)*(pool->localsz)) +
    ptr.addr_or_offs.offset;

  memcpy(addr, src, nbytes);
  return DART_OK;
}

