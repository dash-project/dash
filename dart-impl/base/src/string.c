/**
 * \file dart/base/string.c
 *
 */

#include <dash/dart/base/string.h>

#include <limits.h>
#include <string.h>
#include <memory.h>
#include <stdlib.h>


int dart__base__strcnt(
  const char * haystack,
  char         needle)
{
  int cnt = 0;
  for(; *haystack; haystack++) {
    if (*haystack == needle) { cnt++; }
  }
  return cnt;
}

int dart__base__strscommonprefix(
  const char ** strings,
  int           num_strings,
  char        * prefix_out)
{
  int prefix_len = 0;

  prefix_out[0] = '\0';

  if (num_strings == 0) {
    return prefix_len;
  }

  prefix_len = INT_MAX;
  strcpy(prefix_out, strings[0]);
  for (int i = 0; i < num_strings; i++) {
    char * prefix_i  = malloc((strlen(strings[i]) + 1) * sizeof(char));
    int prefix_i_len = dart__base__strcommonprefix(
                         prefix_out, strings[i], prefix_i);
    if (prefix_i_len < prefix_len) {
      prefix_len = prefix_i_len;
      strcpy(prefix_out, prefix_i);
    }
    free(prefix_i);
  }
  if (prefix_len == INT_MAX) {
    prefix_len = 0;
  }
  prefix_out[prefix_len] = '\0';

  return prefix_len;
}

int dart__base__strcommonprefix(
  const char * string_a,
  const char * string_b,
  char       * prefix_out)
{
  int prefix_len   = 0;
  int string_a_len = strlen(string_a);
  int string_b_len = strlen(string_b);

  if (prefix_out != NULL) {
    prefix_out[0] = '\0';
  }

  if (string_a_len == 0 || string_b_len == 0) {
    return prefix_len;
  }
  for (int i = 0; i < string_a_len; i++) {
    if (i >= string_b_len) {
      break;
    }
    if (string_a[i] != string_b[i]) {
      break;
    } else {
      if (prefix_out != NULL) {
        prefix_out[prefix_len] = string_a[i];
      }
      prefix_len++;
    }
  }
  if (prefix_out != NULL) {
    prefix_out[prefix_len] = '\0';
  }

  return prefix_len;
}

int dart__base__strsunique(
  char ** strings,
  int     num_strings)
{
  int last_unique = 0;
  if (num_strings >= 2) {
    for (int s = 1; s < num_strings; s++) {
      if (strcmp(strings[s], strings[last_unique]) != 0) {
        ++last_unique;
        if (s != last_unique) {
          strcpy(strings[last_unique], strings[s]);
        }
      }
    }
  }
  return last_unique + 1;
}

