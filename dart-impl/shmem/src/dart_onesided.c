
#include <string.h>

#include <dash/dart/base/logging.h>
#include <dash/dart/if/dart.h>
#include <dash/dart/if/dart_types.h>
#include <dash/dart/shmem/dart_mempool.h>
#include <dash/dart/shmem/dart_memarea.h>

dart_ret_t dart_get(
  void *dest,
  dart_gptr_t ptr, 
  size_t nbytes)
{
  // TODO: Blocking dummy implementation
  return dart_get_blocking(dest, ptr, nbytes);
}

dart_ret_t dart_put(
  dart_gptr_t ptr,
  void *src, 
  size_t nbytes)
{
  // TODO: Blocking dummy implementation
  return dart_put_blocking(ptr, src, nbytes);
}

dart_ret_t dart_accumulate_int(
  dart_gptr_t ptr_dest,
  int *values,
  size_t nvalues,
  dart_operation_t op,
  dart_team_t team)
{
  int             * addr;
  int               poolid;
  dart_unit_t       myid;
  dart_mempoolptr   pool;

  poolid = ptr_dest.segid;
  pool   = dart_memarea_get_mempool_by_id(poolid);
  if(!pool) {
    return DART_ERR_OTHER;
  }

  dart_myid(&myid);
  
  addr = ((int*)pool->localbase_addr) +
         ((ptr_dest.unitid-myid) * (pool->localsz)) +
         ptr_dest.addr_or_offs.offset;

  // TODO: Interpret 'op' for requested operation, using addition
  //       for now.
  DART_LOG_DEBUG("ACC  - %d elements, addr: %p", nvalues, addr);
  for (size_t i = 0; i < nvalues; i++) {
    int * ptr_dest = addr + i;
    int * ptr_src  = &values[i];
    int exp_value  = *(ptr_dest);
    int new_value  = exp_value + *(ptr_src);
    int old_value;
    // Compare-and-Swap single elements.
    for(;;) {
      DART_LOG_DEBUG("ACC  - CAS on element %d to %d", i, new_value);
      old_value = __sync_val_compare_and_swap(
        ptr_dest,
        exp_value,
        new_value);
      if (old_value == exp_value) {
        // Assume success, disregarding ABA for now
        DART_LOG_DEBUG("ACC  - CAS succeeded on element %d ", i);
        break;
      }
      exp_value = old_value;
      new_value = exp_value + *(ptr_src);
    }
  }

  return DART_OK;
}

dart_ret_t dart_get_handle(
  void *dest,
  dart_gptr_t ptr, 
	size_t nbytes,
  dart_handle_t *handle)
{
  return DART_ERR_OTHER;
}

dart_ret_t dart_put_handle(
  dart_gptr_t ptr,
  void *src, 
	size_t nbytes,
  dart_handle_t *handle)
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
  size_t nbytes)
{  
  char *addr;
  int poolid;
  dart_unit_t myid;
  dart_mempoolptr pool;

  poolid = ptr.segid;
  pool = dart_memarea_get_mempool_by_id(poolid);
  
  if(!pool) 
    return DART_ERR_OTHER;

  dart_myid(&myid);

  addr = ((char*)pool->localbase_addr) +
    ((ptr.unitid-myid)*(pool->localsz)) +
    ptr.addr_or_offs.offset;

  memcpy(dest, addr, nbytes);
  return DART_OK;
}

dart_ret_t dart_put_blocking(
  dart_gptr_t ptr, 
  void *src,
  size_t nbytes)
{
  char *addr;
  int poolid;
  dart_unit_t myid;
  dart_mempoolptr pool;

  poolid = ptr.segid;
  pool = dart_memarea_get_mempool_by_id(poolid);
  
  if(!pool) 
    return DART_ERR_OTHER;

  dart_myid(&myid);
  
  addr = ((char*)pool->localbase_addr) +
    ((ptr.unitid-myid)*(pool->localsz)) +
    ptr.addr_or_offs.offset;

  memcpy(addr, src, nbytes);
  return DART_OK;
}

