/**
 * \file dart/base/string.h
 *
 */
#ifndef DART__BASE__STRING_H__
#define DART__BASE__STRING_H__

#include <dash/dart/if/dart_util.h>

/**
 * Count occurrences of \c needle in string \c haystack.
 */
DART_API
int dart__base__strcnt(
  const char  * haystack,
  char          needle);

DART_API
int dart__base__strscommonprefix(
  const char ** strings,
  int           num_strings,
  char        * prefix_out);

DART_API
int dart__base__strcommonprefix(
  const char  * string_a,
  const char  * string_b,
  char        * prefix_out);

DART_API
int dart__base__strsunique(
  char       ** strings,
  int           num_strings);

#endif /* DART__BASE__STRING_H__ */
