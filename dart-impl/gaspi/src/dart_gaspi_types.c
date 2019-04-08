#include <dash/dart/base/logging.h>

#include <dash/dart/gaspi/dart_types.h>

dart_datatype_struct_t dart_base_types[DART_TYPE_LAST];


static void
init_basic_datatype( dart_datatype_t dart_base_type, size_t size)
{
  struct dart_datatype_struct *dart_type = &dart_base_types[dart_base_type];
  dart_type->base_type       = dart_base_type;
  dart_type->kind            = DART_KIND_BASIC;
  dart_type->contiguous.size = 0;
  dart_type->num_elem        = 0;
  if (!DART_TYPE_UNDEFINED) {
    dart_type->contiguous.size = size;

    // basic types only represent a single element
    dart_type->num_elem   = 1;
  }
}

dart_ret_t
datatype_init()
{
  init_basic_datatype(DART_TYPE_UNDEFINED,    0);
  init_basic_datatype(DART_TYPE_BYTE,         sizeof(char));
  init_basic_datatype(DART_TYPE_SHORT,        sizeof(short));
  init_basic_datatype(DART_TYPE_INT,          sizeof(int));
  init_basic_datatype(DART_TYPE_UINT,         sizeof(uint32_t));
  init_basic_datatype(DART_TYPE_LONG,         sizeof(int64_t));
  init_basic_datatype(DART_TYPE_ULONG,        sizeof(uint64_t));
  init_basic_datatype(DART_TYPE_LONGLONG,     sizeof(long long));
  init_basic_datatype(DART_TYPE_ULONGLONG,    sizeof(unsigned long long));
  init_basic_datatype(DART_TYPE_FLOAT,        sizeof(float));
  init_basic_datatype(DART_TYPE_DOUBLE,       sizeof(double));
  init_basic_datatype(DART_TYPE_LONG_DOUBLE,  sizeof(long double));

  return DART_OK;
}

dart_ret_t
datatype_fini()
{
  return DART_OK;
}

dart_ret_t
dart_type_create_strided(
  dart_datatype_t   basetype,
  size_t            stride,
  size_t            blocklen,
  dart_datatype_t * newtype)
{
    if (newtype == NULL) {
        DART_LOG_ERROR("newtype pointer may not be NULL!");
        return DART_ERR_INVAL;
    }

    *newtype = DART_TYPE_UNDEFINED;

    dart_datatype_struct_t* dts_src = get_datatype_struct(basetype);
    if (!datatype_iscontiguous(dts_src)) {
        DART_LOG_ERROR("Only contiguous data types allowed in strided datatypes!");
        return DART_ERR_INVAL;
    }

    dart_datatype_struct_t* strided_type =  malloc(sizeof(struct dart_datatype_struct));
    strided_type->base_type = basetype;
    strided_type->kind      = DART_KIND_STRIDED;
    strided_type->num_elem  = blocklen;
    strided_type->strided.stride = stride;

    *newtype = (dart_datatype_t)strided_type;

    DART_LOG_TRACE("Created new strided data type %p", strided_type);

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
    if (newtype == NULL) {
      DART_LOG_ERROR("newtype pointer may not be NULL!");
      return DART_ERR_INVAL;
    }

    *newtype = DART_TYPE_UNDEFINED;

    dart_datatype_struct_t* dts_src = get_datatype_struct(basetype);
    if (!datatype_iscontiguous(dts_src)) {
      DART_LOG_ERROR("Only contiguous data types allowed in indexed datatypes!");
      return DART_ERR_INVAL;
    }

    size_t* indexed_blocklen = malloc(sizeof(size_t) * count);
    size_t* indexed_disps    = malloc(sizeof(size_t) * count);

    size_t num_elem = 0;
    for (size_t i = 0; i < count; ++i) {
      indexed_blocklen[i] = blocklen[i];
      indexed_disps[i]    = offset[i];
      num_elem       += blocklen[i];
    }

    dart_datatype_struct_t *new_indexed_type;
    new_indexed_type = malloc(sizeof(struct dart_datatype_struct));
    new_indexed_type->base_type = basetype;
    new_indexed_type->kind      = DART_KIND_INDEXED;
    new_indexed_type->num_elem  = num_elem;
    new_indexed_type->indexed.blocklens  = indexed_blocklen;
    new_indexed_type->indexed.offsets    = indexed_disps;
    new_indexed_type->indexed.num_blocks = count;

    *newtype = (dart_datatype_t)new_indexed_type;

    DART_LOG_TRACE("Created new indexed data type %p with %zu elements",
                  new_indexed_type, num_elem);

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

    dart_datatype_struct_t *new_custom_type;
    new_custom_type = malloc(sizeof(struct dart_datatype_struct));

    new_custom_type->base_type           = DART_TYPE_BYTE;
    new_custom_type->kind                = DART_KIND_CUSTOM;
    new_custom_type->num_elem            = 1;
    new_custom_type->contiguous.size     = num_bytes;

    *newtype = (dart_datatype_t)new_custom_type;
    DART_LOG_TRACE("Created new custom data type %p with %zu bytes`",
                  new_custom_type, num_bytes);

    return DART_OK;
}

dart_ret_t
dart_type_destroy(dart_datatype_t* dart_type)
{
    if (dart_type == NULL) {
      return DART_ERR_INVAL;
    }

    dart_datatype_struct_t *dart_type_ptr = get_datatype_struct(*dart_type);

    if (dart_type_ptr->kind == DART_KIND_BASIC) {
      DART_LOG_ERROR("dart_type_destroy: Cannot destroy basic type!");
      return DART_ERR_INVAL;
    }

    if (dart_type_ptr->kind == DART_KIND_INDEXED) {
      free(dart_type_ptr->indexed.blocklens);
      dart_type_ptr->indexed.blocklens = NULL;
      free(dart_type_ptr->indexed.offsets);
      dart_type_ptr->indexed.offsets   = NULL;
    }

    free(dart_type_ptr);
    *dart_type = DART_TYPE_UNDEFINED;

    return DART_OK;
}

dart_ret_t
dart_op_create(
  dart_operator_t    op,
  void             * userdata,
  bool               commute,
  dart_datatype_t    dtype,
  bool               dtype_is_tmp,
  dart_operation_t * new_op)
  {
      return DART_OK;
  }


dart_ret_t
dart_op_destroy(dart_operation_t *op)
{
    return DART_OK;
}