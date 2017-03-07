#ifndef DART__SYNCHRONIZATION_H
#define DART__SYNCHRONIZATION_H

/* DART v4.0 */

#ifdef __cplusplus
extern "C" {
#endif

#define DART_INTERFACE_ON

/**
 * \file dart_synchronization.h
 *
 * ### DART process synchronization
 *
 */

typedef struct dart_lock_struct *dart_lock_t;


dart_ret_t dart_team_lock_init(dart_team_t teamid,
			       dart_lock_t* lock);

dart_ret_t dart_team_lock_free(dart_team_t teamid,
			       dart_lock_t* lock);

/* blocking call */
dart_ret_t dart_lock_acquire(dart_lock_t lock);

dart_ret_t dart_lock_try_acquire(dart_lock_t lock, int32_t *result);

dart_ret_t dart_lock_release(dart_lock_t lock);



#define DART_INTERFACE_OFF

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DART__SYNCHRONIZATION_H */

