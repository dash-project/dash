
#ifndef DART_TYPES_H_INCLUDED
#define DART_TYPES_H_INCLUDED

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DART_INTERFACE_ON

/*
   --- DART Types ---
*/

typedef enum 
  {
    DART_OK           = 0,
    DART_PENDING      = 1,
    DART_ERR_INVAL    = 2,
    DART_ERR_NOTFOUND = 3,
    DART_ERR_NOTINIT  = 4,
    DART_ERR_OTHER    = 999,
    /* add error codes as needed */
  } dart_ret_t;

typedef int32_t dart_unit_t;
typedef int32_t dart_team_t;

#define DART_INTERFACE_OFF

#ifdef __cplusplus
}
#endif

#endif /* DART_TYPES_H_INCLUDED */
