/*
 * dart_locks.h
 *
 *  Created on: Apr 9, 2013
 *      Author: maierm
 */

#ifndef DART_LOCKS_H_
#define DART_LOCKS_H_

/*
 --- DART pairwise synchronization ---
 */

/* DART locks: See redmine issue refs #6 */

typedef struct dart_opaque_lock_t* dart_lock;

// creates lock in local-shared address space -> do we need this -> how to pass locks between processes
// int dart_lock_init(dart_lock* lock);

// team_id may be DART_TEAM_ALL. Create lock at team member 0?
int dart_lock_team_init(int team_id, dart_lock* lock);

// lock becomes DART_LOCK_NULL. may be called by any team member that initialized the lock?!
int dart_lock_free(dart_lock* lock);

// blocking call
int dart_lock_acquire(dart_lock lock);

// returns DART_LOCK_ACQUIRE_SUCCESS, DART_LOCK_ACQUIRE_FAILURE, or something like that
int dart_lock_try_acquire(dart_lock lock);
int dart_lock_release(dart_lock lock);

#endif /* DART_LOCKS_H_ */
