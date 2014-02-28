#include <stdio.h>
#include "dart_types.h"
#include "dart_synchronization.h"

#include "adapt/dart_adapt_synchronization.h"
#include "adapt/dart_adapt_initialization.h"
#include "adapt/dart_adapt_globmem.h"
#include "adapt/dart_adapt_team_group.h"
#include "adapt/dart_adapt_communication.h"

dart_ret_t dart_team_lock_init (dart_team_t teamid, dart_lock_t* lock)
{
	       return dart_adapt_team_lock_init (teamid, lock);
}
dart_ret_t dart_lock_acquire (dart_lock_t lock)
{
	       return dart_adapt_lock_acquire (lock) ;
}
dart_ret_t dart_lock_try_acquire (dart_lock_t lock)
{
	       return dart_adapt_lock_try_acquire (lock);
}
dart_ret_t dart_lock_release (dart_lock_t lock)
{
	       return dart_adapt_lock_release (lock);
}
dart_ret_t dart_team_lock_free (dart_team_t teamid, dart_lock_t* lock)
{
	       return dart_adapt_team_lock_free (teamid, lock);
}




