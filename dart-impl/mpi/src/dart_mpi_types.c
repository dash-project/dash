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
#include <string.h>

#define DART_TYPE_NAMELEN 256

static const char* __dart_base_type_names[DART_TYPE_LAST+1] = {
  "UNDEFINED",
  "BYTE",
  "SHORT",
  "INT",
  "UNSIGNED INT",
  "LONG",
  "UNSIGNED LONG",
  "LONG LONG",
  "UNSIGNED LONG LONG",
  "FLOAT",
  "DOUBLE",
  "LONG DOUBLE",
  "INVALID"
};

dart_datatype_struct_t __dart_base_types[DART_TYPE_LAST];

MPI_Datatype
dart__mpi__datatype_create_max_datatype(MPI_Datatype mpi_type)
{
  MPI_Datatype max_type = MPI_DATATYPE_NULL;
  if (mpi_type != MPI_DATATYPE_NULL) {
    // create the chunk data type
    int ret = MPI_Type_contiguous(MAX_CONTIG_ELEMENTS,
                                  mpi_type,
                                  &max_type);
    if (ret != MPI_SUCCESS) {
      DART_LOG_ERROR("Failed to create chunk type of DART data type");
      dart_abort(-1);
    }
    ret = MPI_Type_commit(&max_type);
    if (ret != MPI_SUCCESS) {
      DART_LOG_ERROR("Failed to commit chunk type of DART data type");
      dart_abort(-1);
    }
  }
  return max_type;
}

static void
init_basic_datatype(
  dart_datatype_t dart_type_id,
  MPI_Datatype mpi_type)
{
  int size;
  struct dart_datatype_struct *dart_type = &__dart_base_types[dart_type_id];
  dart_type->base_type       = dart_type_id;
  dart_type->contiguous.mpi_type  = mpi_type;
  dart_type->kind            = DART_KIND_BASIC;
  dart_type->contiguous.size = 0;
  dart_type->num_elem        = 0;
  if (mpi_type != MPI_DATATYPE_NULL) {
    int ret = MPI_Type_size(mpi_type, &size);
    if (ret != MPI_SUCCESS) {
      DART_LOG_ERROR("Failed to query size of MPI data type!");
      dart_abort(-1);
    }
    dart_type->contiguous.size = size;

    // basic types only represent a single element
    dart_type->num_elem   = 1;

    // create the type used for large transfers
    dart_type->contiguous.max_type =
                              dart__mpi__datatype_create_max_datatype(mpi_type);
  }
}

dart_ret_t
dart__mpi__datatype_init()
{
  init_basic_datatype(DART_TYPE_UNDEFINED,    MPI_DATATYPE_NULL);
  init_basic_datatype(DART_TYPE_BYTE,         MPI_BYTE);
  init_basic_datatype(DART_TYPE_SHORT,        MPI_SHORT);
  init_basic_datatype(DART_TYPE_INT,          MPI_INT);
  init_basic_datatype(DART_TYPE_UINT,         MPI_UNSIGNED);
  init_basic_datatype(DART_TYPE_LONG,         MPI_LONG);
  init_basic_datatype(DART_TYPE_ULONG,        MPI_UNSIGNED_LONG);
  init_basic_datatype(DART_TYPE_LONGLONG,     MPI_LONG_LONG);
  init_basic_datatype(DART_TYPE_ULONGLONG,    MPI_UNSIGNED_LONG_LONG);
  init_basic_datatype(DART_TYPE_FLOAT,        MPI_FLOAT);
  init_basic_datatype(DART_TYPE_DOUBLE,       MPI_DOUBLE);
  init_basic_datatype(DART_TYPE_LONG_DOUBLE,  MPI_LONG_DOUBLE);

  return DART_OK;
}

