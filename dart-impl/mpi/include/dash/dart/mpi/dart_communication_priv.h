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
#include <dash/dart/base/logging.h>
#include <dash/dart/base/assert.h>

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
  /// the underlying data-type (type == base_type for basic types)
  dart_datatype_t      base_type;
  /// the kind of this type (basic, strided, indexed)
  dart_type_kind_t     kind;
  /// the overall number of elements in this type
  size_t               num_elem;
  union {
    /// used for basic types
    struct {
      /// the size in bytes of this type
      size_t           size;
      /// the underlying MPI type
      MPI_Datatype     mpi_type;
      /// the underlying MPI type used to handle large (>2GB) transfers
      MPI_Datatype     max_type;
    } basic;
    /// used for DART_KIND_STRIDED
    /// NOTE: the underlying MPI strided type is created dynamically based on
    ///       the number of blocks required.
    struct {
      /// the stride between blocks of size \c num_elem
      int              stride;
    } strided;
    /// used for DART_KIND_INDEXED
    struct {
      /// the underlying MPI type
      MPI_Datatype     mpi_type;
      /// the numbers of elements in each block
      int            * blocklens;
      /// the offsets at which each block starts
      int            * offsets;
      /// the number of blocks
      int              num_blocks;
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
bool dart__mpi__datatype_isstrided(dart_datatype_t dart_type) {
  return (dart__mpi__datatype_struct(dart_type)->kind == DART_KIND_STRIDED);
}

DART_INLINE
bool dart__mpi__datatype_isindexed(dart_datatype_t dart_type) {
  return (dart__mpi__datatype_struct(dart_type)->kind == DART_KIND_INDEXED);
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
  return (dts->kind == DART_KIND_BASIC) ? dts->basic.max_type
                                        : dart__mpi__datatype_maxtype(
                                            dts->base_type);
}

DART_INLINE
size_t dart__mpi__datatype_num_elem(dart_datatype_t dart_type) {
  return (dart__mpi__datatype_struct(dart_type)->num_elem);
}

MPI_Datatype
dart__mpi__create_strided_mpi(
  dart_datatype_t dart_type,
  size_t          num_blocks) DART_INTERNAL;

void
dart__mpi__destroy_strided_mpi(MPI_Datatype *mpi_type) DART_INTERNAL;

DART_INLINE
void
dart__mpi__datatype_convert_mpi(
  dart_datatype_t  dart_type,
  size_t           dart_num_elem,
  MPI_Datatype   * mpi_type,
  int            * mpi_num_elem)
{
  dart_datatype_struct_t *dts = dart__mpi__datatype_struct(dart_type);
  switch(dts->kind) {
    case DART_KIND_BASIC:
      *mpi_num_elem = dart_num_elem;
      *mpi_type     = dts->basic.mpi_type;
      break;
    case DART_KIND_STRIDED:
      *mpi_num_elem = 1;
      *mpi_type     = dart__mpi__create_strided_mpi(
                                      dart_type, dart_num_elem / dts->num_elem);
      break;
    case DART_KIND_INDEXED:
      *mpi_num_elem = dart_num_elem / dts->num_elem;
      *mpi_type     = dts->indexed.mpi_type;
      break;
    default:
      // should not happen!
      DART_ASSERT_MSG(NULL, "Unknown DART type detected!");
  }
}

char* dart__mpi__datatype_name(dart_datatype_t dart_type) DART_INTERNAL;

/**
 * Helper macro that checks whether the given type is a basic type
 * and errors out in case of an error.
 */

#define CHECK_IS_BASICTYPE(_dtype) \
  do {                                                                        \
    if (dart__unlikely(!dart__mpi__datatype_isbasic(_dtype))) {               \
      char *name = dart__mpi__datatype_name(_dtype);                          \
      DART_LOG_ERROR(                                                         \
                 "%s ! Only basic types allowed in this operation (%s given)",\
                 __FUNCTION__, name);                                         \
      free(name);                                                             \
      return DART_ERR_INVAL;                                                  \
    }                                                                         \
  } while (0)


#endif /* DART_ADAPT_COMMUNICATION_PRIV_H_INCLUDED */
