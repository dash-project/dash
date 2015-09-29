/*
 * heat_logger.h
 *
 *  Created on: Mar 13, 2013
 *      Author: maierm
 */

#ifndef DART_LOGGER_H_
#define DART_LOGGER_H_

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include "dart/dart_return_codes.h"
#include "dart/dart_gptr.h"

extern int _glob_myid;

#ifdef DART_DEBUG
#define DEBUG(format, ...) do{fprintf(stderr, "DEBUG|%d|" format "\n", _glob_myid, __VA_ARGS__); }while(0)
#else
#define DEBUG(format, ...)
#endif

#ifdef DART_LOG
#define LOG(format, ...) do{fprintf(stderr, "INFO |%d|" format "\n", _glob_myid, __VA_ARGS__); }while(0)
#else
#define LOG(format, ...)
#endif

#define ERROR(format, ...) do{fprintf(stderr, "\e[1;31mERROR\e[0m|%d|%s,%d|" format "\n", _glob_myid, __FILE__, __LINE__, __VA_ARGS__); }while(0)

#define ERRNO(format, ...) do{char* s = strerror(errno); fprintf(stderr, "\e[1;31mERRNO\e[0m|%d|%s,%d|strerror: %s|" format "\n", _glob_myid, __FILE__, __LINE__, s, __VA_ARGS__); }while(0)

#define DART_SAFE(fncall) do {                                       \
    int _retval;                                                     \
    if ((_retval = fncall) != DART_OK) {                             \
      fprintf(stderr, "ERROR %d calling: %s"                         \
                      " at: %s:%i\n",                                \
              _retval, #fncall, __FILE__, __LINE__);                 \
      fflush(stderr);                                                \
    }                                                                \
  } while(0)

char* gptr_to_string(gptr_t ptr);

#endif /* DART_LOGGER_H_ */
