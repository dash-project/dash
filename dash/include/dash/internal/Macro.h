#ifndef DASH__INTERNAL__MACRO_H_
#define DASH__INTERNAL__MACRO_H_

/**
 * Macro parameter string expasion
 */
#define __xstr(s) __str(s)
/**
 * Macro parameter value string expasion
 */
#define __str(s) #s

/**
 * Workaround GCC versions that do not support the 
 * noinline attribute
 */
#ifndef NOINLINE_ATTRIBUTE
  #ifdef __ICC
    #define NOINLINE_ATTRIBUTE __attribute__(( noinline ))
  #else
    #define NOINLINE_ATTRIBUTE
  #endif // __ICC
#endif // NOINLINE_ATTRIBUTE

#endif // DASH__INTERNAL__MACRO_H_
