/**
 * \file dash/dart/base/env.h
 *
 * Access to environment variables.
 */

#ifndef DASH_DART_BASE_ENV_H_
#define DASH_DART_BASE_ENV_H_

#include <dash/dart/base/macro.h>
#include <dash/dart/base/logging.h>

/**
 * Returns the log level set in DART_LOG_LEVEL, defaults to DART_LOGLEVEL_TRACE
 * if the environment variable is not set.
 */
enum dart__base__logging_loglevel
dart__base__env__log_level() DART_INTERNAL;

/**
 * Returns the number provided in the environment variable or -1
 * if the environment variable is not set or does not represent a number.
 */
int
dart__base__env__number(const char *env) DART_INTERNAL;


/**
 * Parse a size from the provided environment variable.
 * The size value can be postfixed by 'K', 'M', 'G' for kilo-, mega-, and
 * gigabyte as well as 'B' for byte.
 *
 * \return The parsed value or -1 on error.
 */
ssize_t dart__base__env__size(const char *env) DART_INTERNAL;


#endif /* DASH_DART_BASE_ENV_H_ */
