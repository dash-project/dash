/*
 * \file atomic.h
 *
 * Provides atomic operations on basic integer data types.
 * Feel free to add operations you may require.
 *
 * See the URL below for details on the operations:
 * https://gcc.gnu.org/onlinedocs/gcc/_005f_005fsync-Builtins.html#g_t_005f_005fsync-Builtins
 *
 * See also the file kmp_os.h of the Clang OpenMP library, which served as inspiration.
 *
 * TODO: Check that all compilers support these intrinsics, esp. Cray?
 */

#ifndef DASH_DART_BASE_ATOMIC_H_
#define DASH_DART_BASE_ATOMIC_H_


#if DART_HAVE_SYNC_BUILTINS

/**
 * Atomic operations using the __sync* builtin functions.
 *
 * All macros are specialized for 64, 32, 16, and 8 bit wide integers
 * as well as for pointer types.
 *
 * See https://gcc.gnu.org/onlinedocs/gcc/_005f_005fsync-Builtins.html#g_t_005f_005fsync-Builtins
 * for details.
 *
 * Fall-back options are provided as unsafe options in case the built-in
 * functions are not available.
 */


#define DART_FETCH64(ptr) \
          DART_FETCH_AND_ADD64(ptr, 0)
#define DART_FETCH32(ptr) \
          DART_FETCH_AND_ADD32(ptr, 0)
#define DART_FETCH16(ptr) \
          DART_FETCH_AND_ADD16(ptr, 0)
#define DART_FETCH8(ptr)  \
          DART_FETCH_AND_ADD8(ptr, 0)
#define DART_FETCHPTR(ptr) \
          DART_FETCH_AND_ADDPTR(ptr, 0)

#define DART_FETCH_AND_ADD64(ptr, val) \
          __sync_fetch_and_add((int64_t *)(ptr), (val))
#define DART_FETCH_AND_ADD32(ptr, val) \
          __sync_fetch_and_add((int32_t *)(ptr), (val))
#define DART_FETCH_AND_ADD16(ptr, val) \
          __sync_fetch_and_add((int16_t *)(ptr), (val))
#define DART_FETCH_AND_ADD8(ptr, val)  \
          __sync_fetch_and_add((int8_t *)(ptr),  (val))

#define DART_FETCH_AND_SUB64(ptr, val) \
          __sync_fetch_and_sub((int64_t *)(ptr), (val))
#define DART_FETCH_AND_SUB32(ptr, val) \
          __sync_fetch_and_sub((int32_t *)(ptr), (val))
#define DART_FETCH_AND_SUB16(ptr, val) \
          __sync_fetch_and_sub((int16_t *)(ptr), (val))
#define DART_FETCH_AND_SUB8(ptr, val)  \
          __sync_fetch_and_sub((int8_t *)(ptr),  (val))

#define DART_FETCH_AND_INC64(ptr)  \
          __sync_fetch_and_add((int64_t *)(ptr), 1LL)
#define DART_FETCH_AND_INC32(ptr) \
          __sync_fetch_and_add((int32_t *)(ptr), 1)
#define DART_FETCH_AND_INC16(ptr)  \
          __sync_fetch_and_add((int16_t *)(ptr), 1)
#define DART_FETCH_AND_INC8(ptr)   \
          __sync_fetch_and_add((int8_t  *)(ptr), 1)
#define DART_FETCH_AND_INCPTR(ptr) \
          __sync_fetch_and_add((void   **)(ptr), sizeof(**(ptr)))

#define DART_FETCH_AND_DEC64(ptr)  \
          __sync_fetch_and_sub((int64_t *)(ptr), 1LL)
#define DART_FETCH_AND_DEC32(ptr)  \
          __sync_fetch_and_sub((int32_t *)(ptr), 1)
#define DART_FETCH_AND_DEC16(ptr)  \
          __sync_fetch_and_sub((int16_t *)(ptr), 1)
#define DART_FETCH_AND_DEC8(ptr)   \
          __sync_fetch_and_sub((int8_t  *)(ptr), 1)
#define DART_FETCH_AND_DECPTR(ptr) \
          __sync_fetch_and_sub((void   **)(ptr), sizeof(**(ptr)))



