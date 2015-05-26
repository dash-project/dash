/** @file dart_deb_log.h
 *  @date 25 Mar 2014
 */
#ifndef DART_DEB_LOG_H_INCLUDED
#define DART_DEB_LOG_H_INCLUDED

#ifdef ENABLE_DEBUG
#define DEBUG(format, ...) do { printf ("DEBUG, "format "\n", ##__VA_ARGS__); } while (0)
#else
#define DEBUG(format, ...)
#endif

#ifdef ENABLE_LOG
#define LOG(format, ...) do { printf ("LOG," format "\n", ##__VA_ARGS__); } while (0)
#else
#define LOG(format, ...)
#endif


#define ERROR(format, ...) do{printf("ERROR, |%s,%d| " format "\n", __FILE__, __LINE__, ##__VA_ARGS__); }while(0)

#endif /* DART_DEB_LOG_H_INCLUDED*/
