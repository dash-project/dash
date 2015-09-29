/** @file dart_logging.h
 *  @date 29 Sep 2015
 */
#ifndef DART__BASE__LOGGING_H__
#define DART__BASE__LOGGING_H__

#ifdef DASH_ENABLE_TRACE_LOGGING
#define LOG_TRACE(format, ...) \
  do { \
    printf ("[ TRACE    ] " format "\n", ##__VA_ARGS__); \
  } while (0)
#else
#define LOG_TRACE(format, ...)
#endif

#ifdef DASH_ENABLE_LOGGING
#define DEBUG(format, ...) \
  do { \
    printf ("[ DEBUG     ] (%s,%d) " format "\n", \
           __FILE__, __LINE__, ##__VA_ARGS__); \
  } while (0)
#else
#define DEBUG(format, ...)
#endif

#ifdef DASH_ENABLE_LOGGING
#define LOG(format, ...) \
  do { \
    printf ("[ INFO      ] (%s,%d) " format "\n", \
           __FILE__, __LINE__, ##__VA_ARGS__); \
  } while (0)
#else
#define LOG(format, ...)
#endif

#define ERROR(format, ...) \
  do { \
    printf("[ ERROR      ] (%s,%d) " format "\n", \
           __FILE__, __LINE__, ##__VA_ARGS__); \
  } while(0)

#endif /* DART__BASE__LOGGING_H__ */