char* dart__mpi__datatype_name(dart_datatype_t dart_type)
{
  char *buf = NULL;
  if (dart_type <= DART_TYPE_LAST) {
    buf = strdup(__dart_base_type_names[dart_type]);
  } else {
    dart_datatype_struct_t *dts = dart__mpi__datatype_struct(dart_type);
    if (dts->kind == DART_KIND_INDEXED) {
      buf = malloc(DART_TYPE_NAMELEN);
      char *base_name = dart__mpi__datatype_name(dts->base_type);
      snprintf(buf, DART_TYPE_NAMELEN, "INDEXED(%i:%s)",
                dts->indexed.num_blocks, base_name);
      free(base_name);
    } else if (dts->kind == DART_KIND_STRIDED){
      buf = malloc(DART_TYPE_NAMELEN);
      char *base_name = dart__mpi__datatype_name(dts->base_type);
      snprintf(buf, DART_TYPE_NAMELEN, "STRIDED(%zu:%i:%s)",
                dts->num_elem, dts->strided.stride, base_name);
      free(base_name);
    } else if (dts->kind == DART_KIND_CUSTOM) {
      buf = malloc(DART_TYPE_NAMELEN);
      char *base_name = dart__mpi__datatype_name(dts->base_type);
      snprintf(buf, DART_TYPE_NAMELEN, "CUSTOM(%zu:%s)",
                dts->contiguous.size, base_name);
      free(base_name);
    } else {
      DART_LOG_ERROR("INVALID data type detected!");
    }
  }
  return buf;
}

dart_ret_t
dart_type_create_strided(
  dart_datatype_t   basetype_id,
  size_t            stride,
  size_t            blocklen,
  dart_datatype_t * newtype)
{

  if (newtype == NULL) {
    DART_LOG_ERROR("newtype pointer may not be NULL!");
    return DART_ERR_INVAL;
  }

  *newtype = DART_TYPE_UNDEFINED;

  if (!dart__mpi__datatype_iscontiguous(basetype_id)) {
    DART_LOG_ERROR("Only contiguous data types allowed in strided datatypes!");
    return DART_ERR_INVAL;
  }

  if (stride > INT_MAX || blocklen > INT_MAX) {
    DART_LOG_ERROR("dart_type_create_strided: arguments out of range (>INT_MAX)");
    return DART_ERR_INVAL;
  }

  //MPI_Datatype new_mpi_dtype;
  //MPI_Type_vector(num_blocks, blocklen, stride, mpi_dtype, &new_mpi_dtype);
  //MPI_Type_commit(&new_mpi_dtype);
  dart_datatype_struct_t *new_struct;
  new_struct = malloc(sizeof(struct dart_datatype_struct));
  new_struct->base_type        = basetype_id;
  new_struct->kind             = DART_KIND_STRIDED;
  new_struct->num_elem         = blocklen;
  new_struct->strided.stride   = stride;

  *newtype = (dart_datatype_t)new_struct;

  DART_LOG_TRACE("Created new strided data type %p", new_struct);

  return DART_OK;
}


MPI_Datatype
dart__mpi__create_strided_mpi(
  dart_datatype_t dart_type,
  size_t          num_blocks)
{
  MPI_Datatype new_mpi_dtype;
  dart_datatype_struct_t *dts = dart__mpi__datatype_struct(dart_type);
  MPI_Type_vector(
    num_blocks,             // the number of blocks
    dts->num_elem,          // the number of elements per block
    dts->strided.stride,    // the number of elements between start of each block
    dart__mpi__datatype_struct(dts->base_type)->contiguous.mpi_type,
    &new_mpi_dtype);
  MPI_Type_commit(&new_mpi_dtype);
  return new_mpi_dtype;
}

void
dart__mpi__destroy_strided_mpi(MPI_Datatype *mpi_type)
{
  MPI_Type_free(mpi_type);
}

