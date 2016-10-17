/**
 *  \file logging.h
 *
 *  Logging macros.
 */
#ifndef DART__BASE__LOGGING_H__
#define DART__BASE__LOGGING_H__

#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_config.h>
#include <dash/dart/if/dart_team_group.h>
#include <dash/dart/if/dart_tasking.h>

#ifdef DART_ENABLE_ASSERTIONS
#include <assert.h>
#endif


/* Width of unit id field in log messages in number of characters */
#define DASH__DART_LOGGING__UNIT__WIDTH 4
/* Width of process id field in log messages in number of characters */
#define DASH__DART_LOGGING__PROC__WIDTH 5
/* Width of file name field in log messages in number of characters */
#define DASH__DART_LOGGING__FILE__WIDTH 25
/* Width of line number field in log messages in number of characters */
#define DASH__DART_LOGGING__LINE__WIDTH 4
/* Maximum length of a single log message in number of characters */
#define DASH__DART_LOGGING__MAX_MESSAGE_LENGTH 256;

#ifdef DART_LOG_OUTPUT_STDOUT
#define DART_LOG_OUTPUT_TARGET stdout
#else
#define DART_LOG_OUTPUT_TARGET stderr
#endif

/* GNU variant of basename.3 */
static inline char * dart_base_logging_basename(char *path) {
    char *base = strrchr(path, '/');
    return base ? base+1 : path;
}

static inline double dart_base_logging_timestamp() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ((double)ts.tv_sec) + ((double)ts.tv_nsec / 1E9);
}

//
// Always log error messages:
//
#define DART_LOG_ERROR(...) \
  do { \
    const int maxlen = DASH__DART_LOGGING__MAX_MESSAGE_LENGTH; \
    int       sn_ret; \
    char      msg_buf[maxlen]; \
    pid_t     pid = getpid(); \
    sn_ret = snprintf(msg_buf, maxlen, __VA_ARGS__); \
    if (sn_ret < 0 || sn_ret >= maxlen) { \
      break; \
    } \
    dart_unit_t unit_id = -1; \
    dart_myid(&unit_id); \
    fprintf(DART_LOG_OUTPUT_TARGET, \
      "[ %*d:%d ERROR ] [ %f ] [ %*d ] %-*s:%-*d !!! DART: %s\n", \
      DASH__DART_LOGGING__UNIT__WIDTH, unit_id, dart_tasking_thread_num ? dart_tasking_thread_num() : 0, \
      dart_base_logging_timestamp(), \
      DASH__DART_LOGGING__PROC__WIDTH, pid, \
      DASH__DART_LOGGING__FILE__WIDTH, dart_base_logging_basename(__FILE__), \
      DASH__DART_LOGGING__LINE__WIDTH, __LINE__, \
      msg_buf); \
  } while (0)

//
// Debug, Info, and Trace log messages:
//
#ifdef DART_ENABLE_LOGGING

#define DART_LOG_TRACE(...) \
  do { \
    dart_config_t * dart_cfg;  \
    dart_config(&dart_cfg);    \
    if (!dart_cfg->log_enabled) { \
      break; \
    } \
    const int maxlen = DASH__DART_LOGGING__MAX_MESSAGE_LENGTH; \
    int       sn_ret; \
    char      msg_buf[maxlen]; \
    pid_t     pid = getpid(); \
    sn_ret = snprintf(msg_buf, maxlen, __VA_ARGS__); \
    if (sn_ret < 0 || sn_ret >= maxlen) { \
      break; \
    } \
    dart_unit_t unit_id = -1; \
    dart_myid(&unit_id); \
    fprintf(DART_LOG_OUTPUT_TARGET, \
      "[ %*d:%d TRACE ] [ %f ] [ %*d ] %-*s:%-*d :   DART: %s\n", \
      DASH__DART_LOGGING__UNIT__WIDTH, unit_id, dart_tasking_thread_num ? dart_tasking_thread_num() : 0, \
      dart_base_logging_timestamp(), \
      DASH__DART_LOGGING__PROC__WIDTH, pid, \
      DASH__DART_LOGGING__FILE__WIDTH, dart_base_logging_basename(__FILE__), \
      DASH__DART_LOGGING__LINE__WIDTH, __LINE__, \
      msg_buf); \
  } while (0)

#define DART_LOG_DEBUG(...) \
  do { \
    dart_config_t * dart_cfg;  \
    dart_config(&dart_cfg);    \
    if (!dart_cfg->log_enabled) { \
      break; \
    } \
    const int maxlen = DASH__DART_LOGGING__MAX_MESSAGE_LENGTH; \
    int       sn_ret; \
    char      msg_buf[maxlen]; \
    pid_t     pid = getpid(); \
    sn_ret = snprintf(msg_buf, maxlen, __VA_ARGS__); \
    if (sn_ret < 0 || sn_ret >= maxlen) { \
      break; \
    } \
    dart_unit_t unit_id = -1; \
    dart_myid(&unit_id); \
    fprintf(DART_LOG_OUTPUT_TARGET, \
      "[ %*d:%d DEBUG ] [ %f ] [ %*d ] %-*s:%-*d :   DART: %s\n", \
      DASH__DART_LOGGING__UNIT__WIDTH, unit_id, dart_tasking_thread_num ? dart_tasking_thread_num() : 0, \
      dart_base_logging_timestamp(), \
      DASH__DART_LOGGING__PROC__WIDTH, pid, \
      DASH__DART_LOGGING__FILE__WIDTH, dart_base_logging_basename(__FILE__), \
      DASH__DART_LOGGING__LINE__WIDTH, __LINE__, \
      msg_buf); \
  } while (0)

#define DART_LOG_INFO(...) \
  { \
    dart_config_t * dart_cfg;  \
    dart_config(&dart_cfg);    \
    if (dart_cfg->log_enabled) { \
      const int maxlen = DASH__DART_LOGGING__MAX_MESSAGE_LENGTH; \
      int       sn_ret; \
      char      msg_buf[maxlen]; \
      pid_t     pid = getpid(); \
      sn_ret = snprintf(msg_buf, maxlen, __VA_ARGS__); \
      if (sn_ret >= 0) { \
        dart_unit_t unit_id = -1; \
        dart_myid(&unit_id); \
        fprintf(DART_LOG_OUTPUT_TARGET, \
          "[ %*d:%d INFO  ] [ %f ] [ %*d ] %-*s:%-*d :   DART: %s\n", \
          DASH__DART_LOGGING__UNIT__WIDTH, unit_id, dart_tasking_thread_num ? dart_tasking_thread_num() : 0, \
          dart_base_logging_timestamp(), \
          DASH__DART_LOGGING__PROC__WIDTH, pid, \
          DASH__DART_LOGGING__FILE__WIDTH, dart_base_logging_basename(__FILE__), \
          DASH__DART_LOGGING__LINE__WIDTH, __LINE__, \
          msg_buf); \
      } \
    } \
  }

#else /* DART_ENABLE_LOGGING */

#define DART_LOG_TRACE(...) do { } while(0)
#define DART_LOG_DEBUG(...) do { } while(0)
#define DART_LOG_INFO(...)  do { } while(0)

#endif /* DART_ENABLE_LOGGING */

#endif /* DART__BASE__LOGGING_H__ */
