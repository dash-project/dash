/**
 * \file dart_mpi_types.c
 *
 * Provide functionality for creating derived data types in DART.
 *
 * Currently implemented: strided types based on basic types.
 */

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_initialization.h>
#include <dash/dart/base/logging.h>
#include <dash/dart/mpi/dart_communication_priv.h>

#include <stdlib.h>
#include <limits.h>
#include <mpi.h>

dart_datatype_struct_t __dart_base_types[DART_TYPE_LAST];

static void
init_basic_datatype(
  dart_datatype_t dart_type_id,
  MPI_Datatype mpi_type)
{
  int size;
  struct dart_datatype_struct *dart_type = &__dart_base_types[dart_type_id];
  dart_type->base_type = DART_TYPE_UNDEFINED;
  dart_type->mpi_type  = mpi_type;
  dart_type->kind = DART_KIND_BASIC;
  int ret = MPI_Type_size(mpi_type, &size);
  if (ret != MPI_SUCCESS) {
    DART_LOG_ERROR("Failed to query size of MPI data type!");
    dart_abort(-1);
  }
  dart_type->basic.size = size;

  if (mpi_type != MPI_DATATYPE_NULL) {
    // create the chunk data type
    MPI_Datatype max_contig_type;
    ret = MPI_Type_contiguous(MAX_CONTIG_ELEMENTS,
                              mpi_type,
                              &max_contig_type);
    if (ret != MPI_SUCCESS) {
      DART_LOG_ERROR("Failed to create chunk type of DART data type");
      dart_abort(-1);
    }
    ret = MPI_Type_commit(&max_contig_type);
    if (ret != MPI_SUCCESS) {
      DART_LOG_ERROR("Failed to commit chunk type of DART data type");
      dart_abort(-1);
    }
    dart_type->basic.max_contig_type = max_contig_type;
  }

}

dart_ret_t
dart__mpi__datatype_init()
{
  init_basic_datatype(DART_TYPE_UNDEFINED, MPI_DATATYPE_NULL);
  init_basic_datatype(DART_TYPE_BYTE, MPI_BYTE);
  init_basic_datatype(DART_TYPE_SHORT, MPI_SHORT);
  init_basic_datatype(DART_TYPE_INT, MPI_INT);
  init_basic_datatype(DART_TYPE_UINT, MPI_UNSIGNED);
  init_basic_datatype(DART_TYPE_LONG, MPI_LONG);
  init_basic_datatype(DART_TYPE_ULONG, MPI_UNSIGNED_LONG);
  init_basic_datatype(DART_TYPE_LONGLONG, MPI_LONG_LONG);
  init_basic_datatype(DART_TYPE_FLOAT, MPI_FLOAT);
  init_basic_datatype(DART_TYPE_DOUBLE, MPI_DOUBLE);

  return DART_OK;
}

dart_ret_t
dart_type_create_strided(
  dart_datatype_t   basetype_id,
  size_t            stride,
  size_t            blocklen,
  dart_datatype_t * newtype)
{
  *newtype = DART_TYPE_UNDEFINED;

  dart_datatype_struct_t *basetype = dart__mpi__datatype_struct(basetype_id);

  if (basetype->kind != DART_KIND_BASIC) {
    DART_LOG_ERROR("Only basic data types allowed in strided datatypes!");
    return DART_ERR_INVAL;
  }

  MPI_Datatype mpi_dtype = basetype->mpi_type;
  MPI_Datatype new_mpi_dtype;
  MPI_Type_vector(1, blocklen, stride, mpi_dtype, &new_mpi_dtype);
  MPI_Type_commit(&new_mpi_dtype);
  dart_datatype_struct_t *new_struct;
  new_struct = malloc(sizeof(struct dart_datatype_struct));
  new_struct->mpi_type         = new_mpi_dtype;
  new_struct->base_type        = basetype_id;
  new_struct->kind             = DART_KIND_STRIDED;
  new_struct->strided.blocklen = blocklen;
  new_struct->strided.stride   = stride;

  *newtype = (dart_datatype_t)new_struct;

  return DART_OK;
}


