#include <stdlib.h>
#include <string.h>
#include <dash/dart/base/env.h>

#define DART_LOGLEVEL_ENVSTR      "DART_LOG_LEVEL"
#define DART_NUMTHREADS_ENVSTR    "DART_NUM_THREADS"
#define DART_TASKSTACKSIZE_ENVSTR "DART_TASK_STACKSIZE"

/**
 * Returns the log level set in DART_LOG_LEVEL, defaults to DART_LOGLEVEL_TRACE
 * if the environment variable is not set.
 */
enum dart__base__logging_loglevel
dart__base__env__log_level()
{
  static enum dart__base__logging_loglevel level = DART_LOGLEVEL_TRACE;
  static int parsed = 0;
  if (!parsed) {
    parsed = 1;
    const char *envstr = getenv(DART_LOGLEVEL_ENVSTR);
    if (envstr && *envstr != '\0') {
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
  static int parsed = 0;
  if (!parsed) {
    char *endptr;
    parsed = 1;
    const char *envstr = getenv(DART_NUMTHREADS_ENVSTR);
    if (envstr && *envstr != '\0') {
      num_threads = strtol(envstr, &endptr, 10);
      if (*endptr != '\0') {
        // parsing failed
        num_threads = -1;
      }
    }
  }
  return num_threads;
}

static
size_t parse_size(const char *envstr)
{
  size_t res = -1;
  char *endptr;
  if (sizeof(size_t) == sizeof(long long int)) {
    res = strtoll(envstr, &endptr, 10);
  } else {
    res = strtol(envstr, &endptr, 10);
  }
  if (*endptr != '\0') {
    // check for B, K, M, or G suffix
    switch(*endptr) {
    case 'G':
      res *= 1024; /* fall-through */
    case 'M':
      res *= 1024; /* fall-through */
    case 'K':
      res *= 1024; /* fall-through */
    case 'B':
      break;
    default:
      // error
      res = -1;
    }
  }
  return res;
}

ssize_t
dart__base__env__task_stacksize()
{
  static size_t stack_size = -1;
  static int parsed = 0;
  if (!parsed) {
    parsed = 1;
    const char *envstr = getenv(DART_TASKSTACKSIZE_ENVSTR);

    if (envstr && *envstr != '\0') {
      stack_size = parse_size(envstr);
    }
  }
  return stack_size;
}
