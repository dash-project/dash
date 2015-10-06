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

#ifdef DASH_ENABLE_DART_LOGGING

#define DART_LOG_TRACE(format, ...) \
  do { \
    dart_unit_t unit_id = -1; \
    dart_myid(&unit_id); \
    printf("[ %d  DART  | TRACE ] %s:%d  | " format "\n", \
           unit_id, \
           dart_base_logging_basename(__FILE__), __LINE__, ##__VA_ARGS__); \
  } while (0)

#define DART_LOG_DEBUG(format, ...) \
  do { \
    dart_unit_t unit_id = -1; \
    dart_myid(&unit_id); \
    printf("[ %d  DART  | DEBUG ] %s:%d  | " format "\n", \
           unit_id, \
           dart_base_logging_basename(__FILE__), __LINE__, ##__VA_ARGS__); \
  } while (0)

#define DART_LOG_INFO(format, ...) \
  do { \
    dart_unit_t unit_id = -1; \
    dart_myid(&unit_id); \
    printf("[ %d  DART  | INFO  ] %s:%d  | " format "\n", \
           unit_id, \
           dart_base_logging_basename(__FILE__), __LINE__, ##__VA_ARGS__); \
  } while (0)

#else /* DASH_ENABLE_DART_LOGGING */

#define DART_LOG_TRACE(format, ...)
#define DART_LOG_DEBUG(format, ...)
#define DART_LOG_INFO(format, ...)

#endif /* DASH_ENABLE_DART_LOGGING */

#define DART_LOG_ERROR(format, ...) \
  do { \
    dart_unit_t unit_id = -1; \
    dart_myid(&unit_id); \
    printf("[ %d  DART  | ERROR ] %s:%d  | " format "\n", \
           unit_id, \
           dart_base_logging_basename(__FILE__), __LINE__, ##__VA_ARGS__); \
  } while(0)

#endif /* DART__BASE__LOGGING_H__ */