dart_ret_t
dart_type_create_indexed(
  dart_datatype_t   basetype,
  size_t            count,
  const size_t      blocklen[],
  const size_t      offset[],
  dart_datatype_t * newtype)
{
  *newtype = DART_TYPE_UNDEFINED;

  if (dart__mpi__datatype_struct(basetype)->kind != DART_KIND_BASIC) {
    DART_LOG_ERROR("Only basic data types allowed in indexed datatypes!");
    return DART_ERR_INVAL;
  }

  // check for overflows
  if (count > INT_MAX) {
    DART_LOG_ERROR("dart_type_create_indexed: count > INT_MAX");
    return DART_ERR_INVAL;
  }

  int *mpi_blocklen = malloc(sizeof(int) * count);
  int *mpi_disps    = malloc(sizeof(int) * count);

  for (size_t i = 0; i < count; ++i) {
    if (blocklen[i] > INT_MAX) {
      DART_LOG_ERROR("dart_type_create_indexed: blocklen[%zu] > INT_MAX", i);
      free(mpi_blocklen);
      free(mpi_disps);
      return DART_ERR_INVAL;
    }
    if (offset[i] > INT_MAX) {
      DART_LOG_ERROR("dart_type_create_indexed: offset[%zu] > INT_MAX", i);
      free(mpi_blocklen);
      free(mpi_disps);
      return DART_ERR_INVAL;
    }
    mpi_blocklen[i] = blocklen[i];
    mpi_disps[i]    = offset[i];
  }

  MPI_Datatype mpi_base_type = dart__mpi__datatype_struct(basetype)->mpi_type;
  MPI_Datatype mpi_dtype;
  int ret = MPI_Type_indexed(
              count, mpi_blocklen, mpi_disps, mpi_base_type, &mpi_dtype);
  if (ret != MPI_SUCCESS) {
    DART_LOG_ERROR("dart_type_create_indexed: failed to create indexed type!");
    free(mpi_blocklen);
    free(mpi_disps);
    return DART_ERR_INVAL;
  }

  MPI_Type_commit(&mpi_dtype);
  dart_datatype_struct_t *new_struct;
  new_struct = malloc(sizeof(struct dart_datatype_struct));
  new_struct->base_type = basetype;
  new_struct->mpi_type  = mpi_dtype;
  new_struct->kind      = DART_KIND_INDEXED;
  new_struct->indexed.blocklens = mpi_blocklen;
  new_struct->indexed.offsets   = mpi_disps;

  *newtype = (dart_datatype_t)new_struct;

  return DART_OK;
}

dart_ret_t
dart_type_destroy(dart_datatype_t *dart_type_ptr)
{
  if (dart_type_ptr == NULL) {
    return DART_ERR_INVAL;
  }

  dart_datatype_struct_t *dart_type = dart__mpi__datatype_struct(*dart_type_ptr);

  if (dart_type->kind == DART_KIND_BASIC) {
    DART_LOG_ERROR("dart_type_destroy: Cannot destroy basic type!");
    return DART_ERR_INVAL;
  }

  if (dart_type->kind == DART_KIND_INDEXED) {
    free(dart_type->indexed.blocklens);
    dart_type->indexed.blocklens = NULL;
    free(dart_type->indexed.offsets);
    dart_type->indexed.offsets   = NULL;
  }
  free(dart_type);
  *dart_type_ptr = DART_TYPE_UNDEFINED;

  return DART_OK;
}

static void destroy_basic_type(dart_datatype_t dart_type_id)
{
  dart_datatype_struct_t *dart_type = dart__mpi__datatype_struct(dart_type_id);
  MPI_Type_free(&dart_type->basic.max_contig_type);
  dart_type->basic.max_contig_type = MPI_DATATYPE_NULL;
}

dart_ret_t
dart__mpi__datatype_fini()
{
  destroy_basic_type(DART_TYPE_BYTE);
  destroy_basic_type(DART_TYPE_SHORT);
  destroy_basic_type(DART_TYPE_INT);
  destroy_basic_type(DART_TYPE_UINT);
  destroy_basic_type(DART_TYPE_LONG);
  destroy_basic_type(DART_TYPE_ULONG);
  destroy_basic_type(DART_TYPE_LONGLONG);
  destroy_basic_type(DART_TYPE_FLOAT);
  destroy_basic_type(DART_TYPE_DOUBLE);

  return DART_OK;
}
