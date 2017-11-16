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
#include <dash/dart/if/dart_util.h>


/**
 * The maximum number of elements of a certain type to be
 * transfered in one chunk.
 */
#define MAX_CONTIG_ELEMENTS INT_MAX

typedef enum {
  DART_KIND_BASIC = 0,
  DART_KIND_STRIDED,
  DART_KIND_INDEXED
} dart_type_kind_t;

typedef struct dart_datatype_struct {
  MPI_Datatype         mpi_type;
  dart_datatype_t      base_type;
  dart_type_kind_t     kind;
  MPI_Datatype         max_type;
  union {
    // used for basic types
    struct {
      size_t           size;
    } basic;
    // used for DART_KIND_STRIDED
    struct {
      int              stride;
      int              blocklen;
    } strided;
    // used for DART_KIND_INDEXED
    struct {
      int            * blocklens;
      int            * offsets;
      int              count;
    } indexed;
  };
} dart_datatype_struct_t;

DART_INTERNAL
extern dart_datatype_struct_t __dart_base_types[DART_TYPE_LAST];


dart_ret_t
dart__mpi__datatype_init() DART_INTERNAL;

dart_ret_t
dart__mpi__datatype_fini() DART_INTERNAL;

DART_INLINE MPI_Op dart__mpi__op(dart_operation_t dart_op) {
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

DART_INLINE
dart_datatype_struct_t * dart__mpi__datatype_struct(
  dart_datatype_t dart_datatype)
{
  return (dart_datatype < DART_TYPE_LAST)
            ? &__dart_base_types[dart_datatype]
            : (dart_datatype_struct_t *)dart_datatype;
}

DART_INLINE
MPI_Datatype dart__mpi__datatype(dart_datatype_t dart_datatype) {
  return dart__mpi__datatype_struct(dart_datatype)->mpi_type;
}

DART_INLINE
int dart__mpi__datatype_sizeof(dart_datatype_t dart_type) {
  dart_datatype_struct_t *dts = dart__mpi__datatype_struct(dart_type);
  return (dts->kind == DART_KIND_BASIC) ? dts->basic.size : -1;
}

DART_INLINE
dart_datatype_t dart__mpi__datatype_base(dart_datatype_t dart_type) {
  dart_datatype_struct_t *dts = dart__mpi__datatype_struct(dart_type);
  return (dts->kind == DART_KIND_BASIC) ? dart_type : dts->base_type;
}

DART_INLINE
bool dart__mpi__datatype_isbasic(dart_datatype_t dart_type) {
  return (dart__mpi__datatype_struct(dart_type)->kind == DART_KIND_BASIC);
}

DART_INLINE
bool dart__mpi__datatype_samebase(
  dart_datatype_t lhs_type,
  dart_datatype_t rhs_type) {
  return (
    dart__mpi__datatype_base(lhs_type) == dart__mpi__datatype_base(rhs_type));
}

DART_INLINE
MPI_Datatype dart__mpi__datatype_maxtype(dart_datatype_t dart_type) {
  dart_datatype_struct_t *dts = dart__mpi__datatype_struct(dart_type);
  return dts->max_type;
}

char* dart__mpi__datatype_name(dart_datatype_t dart_type) DART_INTERNAL;


#endif /* DART_ADAPT_COMMUNICATION_PRIV_H_INCLUDED */
