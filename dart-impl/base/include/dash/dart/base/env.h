/**
 * \file dash/dart/base/env.h
 *
 * Access to environment variables.
 */

#ifndef DASH_DART_BASE_ENV_H_
#define DASH_DART_BASE_ENV_H_

#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
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
  int                             fallback);

/**
 * Returns the integral number provided in the environment variable or \c fallback
 * if the environment variable is not set or does not represent a number.
 */
int
dart__base__env__number(const char *env, int fallback);


/**
 * Returns the floating point number provided in the environment variable or
 * \c fallback if the environment variable is not set or does not represent a
 * number.
 */
float
dart__base__env__float(const char *env, float fallback);

/**
 * Parse a size from the provided environment variable.
 * The size value can be postfixed by 'K', 'M', 'G' for kilo-, mega-, and
 * gigabyte as well as 'B' for byte.
 *
 * \return The parsed value or \c fallback on error.
 */
ssize_t dart__base__env__size(const char *env, ssize_t fallback);

/**
 * Parse a time in microseconds from the provided environment variable.
 * The time value can be postfixed by 'u'/'us' or 'm'/'ms for micro- and
 * milli-seconds as well as 's' for seconds.
 *
 * \return The parsed value or \c fallback on error.
 */
uint64_t dart__base__env__us(const char *env, uint64_t fallback);

/**
 * Returns a Boolean value parsed from the environment variable.
 * Possible values are '0'/'1', 'True'/'False', 'Yes'/'No'
 * (both lower- and upper-case).
 *
 * \return The parsed value (false on error).
 */
bool dart__base__env__bool(const char *env, bool fallback);

/**
 * Returns the string value of the environment variable or NULL if not set.
 */
const char* dart__base__env__string(const char *env);

#endif /* DASH_DART_BASE_ENV_H_ */
