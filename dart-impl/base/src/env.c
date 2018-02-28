#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <dash/dart/base/env.h>
#include <dash/dart/base/logging.h>


/**
 * Returns the string value of the environment variable or NULL if not set.
 */
const char* dart__base__env__string(const char *env)
{
  return getenv(env);
}

int
dart__base__env__str2int(
  const char                    * env,
  const struct dart_env_str2int * values,
  int                             fallback)
{
  int res = fallback;
  if (values != NULL) {
    int i = 0;
    const char *envstr = getenv(env);
    if (envstr != NULL) {
      bool found = false;
      const struct dart_env_str2int *val;
      while ((val = &values[i++])->envstr != NULL) {
        if (strcasecmp(envstr, val->envstr) == 0) {
          res = val->value;
          found = true;
          break;
        }
      }
      if (!found) {
        DART_LOG_WARN("Unknown value %s found in environment variable %s",
                      envstr, env);
      }
    }
  }

  return res;
}

/**
 * Returns the number provided in the environment variable or -1
 * if the environment variable is not set or does not represent a number.
 */
int
dart__base__env__number(const char *env, int fallback)
{
  int result = fallback;
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

ssize_t dart__base__env__size(const char *env, ssize_t fallback)
{
  size_t res = fallback;
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


bool dart__base__env__bool(const char *env, bool fallback)
{
  bool res = fallback;
  const char *envstr = getenv(env);
  if (envstr != NULL) {
    if (strcasecmp(envstr, "yes")  == 0 ||
        strcasecmp(envstr, "true") == 0 ||
        atoi(envstr)                > 0)
    {
      res = true;
    }
  }

  DART_LOG_TRACE("%s: %s (%i)", env, envstr, res);
  return res;
}
