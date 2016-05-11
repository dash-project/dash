/**
 * \file dart/base/string.c
 *
 */

#include <dash/dart/base/string.h>

int dart__base__strcnt(
  char * haystack,
  char   needle)
{
  int cnt = 0;
  for(; *haystack; haystack++) {
    if (*haystack == needle) { cnt++; }
  }
  return cnt;
}

int dart__base__strscommonprefix(
  char ** strings,
  int     num_strings,
  char ** prefix_out)
{
  int prefix_len = 0;

  return prefix_len;
}

int dart__base__strcommonprefix(
  char  * string_a,
  char  * string_b,
  char ** prefix_out)
{
  int prefix_len = 0;

  return prefix_len;
}



