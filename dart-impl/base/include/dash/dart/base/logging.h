/**
 *  \file logging.h
 *
 *  Logging macros.
 */
#ifndef DART__BASE__LOGGING_H__
#define DART__BASE__LOGGING_H__

#include <stdio.h>
#include <string.h>

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

extern const int dart__base__term_colors[DART_LOG_TCOL_NUM_CODES];

extern const int dart__base__unit_term_colors[DART_LOG_TCOL_NUM_CODES-1];

#ifdef __cplusplus
} /* extern "C" */
#endif


enum dart__base__logging_loglevel{
  DART_LOGLEVEL_ERROR = 0,
  DART_LOGLEVEL_WARN,
  DART_LOGLEVEL_INFO,
  DART_LOGLEVEL_DEBUG,
  DART_LOGLEVEL_TRACE,
  DART_LOGLEVEL_NUM_LEVEL
};

void
dart__base__log_message(
  const char *filename,
  int         line,
  int         level,
  const char *format,
  ...
) __attribute__((format(printf, 4, 5)));

//
// Always log error messages and warnings:
//
#define DART_LOG_ERROR(...) \
  dart__base__log_message(  \
    __FILE__,               \
    __LINE__,               \
    DART_LOGLEVEL_ERROR,    \
    __VA_ARGS__);

#define DART_LOG_WARN(...)  \
  dart__base__log_message(  \
    __FILE__,               \
    __LINE__,               \
    DART_LOGLEVEL_WARN,     \
    __VA_ARGS__);

#define DART_LOG_INFO_ALWAYS(...)    \
  dart__base__log_message(    \
      __FILE__,               \
      __LINE__,               \
      DART_LOGLEVEL_INFO,     \
      __VA_ARGS__             \
    );

//
// Debug, Info, and Trace log messages:
//
#ifdef DART_ENABLE_LOGGING

#define DART_LOG_TRACE(...) \
  dart__base__log_message(   \
    __FILE__,               \
    __LINE__,               \
    DART_LOGLEVEL_TRACE,    \
    __VA_ARGS__);

#define DART_LOG_DEBUG(...) \
  dart__base__log_message(   \
    __FILE__,               \
    __LINE__,               \
    DART_LOGLEVEL_DEBUG,    \
    __VA_ARGS__);

#define DART_LOG_INFO(...)    \
  dart__base__log_message(    \
      __FILE__,               \
      __LINE__,               \
      DART_LOGLEVEL_INFO,     \
      __VA_ARGS__             \
    );

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

#define DART_LOG_TRACE_UNITID_ARRAY(context, fmt, array, nelem) \
  do { \
    int  nchars = (nelem) * 10 + (nelem) * 2; \
    char array_buf[nchars]; \
    array_buf[0] = '\0'; \
    for (int i = 0; i < nelem; i++) { \
      char value_buf[32]; \
      value_buf[0] = '\0'; \
      snprintf(value_buf, 32, fmt " ", (array)[i].id); \
      strncat(array_buf, value_buf, 32); \
    } \
    DART_LOG_TRACE(context ": %s = { %s}", #array, array_buf); \
  } while (0)


#else /* DART_ENABLE_LOGGING */

#define DART_LOG_TRACE(...) do { } while(0)
#define DART_LOG_DEBUG(...) do { } while(0)
#define DART_LOG_INFO(...)  do { } while(0)

#define DART_LOG_TRACE_ARRAY(...) do { } while(0)
#define DART_LOG_TRACE_UNITID_ARRAY(...) do { } while(0)

#endif /* DART_ENABLE_LOGGING */

#endif /* DART__BASE__LOGGING_H__ */
