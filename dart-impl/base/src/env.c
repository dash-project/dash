#include <stdlib.h>
#include <string.h>
#include <dash/dart/base/env.h>

#define DART_LOGLEVEL_ENVSTR      "DART_LOG_LEVEL"

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
 * Returns the number provided in the environment variable or -1
 * if the environment variable is not set or does not represent a number.
 */
int
dart__base__env__number(const char *env)
{
  int result = -1;
  char *endptr;
  const char *envstr = getenv(env);
  if (envstr && *envstr != '\0') {
    result = strtol(envstr, &endptr, 10);
    if (*endptr != '\0') {
      // parsing failed
      result = -1;
    }
  }
  return result;
}

ssize_t dart__base__env__size(const char *env)
{
  size_t res = -1;
  const char *envstr = getenv(env);
  if (envstr != NULL) {
    char *endptr;
    if (sizeof(size_t) == sizeof(long long int)) {
      res = strtoll(envstr, &endptr, 10);
    } else {
      res = strtol(envstr, &endptr, 10);
    }
    DART_LOG_TRACE("%s: %s (%lu %s)", env, envstr, res, endptr);
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
  }
  DART_LOG_TRACE("%s: %s (%lu)", env, envstr, res);
  return res;
}
