
#include "dart_types.h"
#include "dart_synchronization.h"

dart_ret_t dart_team_lock_init(dart_team_t teamid, 
			       dart_lock_t* lock)
{
  return DART_OK;
}

dart_ret_t dart_team_lock_free(dart_team_t teamid,
			       dart_lock_t* lock)
{
  return DART_OK;
}

dart_ret_t dart_lock_acquire(dart_lock_t lock)
{
  return DART_OK;
}


dart_ret_t dart_lock_try_acquire(dart_lock_t lock)
{
  return DART_OK;
}

dart_ret_t dart_lock_release(dart_lock_t lock)
{
  return DART_OK;
}
