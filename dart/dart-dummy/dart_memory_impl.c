
#include "dart_types.h"
#include "dart_gptr.h"
#include "dart_memory.h"


dart_ret_t dart_memalloc(size_t nbytes, dart_gptr_t *gptr)
{
  return DART_OK;
}


dart_ret_t dart_memfree(dart_gptr_t gptr)
{
  return DART_OK;
}


dart_ret_t dart_team_memalloc_aligned(int32_t teamid, 
				      size_t nbytes, dart_gptr_t *gptr)
{
  return DART_OK;
}

dart_ret_t dart_team_memfree(int32_t teamid, dart_gptr_t gptr)
{
  return DART_OK;
}


