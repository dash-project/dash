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
#include <dash/dart/base/env.h>
#include <dash/dart/base/mutex.h>

#include <dash/dart/if/dart_tasking.h>

#define DART_LOGLEVEL_ENVSTR      "DART_LOG_LEVEL"

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


static const struct dart_env_str2int env_vals[] = {
  {"ERROR", DART_LOGLEVEL_ERROR},
  {"WARN",  DART_LOGLEVEL_WARN},
  {"INFO",  DART_LOGLEVEL_INFO},
  {"DEBUG", DART_LOGLEVEL_DEBUG},
  {"TRACE", DART_LOGLEVEL_TRACE},
  {NULL, -1}
};


/**
 * Returns the log level set in DART_LOG_LEVEL, defaults to DART_LOGLEVEL_TRACE
 * if the environment variable is not set.
 */
enum dart__base__logging_loglevel
dart__logging__log_level()
{
  static enum dart__base__logging_loglevel level = DART_LOGLEVEL_TRACE;
  static int parsed = 0;
  if (!parsed) {
    parsed = 1;
    level  = dart__base__env__str2int(DART_LOGLEVEL_ENVSTR, env_vals,
                                      DART_LOGLEVEL_TRACE);
  }

  return level;
}


static dart_mutex_t   logmutex = DART_MUTEX_INITIALIZER;
static pthread_key_t  tls; // thread-local storage
static int            initialized = 0;
static const char    *filename_base = NULL;

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
    " WARN",
    " INFO",
    "DEBUG",
    "TRACE"
};

/* GNU variant of basename.3 */
static inline
const char * dart_base_logging_basename(const char *path) {
    char *base = strrchr(path, '/');
    return base ? base+1 : path;
}

static void
close_logfile(void* ptr)
{
  FILE *file = (FILE*)ptr;
  fclose(file);
}

static void
make_tls()
{
  filename_base = dart__base__env__string("DART_LOG_FILE");
  if (filename_base) {
    (void) pthread_key_create(&tls, &close_logfile);
  }
  initialized = 1;
}

static FILE * get_logfile()
{
  static pthread_once_t key_once = PTHREAD_ONCE_INIT;
  (void) pthread_once(&key_once, &make_tls);
  if (initialized == 0)
  {
    // wait for the initialization to finish
    while (initialized == 0) {}
  }

  FILE *res = DART_LOG_OUTPUT_TARGET;

  if (filename_base) {
    FILE* file = pthread_getspecific(tls);
    if (!file) {
      dart_global_unit_t guid;
      dart_myid(&guid);
      if (guid.id >= 0) {
        char filename[512];
        int thread_num = dart_task_thread_num ? dart_task_thread_num() : 0;
        snprintf(filename, 512, "%s.%d.%d.log", filename_base,
                guid.id, thread_num);
        fprintf(DART_LOG_OUTPUT_TARGET, "Opening log file '%s': %d.%d\n", filename, guid.id, thread_num);
        file = fopen(filename, "w");
        pthread_setspecific(tls, file);
      } else {
        file = DART_LOG_OUTPUT_TARGET;
      }
    }
    res = file;
  }
  return res;
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
  if (level > dart__logging__log_level() ||
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
  FILE *file = get_logfile();
  // avoid inter-thread log interference
  if (file == DART_LOG_OUTPUT_TARGET)
    dart__base__mutex_lock(&logmutex);
  fprintf(file,
    "[ %*d:%-2d %.5s ] [ %.3f ] %-*s:%-*d %.3s DART: %s\n",
    UNIT_WIDTH, unit_id.id,
    dart_task_thread_num ? dart_task_thread_num() : 0,
    loglevel_names[level],
    dart_base_logging_timestamp_ms(),
    FILE_WIDTH, dart_base_logging_basename(filename),
    LINE_WIDTH, line,
    (level < DART_LOGLEVEL_INFO) ? "!!!" : "",
    msg_buf);
  va_end(argp);
  if (file == DART_LOG_OUTPUT_TARGET)
    dart__base__mutex_unlock(&logmutex);
}
