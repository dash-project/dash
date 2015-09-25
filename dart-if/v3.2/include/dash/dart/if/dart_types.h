
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

typedef enum
  {
    DART_OP_UNDEFINED = 0,
    DART_OP_MIN,
    DART_OP_MAX,
    /** Corresponds to MPI_SUM with rhs operand value >= 0 */
    DART_OP_ADD,
    /** Corresponds to MPI_SUM with rhs operand value < 0 */
    DART_OP_SUB,
    /** Corresponds to MPI_PROD with rhs operand value >= 0 */
    DART_OP_MUL,
    /** Corresponds to MPI_PROD with rhs operand value < 0 */
    DART_OP_DIV,
    DART_OP_BAND,
    DART_OP_LAND,
    DART_OP_BOR,
    DART_OP_LOR,
    DART_OP_BXOR,
    DART_OP_LXOR
  } dart_operation_t;

typedef int32_t dart_unit_t;
typedef int32_t dart_team_t;

#define DART_INTERFACE_OFF

#ifdef __cplusplus
}
#endif

#endif /* DART_TYPES_H_INCLUDED */