dart_ret_t
dart_type_create_indexed(
  dart_datatype_t   basetype,
  size_t            count,
  const size_t      blocklen[],
  const size_t      offset[],
  dart_datatype_t * newtype)
{
  if (newtype == NULL) {
    DART_LOG_ERROR("newtype pointer may not be NULL!");
    return DART_ERR_INVAL;
  }

  *newtype = DART_TYPE_UNDEFINED;
  dart_datatype_struct_t *basetype_struct = dart__mpi__datatype_struct(basetype);
  if (!dart__mpi__datatype_iscontiguous(basetype)) {
    DART_LOG_ERROR("Only contiguous data types allowed in indexed datatypes!");
    return DART_ERR_INVAL;
  }

  // check for overflows
  if (count > INT_MAX) {
    DART_LOG_ERROR("dart_type_create_indexed: count > INT_MAX");
    return DART_ERR_INVAL;
  }

  int *mpi_blocklen = malloc(sizeof(int) * count);
  int *mpi_disps    = malloc(sizeof(int) * count);

  size_t num_elem = 0;
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
    num_elem       += blocklen[i];
  }

  MPI_Datatype mpi_base_type = basetype_struct->contiguous.mpi_type;
  MPI_Datatype new_mpi_dtype;
  int ret = MPI_Type_indexed(
              count, mpi_blocklen, mpi_disps, mpi_base_type, &new_mpi_dtype);
  if (ret != MPI_SUCCESS) {
    DART_LOG_ERROR("dart_type_create_indexed: failed to create indexed type!");
    free(mpi_blocklen);
    free(mpi_disps);
    return DART_ERR_INVAL;
  }

  MPI_Type_commit(&new_mpi_dtype);
  dart_datatype_struct_t *new_struct;
  new_struct = malloc(sizeof(struct dart_datatype_struct));
  new_struct->base_type = basetype;
  new_struct->kind      = DART_KIND_INDEXED;
  new_struct->num_elem  = num_elem;
  new_struct->indexed.mpi_type   = new_mpi_dtype;
  new_struct->indexed.blocklens  = mpi_blocklen;
  new_struct->indexed.offsets    = mpi_disps;
  new_struct->indexed.num_blocks = count;

  *newtype = (dart_datatype_t)new_struct;

  DART_LOG_TRACE("Created new indexed data type %p with %zu elements",
                 new_struct, num_elem);

  return DART_OK;
}

dart_ret_t
dart_type_create_custom(
  size_t            num_bytes,
  dart_datatype_t * newtype)
{
  if (newtype == NULL) {
    DART_LOG_ERROR("newtype pointer may not be NULL!");
    return DART_ERR_INVAL;
  }

  *newtype = DART_TYPE_UNDEFINED;

  if (num_bytes > INT_MAX) {
    DART_LOG_ERROR("Custom types larger than 2GB not supported by MPI!");
    return DART_ERR_INVAL;
  }

  MPI_Datatype new_mpi_dtype;
  MPI_Type_contiguous(num_bytes, MPI_BYTE, &new_mpi_dtype);
  MPI_Type_commit(&new_mpi_dtype);

  dart_datatype_struct_t *new_struct;
  new_struct = malloc(sizeof(struct dart_datatype_struct));

  new_struct->base_type           = DART_TYPE_BYTE;
  new_struct->kind                = DART_KIND_CUSTOM;
  new_struct->num_elem            = 1;
  new_struct->contiguous.size     = num_bytes;
  new_struct->contiguous.mpi_type = new_mpi_dtype;
  // max_type will be created on-demand for custom types
  new_struct->contiguous.max_type = DART_MPI_TYPE_UNDEFINED;

  *newtype = (dart_datatype_t)new_struct;
  DART_LOG_TRACE("Created new custom data type %p with %zu bytes`",
                 new_struct, num_bytes);

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
    MPI_Type_free(&dart_type->indexed.mpi_type);
  } else if (dart_type->kind == DART_KIND_CUSTOM) {
    MPI_Type_free(&dart_type->contiguous.mpi_type);
    if (dart_type->contiguous.max_type != DART_MPI_TYPE_UNDEFINED) {
      MPI_Type_free(&dart_type->contiguous.max_type);
    }
  }

  free(dart_type);
  *dart_type_ptr = DART_TYPE_UNDEFINED;

  return DART_OK;
}

static void destroy_basic_type(dart_datatype_t dart_type_id)
{
  dart_datatype_struct_t *dart_type = dart__mpi__datatype_struct(dart_type_id);
  MPI_Type_free(&dart_type->contiguous.max_type);
  dart_type->contiguous.max_type = MPI_DATATYPE_NULL;
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
  destroy_basic_type(DART_TYPE_ULONGLONG);
  destroy_basic_type(DART_TYPE_FLOAT);
  destroy_basic_type(DART_TYPE_DOUBLE);
  destroy_basic_type(DART_TYPE_LONG_DOUBLE);

  return DART_OK;
}
