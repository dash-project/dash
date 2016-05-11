/**
 * \file dart/base/string.h
 *
 */
#ifndef DART__BASE__STRING_H__
#define DART__BASE__STRING_H__

/**
 * Count occurrences of \c needle in string \c haystack.
 */
int dart__base__strcnt(
  char  * haystack,
  char    needle);

int dart__base__strscommonprefix(
  char ** strings,
  int     num_strings,
  char ** prefix_out);

int dart__base__strcommonprefix(
  char  * string_a,
  char  * string_b,
  char ** prefix_out);

#endif /* DART__BASE__STRING_H__ */