#define DART_ADD_AND_FETCH64(ptr, val)   \
          __sync_add_and_fetch((int64_t *)(ptr), (val))
#define DART_ADD_AND_FETCH32(ptr, val)   \
          __sync_add_and_fetch((int32_t *)(ptr), (val))
#define DART_ADD_AND_FETCH16(ptr, val)   \
          __sync_add_and_fetch((int16_t *)(ptr), (val))
#define DART_ADD_AND_FETCH8(ptr, val)   \
          __sync_add_and_fetch((int8_t  *)(ptr), (val))
#define DART_ADD_AND_FETCHPTR(ptr, cnt) \
          __sync_fetch_and_sub((void   **)(ptr), (cnt) * sizeof(**(ptr)))

#define DART_SUB_AND_FETCH64(ptr, val)   \
          __sync_sub_and_fetch((int64_t *)(ptr), (val))
#define DART_SUB_AND_FETCH32(ptr, val)   \
          __sync_sub_and_fetch((int32_t *)(ptr), (val))
#define DART_SUB_AND_FETCH16(ptr, val)   \
          __sync_sub_and_fetch((int16_t *)(ptr), (val))
#define DART_SUB_AND_FETCH8(ptr, val)   \
          __sync_sub_and_fetch((int8_t  *)(ptr), (val))
#define DART_SUB_AND_FETCHPTR(ptr, cnt) \
          __sync_sub_and_fetch((void   **)(ptr), (cnt) * sizeof(**(ptr)))

#define DART_INC_AND_FETCH64(ptr)   \
          __sync_add_and_fetch((int64_t *)(ptr), 1LL)
#define DART_INC_AND_FETCH32(ptr)   \
          __sync_add_and_fetch((int32_t *)(ptr), 1)
#define DART_INC_AND_FETCH16(ptr)   \
          __sync_add_and_fetch((int16_t *)(ptr), 1)
#define DART_INC_AND_FETCH8(ptr)    \
          __sync_add_and_fetch((int8_t  *)(ptr), 1)
#define DART_INC_AND_FETCHPTR(ptr)  \
          __sync_add_and_fetch((void   **)(ptr), sizeof(**(ptr)))

#define DART_DEC_AND_FETCH64(ptr)    \
          __sync_sub_and_fetch((int64_t *)(ptr), 1LL)
#define DART_DEC_AND_FETCH32(ptr)    \
          __sync_sub_and_fetch((int32_t *)(ptr), 1)
#define DART_DEC_AND_FETCH16(ptr)    \
          __sync_sub_and_fetch((int16_t *)(ptr), 1)
#define DART_DEC_AND_FETCH8(ptr)     \
          __sync_sub_and_fetch((int8_t  *)(ptr), 1)
#define DART_DEC_AND_FETCHPTR(ptr)   \
          __sync_sub_and_fetch((void   **)(ptr), sizeof(**(ptr)))


#define DART_COMPARE_AND_SWAP64(ptr, oldval, newval)       \
          __sync_val_compare_and_swap((int64_t *)(ptr),    \
                                      (int64_t  )(oldval), \
                                      (int64_t  )(newval))
#define DART_COMPARE_AND_SWAP32(ptr, oldval, newval)       \
          __sync_val_compare_and_swap((int32_t *)(ptr),    \
                                      (int32_t  )(oldval), \
                                      (int32_t  )(newval))
#define DART_COMPARE_AND_SWAP16(ptr, oldval, newval)       \
          __sync_val_compare_and_swap((int16_t *)(ptr),    \
                                      (int16_t  )(oldval), \
                                      (int16_t  )(newval))
#define DART_COMPARE_AND_SWAP8(ptr, oldval, newval)        \
          __sync_val_compare_and_swap((int8_t  *)(ptr),    \
                                      (int8_t   )(oldval), \
                                      (int8_t   )(newval))
#define DART_COMPARE_AND_SWAPPTR(ptr, oldval, newval)      \
          __sync_val_compare_and_swap((void   **)(ptr),    \
                                      (void    *)(oldval), \
                                      (void    *)(newval))

