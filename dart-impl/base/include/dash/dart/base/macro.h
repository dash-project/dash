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

#endif /* DART__BASE__MACRO_H_ */
