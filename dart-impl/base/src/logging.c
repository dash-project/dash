#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <time.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_config.h>
#include <dash/dart/if/dart_team_group.h>

#include <dash/dart/base/logging.h>
#include <dash/dart/base/mutex.h>


/* Width of unit id field in log messages in number of characters */
#define UNIT_WIDTH 4
/* Width of process id field in log messages in number of characters */
#define PROC_WIDTH 5
/* Width of file name field in log messages in number of characters */
#define FILE_WIDTH 25
/* Width of line number field in log messages in number of characters */
#define LINE_WIDTH 4
/* Maximum length of a single log message in number of characters */
#define MAX_MESSAGE_LENGTH 256;


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

static inline
double dart_base_logging_timestamp_ms()
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ((ts.tv_sec * 1E3)
            + (ts.tv_nsec / 1E6));
}

void
dart__base__log_message(
  const char *filename,
  int         line,
  int         level,
  const char *format,
  ...
)
{
  if (level > env_loglevel() ||
      level > DART_LOGLEVEL_TRACE) {
    return;
  }
  va_list argp;
  va_start(argp, format);
  const int maxlen = MAX_MESSAGE_LENGTH;
  char      msg_buf[maxlen];
  vsnprintf(msg_buf, maxlen, format, argp);
//  if (sn_ret < 0 || sn_ret >= maxlen) {
//    break;
//  }
  dart_global_unit_t unit_id;
  dart_myid(&unit_id);
  // avoid inter-thread log interference
  dart__base__mutex_lock(&logmutex);
  fprintf(DART_LOG_OUTPUT_TARGET,
    "[ %*d %.5s ] [ %.3f ] %-*s:%-*d %.3s DART: %s\n",
    UNIT_WIDTH, unit_id.id,
    loglevel_names[level],
    dart_base_logging_timestamp_ms(),
    FILE_WIDTH, dart_base_logging_basename(filename),
    LINE_WIDTH, line,
    (level < DART_LOGLEVEL_INFO) ? "!!!" : "",
    msg_buf);
  va_end(argp);
  dart__base__mutex_unlock(&logmutex);
}
