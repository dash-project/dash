
#include <string.h>

#include "dart.h"
#include "dart_mempool.h"
#include "dart_memarea.h"

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

dart_ret_t dart_get_blocking(void *dest, dart_gptr_t ptr, size_t nbytes)
{  
  dart_mempool pool;
  dart_memarea_t *mem;
  
  mem = dart_shmem_team_get_memarea(DART_TEAM_ALL);
  if(!mem) return DART_ERR_OTHER;

  pool = dart_memarea_get_mempool_unaligned(mem, 0);
  if(!pool) return DART_ERR_OTHER;

  char *addr = 
    ((char*)pool->shm_address) + ptr.addr_or_offs.offset;

  memcpy(dest, addr, nbytes);
  return DART_OK;
}

dart_ret_t dart_put_blocking(dart_gptr_t ptr, void *src, size_t nbytes)
{
  dart_ret_t ret = DART_OK;

  return ret;
}

