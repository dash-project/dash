#ifndef DART__BASE__INTERNAL__COMPILER_TWEAKS_H__
#define DART__BASE__INTERNAL__COMPILER_TWEAKS_H__

#define PUSH__WARN_IGNORE__
#ifdef __GNUC__
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-qual"
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcast-qual"
#endif
#endif

#define POP__WARN_IGNORE__
#ifdef __GNUC__
#ifdef __clang__
#pragma clang diagnostic pop
#else
#pragma GCC diagnostic pop
#endif
#endif

#endif
