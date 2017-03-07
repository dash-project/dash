#include <stdarg.h>
#include <stdio.h>
#if defined(DART__PLATFORM__LINUX) && !defined(_GNU_SOURCE)
#  define _GNU_SOURCE
#endif
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_config.h>
#include <dash/dart/if/dart_team_group.h>

#include <dash/dart/base/logging.h>
#include <dash/dart/base/mutex.h>


static dart_mutex_t logmutex = DART_MUTEX_INITIALIZER;

#define DART_LOGLEVEL_ENVSTR "DART_LOG_LEVEL"

const int dart__base__term_colors[DART_LOG_TCOL_NUM_CODES] = {
  39, // default
  37, // white
  31, // red
  32, // green
  33, // yellow
  34, // blue
  35, // magenta
  36  // cyan
};

const int dart__base__unit_term_colors[DART_LOG_TCOL_NUM_CODES-1] = {
  DART_LOG_TCOL_CYAN,
  DART_LOG_TCOL_YELLOW,
  DART_LOG_TCOL_MAGENTA,
  DART_LOG_TCOL_WHITE,
  DART_LOG_TCOL_GREEN,
  DART_LOG_TCOL_RED,
  DART_LOG_TCOL_BLUE
};

static const char *loglevel_names[DART_LOGLEVEL_NUM_LEVEL] = {
    "ERROR",
    "WARN",
    "INFO",
    "DEBUG",
    "TRACE"
};

static
enum dart__base__logging_loglevel
env_loglevel()
{
  static enum dart__base__logging_loglevel level = DART_LOGLEVEL_TRACE;
  static int log_level_parsed = 0;
  if (!log_level_parsed) {
    const char *envstr = getenv(DART_LOGLEVEL_ENVSTR);
    if (envstr) {
      if (strcmp(envstr, "ERROR") == 0) {
        level = DART_LOGLEVEL_ERROR;
      } else if (strcmp(envstr, "WARN") == 0) {
        level = DART_LOGLEVEL_WARN;
      } else if (strcmp(envstr, "INFO") == 0) {
        level = DART_LOGLEVEL_INFO;
      } else if (strcmp(envstr, "DEBUG") == 0) {
        level = DART_LOGLEVEL_DEBUG;
      } else if (strcmp(envstr, "TRACE") == 0) {
        level = DART_LOGLEVEL_TRACE;
      }
    }
  }

  return level;
}

/* GNU variant of basename.3 */
static inline
const char * dart_base_logging_basename(const char *path) {
    char *base = strrchr(path, '/');
    return base ? base+1 : path;
}

void
dart__logging__message(
  const char *filename,
  int         line,
  int         level,
  const char *format,
  ...
)
{
  if (level <= env_loglevel() ||
      level > DART_LOGLEVEL_TRACE) {
    return;
  }
  va_list argp;
  va_start(argp, format);
  const int maxlen = DASH__DART_LOGGING__MAX_MESSAGE_LENGTH;
  int       sn_ret;
  char      msg_buf[maxlen];
  pid_t     pid = getpid();
  sn_ret = vsnprintf(msg_buf, maxlen, format, argp);
//  if (sn_ret < 0 || sn_ret >= maxlen) {
//    break;
//  }
  dart_global_unit_t unit_id;
  dart_myid(&unit_id);
  // avoid inter-thread log interference
  dart_mutex_lock(&logmutex);
  fprintf(DART_LOG_OUTPUT_TARGET,
    "[ %*d %.5s ] [ %*d ] %-*s:%-*d %.3s DART: %s\n",
    DASH__DART_LOGGING__UNIT__WIDTH, unit_id.id,
    loglevel_names[level],
    DASH__DART_LOGGING__PROC__WIDTH, pid,
    DASH__DART_LOGGING__FILE__WIDTH, dart_base_logging_basename(filename),
    DASH__DART_LOGGING__LINE__WIDTH, line,
    (level < DART_LOGLEVEL_INFO) ? "!!!" : "",
    msg_buf);
  va_end(argp);
  dart_mutex_unlock(&logmutex);
}
