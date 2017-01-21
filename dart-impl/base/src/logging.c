#include <dash/dart/base/logging.h>

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


enum dart__base__logging_loglevel
dart__base__logging_env_loglevel()
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
