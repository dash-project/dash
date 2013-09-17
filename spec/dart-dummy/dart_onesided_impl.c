
#include "dart_types.h"
#include "dart_globmem.h"
#include "dart_communication.h"
#include "dart_onesided_impl.h"

#include <string.h>

dart_ret_t dart_get_blocking(void *dest, 
			     dart_gptr_t ptr, size_t nbytes)
{
  memset(dest, 42, nbytes);
  return DART_OK;
}

dart_ret_t dart_put_blocking(dart_gptr_t ptr, 
			     void *src, size_t nbytes)
{
  return DART_OK;
}

dart_ret_t dart_get(void *dest, dart_gptr_t ptr, 
		       size_t nbytes, dart_handle_t *handle)
{
  (*handle) = (dart_handle_t) 
    malloc(sizeof(struct dart_handle_struct));
  
  return DART_OK;
}

dart_ret_t dart_put(dart_gptr_t ptr, void *src, 
		       size_t nbytes, dart_handle_t *handle)
{
  (*handle) = (dart_handle_t) 
    malloc(sizeof(struct dart_handle_struct));
  return DART_OK;
}

dart_ret_t dart_wait(dart_handle_t handle)
{
  if( handle!=0 ) {
    free(handle);
  }
  return DART_OK;
}

dart_ret_t dart_test(dart_handle_t handle)
{
  if( handle!=0 ) {
    free(handle);
  }
  return DART_OK;
}

dart_ret_t dart_waitall(dart_handle_t *handle, size_t n)
{
  return DART_OK;
}

dart_ret_t dart_testall(dart_handle_t *handle, size_t n)
{
  return DART_OK;
}
