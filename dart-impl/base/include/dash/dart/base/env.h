/**
 * \file dash/dart/base/env.h
 *
 * Access to environment variables.
 */

#ifndef DASH_DART_BASE_ENV_H_
#define DASH_DART_BASE_ENV_H_

#include <unistd.h>
#include <stdbool.h>
#include <dash/dart/base/macro.h>

struct dart_env_str2int{
  const char *envstr;
  int         value;
};

/**
 * Parse a integral value from a set of options, e.g., from an enum.
 * The array values should be null-terminated, i.e., the last entry should
 * be {NULL, 0}.
 */
int
dart__base__env__str2int(
  const char                    * env,
  const struct dart_env_str2int * values,
  int                             fallback) DART_INTERNAL;

/**
 * Returns the number provided in the environment variable or -1
 * if the environment variable is not set or does not represent a number.
 */
int
dart__base__env__number(const char *env, int fallback) DART_INTERNAL;

/**
 * Parse a size from the provided environment variable.
 * The size value can be postfixed by 'K', 'M', 'G' for kilo-, mega-, and
 * gigabyte as well as 'B' for byte.
 *
 * \return The parsed value or -1 on error.
 */
ssize_t dart__base__env__size(const char *env, ssize_t fallback) DART_INTERNAL;


/**
 * Returns a Boolean value parsed from the environment variable.
 * Possible values are '0'/'1', 'True'/'False', 'Yes'/'No'
 * (both lower- and upper-case).
 *
 * \return The parsed value (false on error).
 */
bool dart__base__env__bool(const char *env, bool fallback) DART_INTERNAL;

/**
 * Returns the string value of the environment variable or NULL if not set.
 */
const char* dart__base__env__string(const char *env) DART_INTERNAL;

#endif /* DASH_DART_BASE_ENV_H_ */
