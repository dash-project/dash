/**
 *  \file logging.h
 *
 *  Logging macros.
 */
#ifndef DART__BASE__LOGGING_H__
#define DART__BASE__LOGGING_H__

#include <dash/dart/base/config.h>
#if defined(DART__PLATFORM__LINUX) && !defined(_GNU_SOURCE)
#  define _GNU_SOURCE
#endif
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_config.h>
#include <dash/dart/if/dart_team_group.h>

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

enum dart__base__term_color_code {
  DART_LOG_TCOL_DEFAULT = 0,
  DART_LOG_TCOL_WHITE,
  DART_LOG_TCOL_RED,
  DART_LOG_TCOL_GREEN,
  DART_LOG_TCOL_YELLOW,
  DART_LOG_TCOL_BLUE,
  DART_LOG_TCOL_MAGENTA,
  DART_LOG_TCOL_CYAN,
  DART_LOG_TCOL_NUM_CODES
};

#ifdef __cplusplus
extern "C" {
#endif

const int dart__base__term_colors[DART_LOG_TCOL_NUM_CODES];

const int dart__base__unit_term_colors[DART_LOG_TCOL_NUM_CODES-1];

#ifdef __cplusplus
} /* extern "C" */
#endif

/* GNU variant of basename.3 */
inline char * dart_base_logging_basename(char *path) {
    char *base = strrchr(path, '/');
    return base ? base+1 : path;
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
    dart_global_unit_t unit_id; \
    dart_myid(&unit_id); \
    fprintf(DART_LOG_OUTPUT_TARGET, \
      "[ %*d ERROR ] [ %*d ] %-*s:%-*d !!! DART: %s\n", \
      DASH__DART_LOGGING__UNIT__WIDTH, unit_id.id, \
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
    dart_global_unit_t unit_id; \
    dart_myid(&unit_id); \
    fprintf(DART_LOG_OUTPUT_TARGET, \
      "[ %*d TRACE ] [ %*d ] %-*s:%-*d :   DART: %s\n", \
      DASH__DART_LOGGING__UNIT__WIDTH, unit_id.id, \
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
    dart_global_unit_t unit_id; \
    dart_myid(&unit_id); \
    fprintf(DART_LOG_OUTPUT_TARGET, \
      "[ %*d DEBUG ] [ %*d ] %-*s:%-*d :   DART: %s\n", \
      DASH__DART_LOGGING__UNIT__WIDTH, unit_id.id, \
      DASH__DART_LOGGING__PROC__WIDTH, pid, \
      DASH__DART_LOGGING__FILE__WIDTH, dart_base_logging_basename(__FILE__), \
      DASH__DART_LOGGING__LINE__WIDTH, __LINE__, \
      msg_buf); \
  } while (0)

#define DART_LOG_INFO(...) \
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
    dart_global_unit_t unit_id; \
    dart_myid(&unit_id); \
    fprintf(DART_LOG_OUTPUT_TARGET, \
      "[ %*d INFO  ] [ %*d ] %-*s:%-*d :   DART: %s\n", \
      DASH__DART_LOGGING__UNIT__WIDTH, unit_id.id, \
      DASH__DART_LOGGING__PROC__WIDTH, pid, \
      DASH__DART_LOGGING__FILE__WIDTH, dart_base_logging_basename(__FILE__), \
      DASH__DART_LOGGING__LINE__WIDTH, __LINE__, \
      msg_buf); \
  } while (0)

#define DART_LOG_TRACE_ARRAY(context, fmt, array, nelem) \
  do { \
    int  nchars = (nelem) * 10 + (nelem) * 2; \
    char array_buf[nchars]; \
    array_buf[0] = '\0'; \
    for (int i = 0; i < nelem; i++) { \
      char value_buf[32]; \
      value_buf[0] = '\0'; \
      snprintf(value_buf, 32, fmt " ", (array)[i]); \
      strncat(array_buf, value_buf, 32); \
    } \
    DART_LOG_TRACE(context ": %s = { %s}", #array, array_buf); \
  } while (0)

#else /* DART_ENABLE_LOGGING */

#define DART_LOG_TRACE(...) do { } while(0)
#define DART_LOG_DEBUG(...) do { } while(0)
#define DART_LOG_INFO(...)  do { } while(0)

#define DART_LOG_TRACE_ARRAY(...) do { } while(0)

#endif /* DART_ENABLE_LOGGING */

#endif /* DART__BASE__LOGGING_H__ */
