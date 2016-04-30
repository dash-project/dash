#ifndef DASH__INTERNAL__MACRO_H_
#define DASH__INTERNAL__MACRO_H_

/**
 * Macro parameter value string expansion.
 */
#define dash__tostr(s) #s
/**
 * Macro parameter string expansion.
 */
#define dash__toxstr(s) dash__tostr(s)
/**
 * Mark variable as intentionally or potentially unused to avoid compiler
 * warning from unused variable.
 */
#define dash__unused(x) (void)(x)

/**
 * Workaround for GCC versions that do not support the noinline attribute.
 */
#ifndef NOINLINE_ATTRIBUTE
  #ifdef __ICC
    #define NOINLINE_ATTRIBUTE __attribute__(( noinline ))
  #else
    #define NOINLINE_ATTRIBUTE
  #endif // __ICC
#endif // NOINLINE_ATTRIBUTE

#endif // DASH__INTERNAL__MACRO_H_
