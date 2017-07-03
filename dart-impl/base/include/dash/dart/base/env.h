/**
 * \file dash/dart/base/env.h
 *
 * Access to environment variables.
 */

#ifndef DASH_DART_BASE_ENV_H_
#define DASH_DART_BASE_ENV_H_

#include <dash/dart/base/logging.h>

/**
 * Returns the log level set in DART_LOG_LEVEL, defaults to DART_LOGLEVEL_TRACE
 * if the environment variable is not set.
 */
enum dart__base__logging_loglevel
dart__base__env__log_level();

/**
 * Returns the number of threads set in DART_NUM_THREADS or -1
 * if the environment variable is not set.
 */
int 
dart__base__env__num_threads();


#endif /* DASH_DART_BASE_ENV_H_ */
