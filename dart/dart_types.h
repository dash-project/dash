
#ifndef DART_TYPES_H_INCLUDED
#define DART_TYPES_H_INCLUDED

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DART_INTERFACE_ON

#if __STDC_VERSION__ >= 199901L
#define HAVE_C99
#endif

/*
   --- DART Types ---
*/
#ifdef HAVE_C99
#include <stdint.h>
#else
#include "dart_stdint.h"
#endif

typedef enum 
  {
    DART_OK=0,
    DART_PENDING=1,
    DART_ERR_INVAL=2,
    DART_ERR_OTHER=3,
    /* add error codes as needed */
  } dart_ret_t;

typedef int32_t dart_unit_t;
typedef int32_t dart_team_t;

#define DART_INTERFACE_OFF

#ifdef __cplusplus
}
#endif

#endif /* DART_TYPES_H_INCLUDED */
