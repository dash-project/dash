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


#define DART_FETCH_AND_ADD64(ptr, val)   __sync_fetch_and_add((int64_t *)(ptr), (val))
#define DART_FETCH_AND_ADD32(ptr, val)   __sync_fetch_and_add((int32_t *)(ptr), (val))
#define DART_FETCH_AND_SUB64(ptr, val)   __sync_fetch_and_sub((int64_t *)(ptr), (val))
#define DART_FETCH_AND_SUB32(ptr, val)   __sync_fetch_and_sub((int32_t *)(ptr), (val))

#define DART_FETCH_AND_INC64(ptr)        __sync_fetch_and_add((int64_t *)(ptr), 1LL)
#define DART_FETCH_AND_INC32(ptr)        __sync_fetch_and_add((int32_t *)(ptr), 1)
#define DART_FETCH_AND_INC16(ptr)        __sync_fetch_and_add((int16_t *)(ptr), 1)
#define DART_FETCH_AND_INC8(ptr)         __sync_fetch_and_add((int8_t  *)(ptr), 1)
#define DART_FETCH_AND_INCPTR(ptr)       __sync_fetch_and_add((void   **)(ptr), sizeof(**ptr))

#define DART_FETCH_AND_DEC64(ptr)        __sync_fetch_and_sub((int64_t *)(ptr), 1LL)
#define DART_FETCH_AND_DEC32(ptr)        __sync_fetch_and_sub((int32_t *)(ptr), 1)
#define DART_FETCH_AND_DEC16(ptr)        __sync_fetch_and_sub((int16_t *)(ptr), 1)
#define DART_FETCH_AND_DEC8(ptr)         __sync_fetch_and_sub((int8_t  *)(ptr), 1)
#define DART_FETCH_AND_DECPTR(ptr)       __sync_fetch_and_sub((void   **)(ptr), sizeof(**ptr))

#define DART_COMPARE_AND_SWAP64(ptr, oldval, newval)   __sync_val_compare_and_swap((int64_t *)(ptr), (int64_t)(oldval), (int64_t)(newval))
#define DART_COMPARE_AND_SWAP32(ptr, oldval, newval)   __sync_val_compare_and_swap((int32_t *)(ptr), (int32_t)(oldval), (int32_t)(newval))
#define DART_COMPARE_AND_SWAP16(ptr, oldval, newval)   __sync_val_compare_and_swap((int16_t *)(ptr), (int16_t)(oldval), (int16_t)(newval))
#define DART_COMPARE_AND_SWAP8(ptr, oldval, newval)    __sync_val_compare_and_swap((int8_t  *)(ptr), (int8_t )(oldval), (int8_t )(newval))
#define DART_COMPARE_AND_SWAPPTR(ptr, oldval, newval)  __sync_val_compare_and_swap((void   **)(ptr), (void  *)(oldval), (void  *)(newval))

#endif /* DASH_DART_BASE_ATOMIC_H_ */
