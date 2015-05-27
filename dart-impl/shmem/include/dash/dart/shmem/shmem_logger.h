#ifndef SHMEM_LOGGER_H_INCLUDED
#define SHMEM_LOGGER_H_INCLUDED

#include <errno.h>
#include <string.h>
#include <stdio.h>

#include "extern_c.h"
EXTERN_C_BEGIN

// KF #include "dart/dart_return_codes.h"
// KF #include "dart/dart_gptr.h"

extern int _glob_myid;

#define XDEBUG(format, ...)						\
  do{fprintf(stderr, "DEBUG|%d|" format "\n",				\
	     _glob_myid, __VA_ARGS__);}while(0)


#ifdef DART_DEBUG
#define DEBUG(format, ...)						\
  do{fprintf(stderr, "DEBUG|%d|" format "\n",				\
	     _glob_myid, __VA_ARGS__);}while(0)
#else
#define DEBUG(format, ...)
#endif

#ifdef DART_LOG
#define LOG(format, ...)						\
  do{fprintf(stderr, "INFO |%d|" format "\n",				\
	     _glob_myid, __VA_ARGS__); }while(0)
#else
#define LOG(format, ...)
#endif

#define ERROR(format, ...)						\
  do{fprintf(stderr, "\e[1;31mERROR\e[0m|%d|%s,%d|" format "\n",	\
	     _glob_myid, __FILE__, __LINE__, __VA_ARGS__); }while(0)

#define ERRNO(format, ...)						\
  do{char* s = strerror(errno);						\
    fprintf(stderr, "\e[1;31mERRNO\e[0m|%d|%s,%d|strerror: %s|" format "\n", \
	    _glob_myid, __FILE__, __LINE__, s, __VA_ARGS__); }while(0)

#define DART_SAFE(fncall) do {                                       \
    int _retval;                                                     \
    if ((_retval = fncall) != DART_OK) {                             \
      fprintf(stderr, "ERROR %d calling: %s"                         \
	      " at: %s:%i",					     \
              _retval, #fncall, __FILE__, __LINE__);                 \
      fflush(stderr);                                                \
    }                                                                \
  } while(0)



#define PTHREAD_SAFE(fncall) do {				     \
    int _retval;						     \
    if ((_retval = fncall) != 0) {				     \
      fprintf(stderr, "ERROR calling: %s\n"			     \
	      " at: %s:%i, return value: %s\n",			     \
	      #fncall, __FILE__, __LINE__, strerror(_retval));	     \
      fflush(stderr);						     \
      return -999;						     \
    }								     \
  } while(0)

#define PTHREAD_SAFE_NORET(fncall) do {					\
    int _retval;							\
    if ((_retval = fncall) != 0) {					\
      fprintf(stderr, "ERROR calling: %s\n"				\
	      " at: %s:%i, return value: %s\n",				\
	      #fncall, __FILE__, __LINE__, strerror(_retval));		\
      fflush(stderr);							\
    }									\
  } while(0)

// KF
//char* gptr_to_string(gptr_t ptr);

EXTERN_C_END

#endif /* SHMEM_LOGGER_H_INCLUDED */
