/**
 * \file dart/base/string.h
 *
 */
#ifndef DART__BASE__STRING_H__
#define DART__BASE__STRING_H__

/**
 * Count occurrences of \c needle in string \c haystack.
 */
static inline int dart__strcnt(
  const char * haystack, char needle)
{
  int cnt = 0;
  for(; *haystack; haystack++) {
    if (*haystack == needle) { cnt++; }
  }
  return cnt;
}

#endif /* DART__BASE__STRING_H__ */