#else

#define DART_MAYBE_UNUSED __attribute__((unused))

/**
 * Fall-back version in case __sync* functions are not available.
 *
 * These surrogates are NOT THREADSAFE!
 */

static inline int64_t
DART_MAYBE_UNUSED
__fetch_and_add64(int64_t *ptr, int64_t val) {
  int64_t res = *ptr;
  *ptr += val;
  return res;
}

static inline int32_t
DART_MAYBE_UNUSED
__fetch_and_add32(int32_t *ptr, int32_t val) {
  int32_t res = *ptr;
  *ptr += val;
  return res;
}

static inline int16_t
DART_MAYBE_UNUSED
__fetch_and_add16(int16_t *ptr, int16_t val) {
  int16_t res = *ptr;
  *ptr += val;
  return res;
}

static inline int8_t
DART_MAYBE_UNUSED
__fetch_and_add8(int8_t *ptr, int8_t val) {
  int8_t res = *ptr;
  *ptr += val;
  return res;
}

static inline void *
DART_MAYBE_UNUSED
__fetch_and_addptr(char **ptr, int64_t val) {
  char * res = *ptr;
  *ptr += val;
  return res;
}



#define DART_FETCH_AND_ADD64(ptr, val) \
          __fetch_and_add64((ptr), (val))
#define DART_FETCH_AND_ADD32(ptr, val) \
          __fetch_and_add32((ptr), (val))
#define DART_FETCH_AND_ADD16(ptr, val) \
          __fetch_and_add16((ptr), (val))
#define DART_FETCH_AND_ADD8(ptr, val) \
          __fetch_and_add8((ptr), (val))

#define DART_FETCH_AND_SUB64(ptr, val) \
          __fetch_and_add64((ptr), (-1) * (val))
#define DART_FETCH_AND_SUB32(ptr, val) \
          __fetch_and_add32((ptr), (-1) * (val))
#define DART_FETCH_AND_SUB16(ptr, val) \
          __fetch_and_add16((ptr), (-1) * (val))
#define DART_FETCH_AND_SUB8(ptr, val) \
          __fetch_and_add8((ptr),  (-1) * (val))


#define DART_FETCH_AND_INC64(ptr)  \
          ((*(int64_t *)ptr)++)
#define DART_FETCH_AND_INC32(ptr) \
          ((*(int64_t *)ptr)++)
#define DART_FETCH_AND_INC16(ptr)  \
          ((*(int64_t *)ptr)++)
#define DART_FETCH_AND_INC8(ptr)   \
          ((*(int64_t *)ptr)++)
#define DART_FETCH_AND_INCPTR(ptr) \
          __fetch_and_addptr((void **)(ptr), sizeof(**(ptr)))


#define DART_FETCH_AND_DEC64(ptr)  \
          ((*(int64_t *)ptr)--)
#define DART_FETCH_AND_DEC32(ptr)  \
          ((*(int32_t *)ptr)--)
#define DART_FETCH_AND_DEC16(ptr)  \
          ((*(int16_t *)ptr)--)
#define DART_FETCH_AND_DEC8(ptr)   \
          ((*(int8_t  *)ptr)--)
#define DART_FETCH_AND_DECPTR(ptr) \
          __fetch_and_addptr((char **)(ptr), (-1) * sizeof(**(ptr)))



#define DART_ADD_AND_FETCH64(ptr, val)   \
          (*(int64_t *)ptr += (val))
#define DART_ADD_AND_FETCH32(ptr, val)   \
          (*(int32_t *)ptr += (val))
#define DART_ADD_AND_FETCH16(ptr, val)   \
          (*(int16_t *)ptr += (val))
#define DART_ADD_AND_FETCH8(ptr, val)   \
          (*(int8_t *)ptr += (val))
#define DART_ADD_AND_FETCHPTR(ptr, cnt) \
          (void*)(*(char **)ptr += (cnt) * sizeof(**(ptr)))


#define DART_SUB_AND_FETCH64(ptr, val)   \
          (*(int64_t *)(ptr) -= (val))
#define DART_SUB_AND_FETCH32(ptr, val)   \
          (*(int32_t *)(ptr) -= (val))
