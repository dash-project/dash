#ifndef DART__BASE__INTERNAL__COMPILER_TWEAKS_H__
#define DART__BASE__INTERNAL__COMPILER_TWEAKS_H__

#define STRINGIFY(x) #x
#ifdef __GNUC__
#ifdef __clang__
#define PRAGMA__PUSH _Pragma(STRINGIFY(clang diagnostic push))
#else
#define PRAGMA__PUSH _Pragma(STRINGIFY(GCC diagnostic push))
#endif
#endif
#ifdef __GNUC__
#ifdef __clang__
#define PRAGMA__IGNORE _Pragma(STRINGIFY(clang diagnostic ignored "-Wcast-qual"))
#else
#define PRAGMA__IGNORE _Pragma(STRINGIFY(GCC diagnostic ignored "-Wcast-qual"))
#endif
#endif
#ifdef __GNUC__
#ifdef __clang__
#define PRAGMA__POP _Pragma(STRINGIFY(clang diagnostic pop))
#else
#define PRAGMA__POP _Pragma(STRINGIFY(GCC diagnostic pop))
#endif
#endif

#endif
