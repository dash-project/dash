/** @file dart_communication_priv.h
 *  @date 25 Aug 2014
 *  @brief Definition of dart_handle_struct
 */
#ifndef DART_ADAPT_COMMUNICATION_PRIV_H_INCLUDED
#define DART_ADAPT_COMMUNICATION_PRIV_H_INCLUDED

#include <stdio.h>
#include <mpi.h>
#include <stdbool.h>

#include <dash/dart/base/macro.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_globmem.h>
#include <dash/dart/if/dart_communication.h>

DART_INTERNAL
extern int dart__mpi__datatype_sizes[DART_TYPE_COUNT];

/** DART handle type for non-blocking one-sided operations. */
struct dart_handle_struct
{
  MPI_Request request;
  MPI_Win     win;
  dart_unit_t dest;
  bool        needs_flush;
};

dart_ret_t
dart__mpi__datatype_init() DART_INTERNAL;

dart_ret_t
dart__mpi__datatype_fini() DART_INTERNAL;

static inline MPI_Op dart__mpi__op(dart_operation_t dart_op) {
  switch (dart_op) {
    case DART_OP_MIN     : return MPI_MIN;
    case DART_OP_MAX     : return MPI_MAX;
    case DART_OP_SUM     : return MPI_SUM;
    case DART_OP_PROD    : return MPI_PROD;
    case DART_OP_BAND    : return MPI_BAND;
    case DART_OP_LAND    : return MPI_LAND;
    case DART_OP_BOR     : return MPI_BOR;
    case DART_OP_LOR     : return MPI_LOR;
    case DART_OP_BXOR    : return MPI_BXOR;
    case DART_OP_LXOR    : return MPI_LXOR;
    case DART_OP_REPLACE : return MPI_REPLACE;
    case DART_OP_NO_OP   : return MPI_NO_OP;
    default              : return (MPI_Op)(-1);
  }
}

static inline MPI_Datatype dart__mpi__datatype(dart_datatype_t dart_datatype) {
  switch (dart_datatype) {
    case DART_TYPE_BYTE     : return MPI_BYTE;
    case DART_TYPE_SHORT    : return MPI_SHORT;
    case DART_TYPE_INT      : return MPI_INT;
    case DART_TYPE_UINT     : return MPI_UNSIGNED;
    case DART_TYPE_LONG     : return MPI_LONG;
    case DART_TYPE_ULONG    : return MPI_UNSIGNED_LONG;
    case DART_TYPE_LONGLONG : return MPI_LONG_LONG_INT;
    case DART_TYPE_FLOAT    : return MPI_FLOAT;
    case DART_TYPE_DOUBLE   : return MPI_DOUBLE;
    default                 : return (MPI_Datatype)(-1);
  }
}

static inline int dart__mpi__datatype_sizeof(dart_datatype_t dart_datatype) {

  if (dart_datatype > DART_TYPE_UNDEFINED && dart_datatype < DART_TYPE_COUNT)
  {
    return dart__mpi__datatype_sizes[dart_datatype];
  }
  return -1;
}


#if 0
static inline int dart_mpi_datatype_disp_unit(dart_datatype_t dart_datatype) {
  switch (dart_datatype) {
    case DART_TYPE_BYTE     : return 1;
    case DART_TYPE_SHORT    : return 1;
    case DART_TYPE_INT      : return 4;
    case DART_TYPE_UINT     : return 4;
    case DART_TYPE_LONG     : return 4;
    case DART_TYPE_ULONG    : return 4;
    case DART_TYPE_LONGLONG : return 8;
    case DART_TYPE_FLOAT    : return 4;
    case DART_TYPE_DOUBLE   : return 8;
    default                 : return 1;
  }
}
#endif

#endif /* DART_ADAPT_COMMUNICATION_PRIV_H_INCLUDED */
