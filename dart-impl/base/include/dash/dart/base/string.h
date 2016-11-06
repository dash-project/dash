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
  const char  * haystack,
  char          needle);

int dart__base__strscommonprefix(
  const char ** strings,
  int           num_strings,
  char        * prefix_out);

int dart__base__strcommonprefix(
  const char  * string_a,
  const char  * string_b,
  char        * prefix_out);

int dart__base__strsunique(
  char       ** strings,
  int           num_strings);

/**
 * Alternative to `strncat' that restricts the length of the destination
 * string.
 *
 * Expects reference to string length and remaining capacity of `str_lhs`
 * before appending `str_rhs`.
 *
 * Values referenced by `str_lhs_len` and `str_lhs_remaining_cap` are
 * updated to string length and remaining capacity of `str_lhs` after
 * appending `str_rhs`.
 */
char * dart__base__strappend(
  /** Destination string. */
  char        * str_lhs,
  /** String to append. */
  const char  * str_rhs,
  /** Length of string str_lhs before appending str_rhs.
   *  Referenced value is updated to new length of str_lhs. */
  int         * str_lhs_len,
  /** Remaining capacity of `str_lhs` before appending `str_rhs`.
   *  Referenced value is updated to new remaining capacity of `str_lhs`
   *  after appending `str_rhs`. */
  int         * str_lhs_remaining_cap);

#endif /* DART__BASE__STRING_H__ */
