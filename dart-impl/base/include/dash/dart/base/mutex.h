#ifndef DASH_DART_BASE_MUTEX__H_
#define DASH_DART_BASE_MUTEX__H_

#include <dash/dart/if/dart_types.h>


#ifdef HAVE_PTHREADS
#include <pthread.h>
#endif

typedef struct dart_mutex {
#ifdef HAVE_PTHREADS
pthread_mutex_t mutex;
#endif
} dart_mutex_t;

static inline
dart_ret_t
dart_mutex_init(dart_mutex_t *mutex)
{
#ifdef HAVE_PTHREADS
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
#ifdef HAVE_PTHREADS
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
#ifdef HAVE_PTHREADS
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
#ifdef HAVE_PTHREADS
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
#ifdef HAVE_PTHREADS
  pthread_mutex_destroy(&mutex->mutex);
  return DART_OK;
#else
  return DART_ERR_INVAL;
#endif
}

#endif /* DASH_DART_BASE_MUTEX__H_ */
