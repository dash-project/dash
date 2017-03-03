#ifndef DASH_DART_BASE_MUTEX__H_
#define DASH_DART_BASE_MUTEX__H_

#include <dash/dart/if/dart_types.h>

// TODO: temporary until bug-118-threadsafety has been merged
#define DART_ENABLE_THREADING
#define DART_THREADING_PTHREADS

#if defined(DART_ENABLE_THREADING) && !defined(DART_THREADING_PTHREADS)
#error "Thread support has been enabled but DART_THREADING_PTHREADS is not defined!"
#endif

#if !defined(DART_ENABLE_THREADING) && defined(DART_THREADING_PTHREADS)
#undef DART_THREADING_PTHREADS
#endif

#ifdef DART_THREADING_PTHREADS
#include <pthread.h>
#endif

typedef struct dart_mutex {
#ifdef DART_THREADING_PTHREADS
pthread_mutex_t mutex;
#endif
} dart_mutex_t;

#ifdef DART_THREADING_PTHREADS
#define DART_MUTEX_INITIALIZER { PTHREAD_MUTEX_INITIALIZER }
#else
#define DART_MUTEX_INITIALIZER { 0 }
#endif

static inline
dart_ret_t
dart_mutex_init(dart_mutex_t *mutex)
{
#ifdef DART_THREADING_PTHREADS
  pthread_mutex_init(&mutex->mutex, NULL);
  return DART_OK;
#else
  return DART_ERR_INVAL;
#endif
}

static inline
dart_ret_t
dart_mutex_lock(dart_mutex_t *mutex)
{
#ifdef DART_THREADING_PTHREADS
  if (pthread_mutex_lock(&mutex->mutex) == 0){
    return DART_OK;
  } else {
    DART_LOG_ERROR("Failed to lock mutex!");
    return DART_ERR_OTHER;
  }

#else
  DART_LOG_ERROR("Failed to lock mutex!");
  return DART_ERR_INVAL;
#endif
}

static inline
dart_ret_t
dart_mutex_unlock(dart_mutex_t *mutex)
{
#ifdef DART_THREADING_PTHREADS
  if (pthread_mutex_unlock(&mutex->mutex) == 0) {
    return DART_OK;
  }else {
    DART_LOG_ERROR("Failed to unlock mutex!");
    return DART_ERR_OTHER;
  }
#else
  DART_LOG_ERROR("Failed to unlock mutex!");
  return DART_ERR_INVAL;
#endif
}

static inline
dart_ret_t
dart_mutex_trylock(dart_mutex_t *mutex)
{
#ifdef DART_THREADING_PTHREADS
  int ret = pthread_mutex_trylock(&mutex->mutex);
  return (ret == 0) ? DART_OK : DART_PENDING;
#else
  return DART_OK;
#endif
}


static inline
dart_ret_t
dart_mutex_destroy(dart_mutex_t *mutex)
{
#ifdef DART_THREADING_PTHREADS
  pthread_mutex_destroy(&mutex->mutex);
  return DART_OK;
#else
  return DART_ERR_INVAL;
#endif
}

#endif /* DASH_DART_BASE_MUTEX__H_ */
