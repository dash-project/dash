#ifndef DART_SYNCHRONIZATION_H_INCLUDED
#define DART_SYNCHRONIZATION_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

/** \cond DART_HIDDEN_SYMBOLS */
#define DART_INTERFACE_ON
/** \endcond */

typedef struct dart_lock_struct *dart_lock_t;


dart_ret_t dart_team_lock_init(dart_team_t teamid,
			       dart_lock_t* lock);

dart_ret_t dart_team_lock_free(dart_team_t teamid,
			       dart_lock_t* lock);

/* blocking call */
dart_ret_t dart_lock_acquire(dart_lock_t lock);

dart_ret_t dart_lock_try_acquire(dart_lock_t lock, int32_t *result);

dart_ret_t dart_lock_release(dart_lock_t lock);


/** \cond DART_HIDDEN_SYMBOLS */
#define DART_INTERFACE_OFF
/** \endcond */

#ifdef __cplusplus
}
#endif

#endif /* DART_SYNCHRONIZATION_H_INCLUDED */

