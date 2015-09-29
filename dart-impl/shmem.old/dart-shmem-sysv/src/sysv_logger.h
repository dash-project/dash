/*
 * dart_logger_pthread.h
 *
 *  Created on: Apr 3, 2013
 *      Author: maierm
 */

#ifndef DART_LOGGER_PTHREAD_H_
#define DART_LOGGER_PTHREAD_H_

#include <errno.h>
#include <string.h>
#include <stdio.h>

#ifdef DART_DEBUG
#define DEBUG(format, ...) do{fprintf(stderr, "DEBUG|" format "\n", __VA_ARGS__); }while(0)
#else
#define DEBUG(format, ...)
#endif

#ifdef DART_LOG
#define LOG(format, ...) do{fprintf(stderr, "INFO |" format "\n", __VA_ARGS__); }while(0)
#else
#define LOG(format, ...)
#endif

#define ERROR(format, ...) do{fprintf(stderr, "\e[1;31mERROR\e[0m|%s,%d|" format "\n", __FILE__, __LINE__, __VA_ARGS__); }while(0)

#define ERRNO(format, ...) do{char* s = strerror(errno); fprintf(stderr, "\e[1;31mERRNO\e[0m|%s,%d|strerror: %s|" format "\n", __FILE__, __LINE__, s, __VA_ARGS__); }while(0)

#define PTHREAD_SAFE(fncall) do {                                    \
    int _retval;                                                     \
    if ((_retval = fncall) != 0) {                                   \
      fprintf(stderr, "ERROR calling: %s\n"                          \
                      " at: %s:%i, return value: %s\n",              \
              #fncall, __FILE__, __LINE__, strerror(_retval));\
      fflush(stderr);                                                \
      return -999;                                                   \
    }                                                                \
  } while(0)

#define PTHREAD_SAFE_NORET(fncall) do {                              \
    int _retval;                                                     \
    if ((_retval = fncall) != 0) {                                   \
      fprintf(stderr, "ERROR calling: %s\n"                          \
                      " at: %s:%i, return value: %s\n",              \
              #fncall, __FILE__, __LINE__, strerror(_retval));\
      fflush(stderr);                                                \
    }                                                                \
  } while(0)

#endif /* DART_LOGGER_PTHREAD_H_ */
