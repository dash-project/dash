#ifndef DASH_DART_BASE_MUTEX__H_
#define DASH_DART_BASE_MUTEX__H_

#include <dash/dart/if/dart_types.h>

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
#else 
// required since C99 does not allow empty structs
// TODO: this could be used for correctness checking
char __dummy;
#endif
} dart_mutex_t;

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
  pthread_mutex_lock(&mutex->mutex);
  return DART_OK;
#else
  return DART_ERR_INVAL;
#endif
}

static inline
dart_ret_t
dart_mutex_unlock(dart_mutex_t *mutex)
{
#ifdef DART_THREADING_PTHREADS
  pthread_mutex_unlock(&mutex->mutex);
  return DART_OK;
#else
  return DART_ERR_INVAL;
#endif
}

static inline
dart_ret_t
dart_mutex_trylock(dart_mutex_t *mutex)
{
#ifdef DART_THREADING_PTHREADS
  pthread_mutex_trylock(&mutex->mutex);
  return DART_OK;
#else
  return DART_ERR_INVAL;
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
