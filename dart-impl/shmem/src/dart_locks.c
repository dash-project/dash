
#include <pthread.h>
#include <dash/dart/if/dart_types.h>

#include <dash/dart/shmem/dart_locks.h>
#include <dash/dart/shmem/dart_teams_impl.h>
#include <dash/dart/shmem/shmem_barriers_if.h>
#include <dash/dart/shmem/shmem_logger.h>


/* collective call, all members of the team have to call 
   this function to initialize a lock */
dart_ret_t dart_team_lock_init(dart_team_t teamid, 
			       dart_lock_t* lock)
{
  int lockid;
  syncarea_t area;
  dart_team_unit_t myid;
  dart_team_myid(teamid, &myid);
  dart_team_unit_t root;
  root.id = 0;

  if( myid.id == 0 ) {
    area = shmem_getsyncarea();
    PTHREAD_SAFE_NORET(pthread_mutex_lock(&(area->barrier_lock)));

    for( lockid=0; lockid<MAXNUM_LOCKS; lockid++ ) {
      if( !(area->locks[lockid]).inuse ) {
	(area->locks[lockid]).inuse=1;
	(area->locks[lockid]).teamid=teamid;
	break;
      }
    }
    
    PTHREAD_SAFE_NORET(pthread_mutex_unlock(&(area->barrier_lock)));
  }
  dart_bcast(&lockid, sizeof(int), DART_TYPE_INT, root, teamid);

  if( lockid==MAXNUM_LOCKS ) 
    return DART_ERR_OTHER;
  
  (*lock) = &(shmem_getsyncarea()->locks[lockid]);
  
  return DART_OK;
}

/* collective call, all members of the team have to call 
   this function to free a lock */
dart_ret_t dart_team_lock_free(dart_team_t teamid,
			       dart_lock_t* lock)
{
  int lockid;
  syncarea_t area;
  dart_team_unit_t myid;
  dart_team_myid(teamid, &myid);
  
  if( myid.id == 0 ) {
    area = shmem_getsyncarea();
    PTHREAD_SAFE_NORET(pthread_mutex_lock(&(area->barrier_lock)));

    for( lockid=0; lockid<MAXNUM_LOCKS; lockid++ ) {
      if( (*lock)==&(area->locks[lockid]) ) {
	(area->locks[lockid]).inuse=0;
	break;
      }
    }
    
    PTHREAD_SAFE_NORET(pthread_mutex_unlock(&(area->barrier_lock)));
  }
  dart_barrier(teamid);
  
  return DART_OK;
}

dart_ret_t dart_lock_acquire(dart_lock_t lock)
{
  pthread_mutex_t *plock;
  plock = &(lock->mutex);
  //fprintf(stderr, "locking %p\n", plock);
  PTHREAD_SAFE(pthread_mutex_lock(plock));
  return DART_OK;
}

dart_ret_t dart_lock_release(dart_lock_t lock)
{
  pthread_mutex_t *plock;
  plock = &(lock->mutex);
  //fprintf(stderr, "unlocking %p ...\n", plock);
  PTHREAD_SAFE(pthread_mutex_unlock(plock));
  return DART_OK;
}


dart_ret_t dart_lock_try_acquire(dart_lock_t lock, 
				 int32_t *result)
{
  pthread_mutex_t *plock;
  plock = &(lock->mutex);
  
  int ret = pthread_mutex_trylock(plock);

  if( ret==0 ) {
    (*result)=1;
    return DART_OK;
  } else {
    (*result)=0;
    return DART_ERR_OTHER;
  }
}


