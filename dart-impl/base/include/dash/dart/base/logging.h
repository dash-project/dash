/**
 *  \file logging.h
 *
 *  Logging macros.
 */
#ifndef DART__BASE__LOGGING_H__
#define DART__BASE__LOGGING_H__

#include <string.h>
#include <dash/dart/base/logging.h>
#include <stdio.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_team_group.h>

#include <dash/dart/base/assert.h>


/* Width of unit id field in log messages in number of characters */
#define DASH__DART_LOGGING__UNIT__WIDTH 4
/* Width of file name field in log messages in number of characters */
#define DASH__DART_LOGGING__FILE__WIDTH 25
/* Width of line number field in log messages in number of characters */
#define DASH__DART_LOGGING__LINE__WIDTH 4
/* Maximum length of a single log message in number of characters */
#define DASH__DART_LOGGING__MAX_MESSAGE_LENGTH 256;

#ifdef DART_LOG_OUTPUT_STDERR
#define DART_LOG_OUTPUT_TARGET stderr
#else
#define DART_LOG_OUTPUT_TARGET stdout
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
    sn_ret = snprintf(msg_buf, maxlen, __VA_ARGS__); \
    if (sn_ret < 0 || sn_ret >= maxlen) { \
      break; \
    } \
    dart_unit_t unit_id = -1; \
    dart_myid(&unit_id); \
    fprintf(DART_LOG_OUTPUT_TARGET, \
      "[ %*d ERROR ] %-*s:%-*d |   DART: %s\n", \
      DASH__DART_LOGGING__UNIT__WIDTH, unit_id, \
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
    const int maxlen = DASH__DART_LOGGING__MAX_MESSAGE_LENGTH; \
    int       sn_ret; \
    char      msg_buf[maxlen]; \
    sn_ret = snprintf(msg_buf, maxlen, __VA_ARGS__); \
    if (sn_ret < 0 || sn_ret >= maxlen) { \
      break; \
    } \
    dart_unit_t unit_id = -1; \
    dart_myid(&unit_id); \
    fprintf(DART_LOG_OUTPUT_TARGET, \
      "[ %*d TRACE ] %-*s:%-*d |   DART: %s\n", \
      DASH__DART_LOGGING__UNIT__WIDTH, unit_id, \
      DASH__DART_LOGGING__FILE__WIDTH, dart_base_logging_basename(__FILE__), \
      DASH__DART_LOGGING__LINE__WIDTH, __LINE__, \
      msg_buf); \
  } while (0)

#define DART_LOG_DEBUG(...) \
  do { \
    const int maxlen = DASH__DART_LOGGING__MAX_MESSAGE_LENGTH; \
    int       sn_ret; \
    char      msg_buf[maxlen]; \
    sn_ret = snprintf(msg_buf, maxlen, __VA_ARGS__); \
    if (sn_ret < 0 || sn_ret >= maxlen) { \
      break; \
    } \
    dart_unit_t unit_id = -1; \
    dart_myid(&unit_id); \
    fprintf(DART_LOG_OUTPUT_TARGET, \
      "[ %*d DEBUG ] %-*s:%-*d |   DART: %s\n", \
      DASH__DART_LOGGING__UNIT__WIDTH, unit_id, \
      DASH__DART_LOGGING__FILE__WIDTH, dart_base_logging_basename(__FILE__), \
      DASH__DART_LOGGING__LINE__WIDTH, __LINE__, \
      msg_buf); \
  } while (0)

#define DART_LOG_INFO(...) \
  do { \
    const int maxlen = DASH__DART_LOGGING__MAX_MESSAGE_LENGTH; \
    int       sn_ret; \
    char      msg_buf[maxlen]; \
    sn_ret = snprintf(msg_buf, maxlen, __VA_ARGS__); \
    if (sn_ret < 0 || sn_ret >= maxlen) { \
      break; \
    } \
    dart_unit_t unit_id = -1; \
    dart_myid(&unit_id); \
    fprintf(DART_LOG_OUTPUT_TARGET, \
      "[ %*d INFO  ] %-*s:%-*d |   DART: %s\n", \
      DASH__DART_LOGGING__UNIT__WIDTH, unit_id, \
      DASH__DART_LOGGING__FILE__WIDTH, dart_base_logging_basename(__FILE__), \
      DASH__DART_LOGGING__LINE__WIDTH, __LINE__, \
      msg_buf); \
  } while (0)

#else /* DART_ENABLE_LOGGING */

#define DART_LOG_TRACE(...) do { } while(0)
#define DART_LOG_DEBUG(...) do { } while(0)
#define DART_LOG_INFO(...)  do { } while(0)

#endif /* DART_ENABLE_LOGGING */

#endif /* DART__BASE__LOGGING_H__ */
