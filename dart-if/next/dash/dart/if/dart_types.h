#ifndef DART__TYPES_H
#define DART__TYPES_H

/* DART v4.0 */

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DART_INTERFACE_ON

/**
 * \file dart_types.h
 *
 * ### DART data types ###
 *
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

typedef enum
  {
    DART_OP_UNDEFINED = 0,
    DART_OP_MIN,
    DART_OP_MAX,
    DART_OP_SUM,
    DART_OP_PROD,
    DART_OP_BAND,
    DART_OP_LAND,
    DART_OP_BOR,
    DART_OP_LOR,
    DART_OP_BXOR,
    DART_OP_LXOR
  } dart_operation_t;

typedef enum
  {
    DART_TYPE_UNDEFINED = 0,
    DART_TYPE_BYTE,
    DART_TYPE_SHORT,
    DART_TYPE_INT,
    DART_TYPE_UINT,
    DART_TYPE_LONG,
    DART_TYPE_ULONG,
    DART_TYPE_LONGLONG,
    DART_TYPE_FLOAT,
    DART_TYPE_DOUBLE,
    DART_TYPE_FLOAT_INT,  /* {float , int} */
    DART_TYPE_DOUBLE_INT, /* {double, int} */
    DART_TYPE_SHORT_INT,  /* {short , int} */
    DART_TYPE_LONG_INT,   /* {long  , int} */
    DART_TYPE_INT_INT     /* {int   , int} */
  } dart_datatype_t;


typedef int32_t dart_unit_t;
typedef int32_t dart_team_t;

#define DART_UNDEFINED_UNIT_ID ((dart_unit_t)(-1))


#define DART_INTERFACE_OFF

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DART__TYPES_H */

