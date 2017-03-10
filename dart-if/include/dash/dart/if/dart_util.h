/*
 * \file dart_util.h
 * 
 * utility macros for the DART interface
 */

#ifndef DART_UTIL_H_
#define DART_UTIL_H_


#ifdef __cplusplus
#define DART_NOTHROW __attribute__((nothrow))
#else 
#define DART_NOTHROW
#endif

#define DART_MAYBE_UNUSED __attribute__((unused))

#define DART_INLINE static inline DART_MAYBE_UNUSED

#endif /* DART_UTIL_H_ */
