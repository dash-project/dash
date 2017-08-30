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

#define DASH__DECLTYPE_AUTO_RETURN(...) \
  -> decltype(__VA_ARGS__)              \
  { return (__VA_ARGS__); }             \
  /**/

#define DASH__DECLTYPE_AUTO_RETURN_NOEXCEPT(...)          \
  noexcept(noexcept(decltype(__VA_ARGS__)(__VA_ARGS__)))  \
  -> decltype(__VA_ARGS__)                                \
  { return (__VA_ARGS__); }                               \
  /**/

#define DASH__DECLTYPE_NOEXCEPT(...)                      \
  noexcept(noexcept(decltype(__VA_ARGS__)(__VA_ARGS__)))  \
  -> decltype(__VA_ARGS__)                                \
  /**/

#define DASH__NORETURN __attribute__(( noreturn ))

#endif // DASH__INTERNAL__MACRO_H_
