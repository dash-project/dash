#include <stdlib.h>
#include <string.h>
#include <dash/dart/base/env.h>

#define DART_LOGLEVEL_ENVSTR   "DART_LOG_LEVEL"
#define DART_NUMTHREADS_ENVSTR "DART_NUM_THREADS"

/**
 * Returns the log level set in DART_LOG_LEVEL, defaults to DART_LOGLEVEL_TRACE
 * if the environment variable is not set.
 */
enum dart__base__logging_loglevel
dart__base__env__log_level()
{
  static enum dart__base__logging_loglevel level = DART_LOGLEVEL_TRACE;
  static int log_level_parsed = 0;
  if (!log_level_parsed) {
    log_level_parsed = 1;
    const char *envstr = getenv(DART_LOGLEVEL_ENVSTR);
    if (envstr) {
      if (strncmp(envstr, "ERROR", 5) == 0) {
        level = DART_LOGLEVEL_ERROR;
      } else if (strncmp(envstr, "WARN", 4) == 0) {
        level = DART_LOGLEVEL_WARN;
      } else if (strncmp(envstr, "INFO", 4) == 0) {
        level = DART_LOGLEVEL_INFO;
      } else if (strncmp(envstr, "DEBUG", 5) == 0) {
        level = DART_LOGLEVEL_DEBUG;
      } else if (strncmp(envstr, "TRACE", 5) == 0) {
        level = DART_LOGLEVEL_TRACE;
      }
    }
  }

  return level;
}

/**
 * Returns the number of threads set in DART_NUM_THREADS or -1
 * if the environment variable is not set.
 */
int
dart__base__env__num_threads()
{
  static int num_threads = -1;
  static int num_threads_parsed = 0;
  if (!num_threads_parsed) {
    num_threads_parsed = 1;
    const char *envstr = getenv(DART_NUMTHREADS_ENVSTR);
    if (envstr) {
      num_threads = atoi(envstr);
    }
  }
  return num_threads;
}
