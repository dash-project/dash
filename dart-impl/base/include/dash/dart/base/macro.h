#ifndef DART__BASE__MACRO_H_
#define DART__BASE__MACRO_H_

/**
 * Macro parameter value string expansion.
 */
#define dart__tostr(s) #s
/**
 * Macro parameter string expansion.
 */
#define dart__toxstr(s) dart__tostr(s)
/**
 * Mark variable as intentionally or potentially unused to avoid compiler
 * warning from unused variable.
 */
#define dart__unused(x) (void)(x)


/**
 * Flag a boolean expression as highly likely to be true.
 * __builtin_expect is supported by all compilers supported by DART/DASH
 */
#define dart__likely(x)      __builtin_expect(!!(x), 1)

/**
 * Flag a boolean expression as highly likely to be false.
 * __builtin_expect is supported by all compilers supported by DART/DASH
 */
#define dart__unlikely(x)    __builtin_expect(!!(x), 0)

#if !defined(_CRAYC)
/**
 * Mark a variable or function internal, i.e., it is not accessed from outside
 * the shared object, neither directly nor through function pointer.
 * Note that symbols defined in DART base and called from DART MPI
 * should not be marked internal.
 */
#define DART_INTERNAL __attribute__((visibility("hidden")))
#else
/* the Cray Compiler does not seem to support this attribute */
#define DART_INTERNAL
#endif

/* Static Assertion Macro */
#if __STDC_VERSION__ >= 201112L
#define _STATIC_ASSERT(COND,MSG) _Static_assert(COND, MSG)
#else
#define _STATIC_ASSERT(COND,MSG) typedef char static_assertion[(!!(COND))*2-1]
#endif
#define DART_STATIC_ASSERT(x)           _STATIC_ASSERT(x,dart__toxstr(x))
#define DART_STATIC_ASSERT_MSG(x, msg)  _STATIC_ASSERT(x, msg)


#endif /* DART__BASE__MACRO_H_ */