#define DART_SUB_AND_FETCH16(ptr, val)   \
          (*(int16_t *)(ptr) -= (val))
#define DART_SUB_AND_FETCH8(ptr, val)   \
          (*(int8_t  *)(ptr) -= (val))
#define DART_SUB_AND_FETCHPTR(ptr, cnt) \
          (void*)(*(char **)(ptr) -= (cnt) * sizeof(**(ptr)))


#define DART_INC_AND_FETCH64(ptr)   \
          (++(*(int64_t *)(ptr)))
#define DART_INC_AND_FETCH32(ptr)   \
          (++(*(int32_t *)(ptr)))
#define DART_INC_AND_FETCH16(ptr)   \
          (++(*(int16_t *)(ptr)))
#define DART_INC_AND_FETCH8(ptr)    \
          (++(*(int8_t  *)(ptr)))
#define DART_INC_AND_FETCHPTR(ptr)  \
          (++(*(void   **)(ptr)))


#define DART_DEC_AND_FETCH64(ptr)    \
          (--(*(int64_t *)(ptr)))
#define DART_DEC_AND_FETCH32(ptr)    \
          (--(*(int32_t *)(ptr)))
#define DART_DEC_AND_FETCH16(ptr)    \
          (--(*(int16_t *)(ptr)))
#define DART_DEC_AND_FETCH8(ptr)     \
          (--(*(int8_t  *)(ptr)))
#define DART_DEC_AND_FETCHPTR(ptr)   \
          (--(*(void   **)(ptr)))


static inline int64_t
DART_MAYBE_UNUSED
__compare_and_swap64(int64_t *ptr, int64_t oldval, int64_t newval) {
  int64_t res = *ptr;
  if (*ptr == oldval) {
    *ptr = newval;
  }
  return res;
}

static inline int32_t
DART_MAYBE_UNUSED
__compare_and_swap32(int32_t *ptr, int32_t oldval, int32_t newval) {
  int32_t res = *ptr;
  if (*ptr == oldval) {
    *ptr = newval;
  }
  return res;
}

static inline int16_t
DART_MAYBE_UNUSED
__compare_and_swap16(int16_t *ptr, int16_t oldval, int16_t newval) {
  int16_t res = *ptr;
  if (*ptr == oldval) {
    *ptr = newval;
  }
  return res;
}

static inline int8_t
DART_MAYBE_UNUSED
__compare_and_swap8(int8_t *ptr, int8_t oldval, int8_t newval) {
  int8_t res = *ptr;
  if (*ptr == oldval) {
    *ptr = newval;
  }
  return res;
}

static inline void*
DART_MAYBE_UNUSED
__compare_and_swapptr(void **ptr, void *oldval, void *newval) {
  void *res = *ptr;
  if (*ptr == oldval) {
    *ptr = newval;
  }
  return res;
}


#define DART_COMPARE_AND_SWAP64(ptr, oldval, newval) \
          __compare_and_swap64((int64_t *)(ptr),     \
                               (int64_t  )(oldval),  \
                               (int64_t  )(newval))
#define DART_COMPARE_AND_SWAP32(ptr, oldval, newval) \
          __compare_and_swap32((int32_t *)(ptr),     \
                               (int32_t  )(oldval),  \
                               (int32_t  )(newval))
#define DART_COMPARE_AND_SWAP16(ptr, oldval, newval) \
          __compare_and_swap16((int16_t *)(ptr),     \
                               (int16_t  )(oldval),  \
                               (int16_t  )(newval))
#define DART_COMPARE_AND_SWAP8(ptr, oldval, newval)  \
          __compare_and_swap8((int8_t  *)(ptr),      \
                              (int8_t   )(oldval),   \
                              (int8_t   )(newval))
#define DART_COMPARE_AND_SWAPPTR(ptr, oldval, newval) \
          __compare_and_swapptr((void   **)(ptr),     \
                                (void    *)(oldval),  \
                                (void    *)(newval))


#endif /* DART_HAVE_SYNC_BUILTINS */
#endif /* DASH_DART_BASE_ATOMIC_H_ */
