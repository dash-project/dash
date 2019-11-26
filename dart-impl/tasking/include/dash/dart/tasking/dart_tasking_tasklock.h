#ifndef DART__TASKING__TASKLOCK_H__
#define DART__TASKING__TASKLOCK_H__

//#define USE_DART_MUTEX

// Set this to disable the use of atomic_flag_test_and_set
#define USE_CMP_SWAP

#ifdef USE_DART_MUTEX

#include <dash/dart/base/mutex.h>

typedef dart_mutex_t dart_tasklock_t;

#define TASKLOCK_INITIALIZER DART_MUTEX_INITIALIZER

#define TASKLOCK_INIT(__task) do { \
 static dart_mutex_t tmp = DART_MUTEX_INITIALIZER; \
 (__task)->lock = tmp; \
} while (0)

#define LOCK_TASK(__task) dart__base__mutex_lock(&(__task)->lock)

#define UNLOCK_TASK(__task) dart__base__mutex_unlock(&(__task)->lock)

# elif defined(__STDC_NO_ATOMICS__) && defined (USE_CMP_SWAP)

#include <dash/dart/base/atomic.h>

typedef int32_t dart_tasklock_t;
#define TASKLOCK_INITIALIZER ((int32_t)0)

#define TASKLOCK_INIT(__task) do {  \
  __task->lock = TASKLOCK_INITIALIZER;\
} while (0)

#define LOCK_TASK(__task) do {\
  int cnt = 0; \
  while ((__task)->lock || !DART_COMPARE_AND_SWAP32(&(__task)->lock, 0, 1)) \
  { if (++cnt == 1000) { sched_yield(); cnt = 0; } } \
} while(0)

#define UNLOCK_TASK(__task) do {                          \
  dart_tasklock_t lck = DART_FETCH_AND_DEC32(&(__task)->lock); \
  dart__unused(lck);                                      \
  DART_ASSERT(lck == 1);                                  \
} while(0)

#elif defined(USE_CMP_SWAP)

typedef int32_t dart_tasklock_t;
#define TASKLOCK_INITIALIZER ((int32_t)0)

#define TASKLOCK_INIT(__task) do {  \
  __task->lock = TASKLOCK_INITIALIZER;\
} while (0)

#define LOCK_TASK(__task) do {\
  int cnt = 0; \
  dart_tasklock_t tmp = 0; \
  while ((__task)->lock || !atomic_compare_exchange_weak_explicit(&(__task)->lock, &tmp, 1, memory_order_acquire, memory_order_relaxed)) \
  { if (++cnt == 1000) { sched_yield(); cnt = 0; } } \
} while(0)

#define UNLOCK_TASK(__task) do {                          \
  atomic_store_explicit(&(__task)->lock, 0, memory_order_release); \
} while(0)


#else // defined(__STDC_NO_ATOMICS__)

#include <stdatomic.h>

typedef atomic_flag dart_tasklock_t;

#define TASKLOCK_INITIALIZER ATOMIC_FLAG_INIT

#define TASKLOCK_INIT(__task) do {    \
  atomic_flag_clear(&(__task)->lock); \
} while (0)

#define LOCK_TASK(__task) do {                         \
  int cnt = 0; \
  while (atomic_flag_test_and_set(&(__task)->lock)) \
  { if (++cnt == 1000) { sched_yield(); cnt = 0; } } \
} while(0)

#define UNLOCK_TASK(__task) do {      \
  atomic_flag_clear(&(__task)->lock); \
} while(0)

#endif // USE_DART_MUTEX

#endif // DART__TASKING__TASKLOCK_H__
