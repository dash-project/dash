#ifndef DART_SYNC_H_INCLUDED
#define DART_SYNC_H_INCLUDED

#include <pthread.h>
#include <dash/dart/shmem/shmem_barriers_if.h>


dart_ret_t dart_team_lock_init(dart_team_t teamid, 
			       dart_lock_t* lock);

dart_ret_t dart_team_lock_free(dart_team_t teamid,
			       dart_lock_t* lock);

dart_ret_t dart_lock_acquire(dart_lock_t lock);

dart_ret_t dart_lock_try_acquire(dart_lock_t lock, int32_t *result);

dart_ret_t dart_lock_release(dart_lock_t lock);


#endif /* DART_SYNC_H_INCLUDED */
