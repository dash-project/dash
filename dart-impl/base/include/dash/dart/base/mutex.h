#ifndef DASH_DART_BASE_MUTEX__H_
#define DASH_DART_BASE_MUTEX__H_

#include <dash/dart/base/logging.h>
#include <dash/dart/base/config.h>
#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_util.h>
#include <stdint.h>
#include <sched.h>

#define DART_MUTEX_ATOMIC_LOCK


/* TODO: using CAS seems broken, check that! */
//#define DART_MUTEX_CAS

// enable to yield after this many unsuccesful attempts
//#define DART_MUTEX_SCHED_YIELD 1000

#if defined(DART_MUTEX_ATOMIC_LOCK) || defined(DART_MUTEX_CAS)
#include <stdatomic.h>
#endif // DART_MUTEX_ATOMIC_LOCK

#if defined(DART_ENABLE_THREADSUPPORT) && !defined(DART_HAVE_PTHREADS)
#error "Thread support has been enabled but PTHREADS support is not available!"
#endif

#if !defined(DART_ENABLE_THREADSUPPORT) && defined(DART_HAVE_PTHREADS)
#undef DART_HAVE_PTHREADS
#endif

#ifdef DART_MUTEX_ATOMIC_LOCK
#define DART_MUTEX_INITIALIZER { .flag = ATOMIC_FLAG_INIT }
#elif defined(DART_MUTEX_CAS)
#define DART_MUTEX_INITIALIZER { .lock = 0 }
#elif defined(DART_HAVE_PTHREADS)
#include <pthread.h>
#define DART_MUTEX_INITIALIZER { .mutex = PTHREAD_MUTEX_INITIALIZER }
#else
#define DART_MUTEX_INITIALIZER { 0 }
#endif

typedef struct dart_mutex {
#ifdef DART_MUTEX_ATOMIC_LOCK
atomic_flag     flag;
#elif defined(DART_MUTEX_CAS)
volatile uint32_t        lock;
#elif defined(DART_HAVE_PTHREADS)
pthread_mutex_t mutex;
#else
// required since C99 does not allow empty structs
// TODO: this could be used for correctness checking
char __dummy;
#endif
} dart_mutex_t;

DART_INLINE
dart_ret_t
dart__base__mutex_init(dart_mutex_t *mutex)
{
#ifdef DART_MUTEX_ATOMIC_LOCK
  atomic_flag_clear(&mutex->flag);
#elif defined(DART_MUTEX_CAS)
  mutex->lock = 0;
#elif defined(DART_HAVE_PTHREADS)
  // pthread_mutex_init always succeeds
  pthread_mutex_init(&mutex->mutex, NULL);
  DART_LOG_TRACE("%s: Initialized fast mutex %p", __func__, mutex);
#else
  static int single = 0;
  if (!single) {
    DART_LOG_WARN("%s: thread-support disabled", __FUNCTION__);
    single = 1;
  }
#endif
  return DART_OK;
}

DART_INLINE
dart_ret_t
dart__base__mutex_init_recursive(dart_mutex_t *mutex)
{
#ifdef DART_MUTEX_ATOMIC_LOCK
  atomic_flag_clear(&mutex->flag);
#elif defined(DART_MUTEX_CAS)
  mutex->lock = 0;
#elif defined(DART_HAVE_PTHREADS)
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  int ret = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE_NP);
  if (ret != 0) {
    DART_LOG_WARN("dart__base__mutex_init_recursive: Failed to initialize "
                  "recursive pthread mutex! "
                  "Falling back to standard mutex...");
    pthread_mutexattr_destroy(&attr);
    return dart__base__mutex_init(mutex);
  }
  pthread_mutex_init(&mutex->mutex, &attr);
  pthread_mutexattr_destroy(&attr);
  DART_LOG_TRACE("%s: Initialized recursive mutex %p", __func__, mutex);
#else
  static int single = 0;
  if (!single) {
    DART_LOG_WARN("%s: thread-support disabled", __FUNCTION__);
    single = 1;
  }
#endif
  return DART_OK;
}

DART_INLINE
dart_ret_t
dart__base__mutex_lock(dart_mutex_t *mutex)
{
#ifdef DART_MUTEX_ATOMIC_LOCK
  while (atomic_flag_test_and_set(&mutex->flag) != false) {}
#elif defined(DART_MUTEX_CAS)
#ifdef DART_MUTEX_SCHED_YIELD
  int cnt = 0;
#endif // DART_MUTEX_SCHED_YIELD
  uint32_t tmp = 0;
  while (mutex->lock || !atomic_compare_exchange_weak_explicit(&mutex->lock, &tmp, 1, memory_order_acquire, memory_order_relaxed))
  {
#ifdef DART_MUTEX_SCHED_YIELD
    if (++cnt == DART_MUTEX_SCHED_YIELD) {
      sched_yield();
      cnt = 0;
    }
#endif // DART_MUTEX_SCHED_YIELD
    tmp = 0;
  }
#elif defined(DART_HAVE_PTHREADS)
  int ret = pthread_mutex_lock(&mutex->mutex);
  if (ret != 0) {
    DART_LOG_TRACE("%s: Failed to lock mutex (%i)", __func__, ret);
    return DART_ERR_OTHER;
  }
#endif
  return DART_OK;
}

DART_INLINE
dart_ret_t
dart__base__mutex_unlock(dart_mutex_t *mutex)
{
#ifdef DART_MUTEX_ATOMIC_LOCK
  atomic_flag_clear(&mutex->flag);
#elif defined(DART_MUTEX_CAS)
  atomic_store_explicit(&mutex->lock, 0, memory_order_release);
#elif defined(DART_HAVE_PTHREADS)
  int ret = pthread_mutex_unlock(&mutex->mutex);
  if (ret != 0) {
    DART_LOG_TRACE("%s: Failed to unlock mutex (%i)", __func__, ret);
    return DART_ERR_OTHER;
  }
#endif
  return DART_OK;
}

DART_INLINE
dart_ret_t
dart__base__mutex_trylock(dart_mutex_t *mutex)
{
#ifdef DART_MUTEX_ATOMIC_LOCK
  return atomic_flag_test_and_set(&mutex->flag) ? DART_PENDING : DART_OK;
#elif defined(DART_MUTEX_CAS)
  uint32_t tmp = 0;
  return atomic_compare_exchange_strong_explicit(&mutex->lock, &tmp, 1, memory_order_acquire, memory_order_relaxed);
#elif defined(DART_HAVE_PTHREADS)
  int ret = pthread_mutex_trylock(&mutex->mutex);
  return (ret == 0) ? DART_OK : DART_PENDING;
#else
  return DART_OK;
#endif
}

DART_INLINE
dart_ret_t
dart__base__mutex_destroy(dart_mutex_t *mutex)
{
#ifdef DART_MUTEX_ATOMIC_LOCK
  atomic_flag_clear(&mutex->flag);
  return DART_OK;
#elif defined(DART_MUTEX_CAS)
  atomic_store_explicit(&mutex->lock, 0, memory_order_release);
#elif defined(DART_HAVE_PTHREADS)
  int ret = pthread_mutex_destroy(&mutex->mutex);
  if (ret != 0) {
    DART_LOG_TRACE("%s: Failed to destroy mutex (%i)", __func__, ret);
    return DART_ERR_OTHER;
  }
  return DART_OK;
#else
  return DART_OK;
#endif
}

#endif /* DASH_DART_BASE_MUTEX__H_ */
