
#include <string.h>

#include <dash/dart/if/dart.h>
#include <dash/dart/shmem/dart_mempool.h>
#include <dash/dart/shmem/dart_memarea.h>

/* --- TO IMPLEMENT --- 

// blocking versions of one-sided communication operations
dart_ret_t dart_get_blocking(void *dest, dart_gptr_t ptr, size_t nbytes);
dart_ret_t dart_put_blocking(dart_gptr_t ptr, void *src, size_t nbytes);

// non-blocking versions returning a handle
dart_ret_t dart_get(void *dest, dart_gptr_t ptr, 
		    size_t nbytes, dart_handle_t *handle);
dart_ret_t dart_put(dart_gptr_t ptr, void *src, 
		    size_t nbytes, dart_handle_t *handle);
  
// wait and test for the completion of a single handle
dart_ret_t dart_wait(dart_handle_t handle);
dart_ret_t dart_test(dart_handle_t handle);

// wait and test for the completion of multiple handles 
dart_ret_t dart_waitall(dart_handle_t *handle, size_t n);
dart_ret_t dart_testall(dart_handle_t *handle, size_t n);

*/

dart_ret_t dart_get_blocking(void *dest, 
			     dart_gptr_t ptr, size_t nbytes)
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

  /*
  {
    char buf[200];
    GPTR_SPRINTF(buf, ptr);

    fprintf(stderr, "[%0d] GETting %d bytes from addr=%0x base_addr=%x\n", 
	    myid, nbytes, addr, ((char*)pool->base_addr) );
  }
  */

  memcpy(dest, addr, nbytes);
  return DART_OK;
}

dart_ret_t dart_put_blocking(dart_gptr_t ptr, 
			     void *src, size_t nbytes)
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

  /*
  {
    char buf[200];
    GPTR_SPRINTF(buf, ptr);

    fprintf(stderr, "[%0d] GETting %d bytes from addr=%0x base_addr=%x\n", 
	    myid, nbytes, addr, ((char*)pool->base_addr) );
  }
  */

  memcpy(addr, src, nbytes);
  return DART_OK;
}

