/*
 * \file dart_util.h
 *
 * utility macros for the DART interface
 */

#ifndef DART_UTIL_H_
#define DART_UTIL_H_

/** \cond DART_HIDDEN_SYMBOLS */
#define DART_INTERFACE_ON
/** \endcond */

/**
 * Mark a function as not throwing C++ exceptions.
 */
#ifdef __cplusplus
#define DART_NOTHROW __attribute__((nothrow))
#else
#define DART_NOTHROW
#endif

/**
 * Mark a function (or variable) as possibly unused
 * (to suppress compiler warnings).
 */
#define DART_MAYBE_UNUSED __attribute__((unused))

/**
 * Mark a (public) function as inline (and potentially unused).
 */
#define DART_INLINE static inline DART_MAYBE_UNUSED

/**
 * Mark a function as not returning to the caller.
 */
#define DART_NORETURN __attribute__((noreturn))

/** \cond DART_HIDDEN_SYMBOLS */
#define DART_INTERFACE_OFF
/** \endcond */

#endif /* DART_UTIL_H_ */
