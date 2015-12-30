/** 
 *  \file logging.h
 *
 *  Logging macros.
 */
#ifndef DART__BASE__LOGGING_H__
#define DART__BASE__LOGGING_H__

#include <string.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_team_group.h>

/* GNU variant of basename.3 */
inline char * dart_base_logging_basename(char *path) {
    char *base = strrchr(path, '/');
    return base ? base+1 : path;
}

/* Width of unit id field in log messages in number of characters */
#define DASH__DART_LOGGING__UNIT__WIDTH 4
/* Width of file name field in log messages in number of characters */
#define DASH__DART_LOGGING__FILE__WIDTH 25
/* Width of line number field in log messages in number of characters */
#define DASH__DART_LOGGING__LINE__WIDTH 4

#ifdef DART_ENABLE_LOGGING

#define DART_LOG_TRACE(format, ...) \
  do { \
    dart_unit_t unit_id = -1; \
    dart_myid(&unit_id); \
    printf("[ %*d TRACE ] %-*s:%-*d |   DART: " format "\n", \
           DASH__DART_LOGGING__UNIT__WIDTH, unit_id, \
           DASH__DART_LOGGING__FILE__WIDTH, \
           dart_base_logging_basename(__FILE__), \
           DASH__DART_LOGGING__LINE__WIDTH, \
           __LINE__, ##__VA_ARGS__); \
  } while (0)

#define DART_LOG_DEBUG(format, ...) \
  do { \
    dart_unit_t unit_id = -1; \
    dart_myid(&unit_id); \
    printf("[ %*d DEBUG ] %-*s:%-*d |   DART: " format "\n", \
           DASH__DART_LOGGING__UNIT__WIDTH,  unit_id, \
           DASH__DART_LOGGING__FILE__WIDTH, \
           dart_base_logging_basename(__FILE__), \
           DASH__DART_LOGGING__LINE__WIDTH, \
           __LINE__, ##__VA_ARGS__); \
  } while (0)

#define DART_LOG_INFO(format, ...) \
  do { \
    dart_unit_t unit_id = -1; \
    dart_myid(&unit_id); \
    printf("[ %*d INFO  ] %-*s:%-*d |   DART: " format "\n", \
           DASH__DART_LOGGING__UNIT__WIDTH,  unit_id, \
           DASH__DART_LOGGING__FILE__WIDTH, \
           dart_base_logging_basename(__FILE__), \
           DASH__DART_LOGGING__LINE__WIDTH, \
           __LINE__, ##__VA_ARGS__); \
  } while (0)

#else /* DART_ENABLE_LOGGING */

#define DART_LOG_TRACE(format, ...)
#define DART_LOG_DEBUG(format, ...)
#define DART_LOG_INFO(format, ...)

#endif /* DART_ENABLE_LOGGING */

#define DART_LOG_ERROR(format, ...) \
  do { \
    dart_unit_t unit_id = -1; \
    dart_myid(&unit_id); \
    printf("[ %*d ERROR ] %-*s:%-*d |   DART: " format "\n", \
           DASH__DART_LOGGING__UNIT__WIDTH,  unit_id, \
           DASH__DART_LOGGING__FILE__WIDTH, \
           dart_base_logging_basename(__FILE__), \
           DASH__DART_LOGGING__LINE__WIDTH, \
           __LINE__, ##__VA_ARGS__); \
  } while(0)

#endif /* DART__BASE__LOGGING_H__ */
