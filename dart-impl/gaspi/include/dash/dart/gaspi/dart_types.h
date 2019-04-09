#ifndef DART_GASPI_TYPES_H_INCLUDED
#define DART_GASPI_TYPES_H_INCLUDED

#include <stdbool.h>

#include <dash/dart/if/dart_types.h>

typedef enum {
  DART_KIND_BASIC = 0,
  DART_KIND_STRIDED,
  DART_KIND_INDEXED,
  DART_KIND_CUSTOM
} dart_type_kind_t;


typedef struct dart_datatype_struct {
  /// the underlying data-type (type == base_type for basic types)
  dart_datatype_t      base_type;
  /// the kind of this type (basic, strided, indexed)
  dart_type_kind_t     kind;
  /// the overall number of elements in this type
  size_t               num_elem;
  union {
    /// used for contiguous (basic & custom) types
    struct {
      /// the size in bytes of this type
      size_t           size;
    } contiguous;
    /// used for DART_KIND_STRIDED
    struct {
      /// the stride between blocks of size \c num_elem
      int              stride;
    } strided;
    /// used for DART_KIND_INDEXED
    struct {
      /// the numbers of elements in each block
      size_t *         blocklens;
      /// the offsets at which each block starts
      size_t *         offsets;
      /// the number of blocks
      size_t           num_blocks;
    } indexed;
  };
} dart_datatype_struct_t;

extern dart_datatype_struct_t dart_base_types[DART_TYPE_LAST];

dart_ret_t
datatype_init();

dart_ret_t
datatype_fini();


static inline
dart_datatype_struct_t * get_datatype_struct(
  dart_datatype_t dart_datatype)
{
  if(dart_datatype < DART_TYPE_LAST)
      return &dart_base_types[dart_datatype];

  return (dart_datatype_struct_t *)dart_datatype;
}

static inline
dart_datatype_t datatype_base(dart_datatype_struct_t* dts) {
  return dts->base_type;
}

static inline
dart_datatype_struct_t* datatype_base_struct(dart_datatype_struct_t* dts) {
  return (dts->kind == DART_KIND_BASIC) ? dts
                                        : get_datatype_struct(dts->base_type);
}

static inline
bool datatype_isbasic(dart_datatype_struct_t* dts) {
  return (dts->kind == DART_KIND_BASIC);
}


static inline
bool datatype_iscontiguous(dart_datatype_struct_t* dts) {
  return (dts->kind == DART_KIND_BASIC || dts->kind == DART_KIND_CUSTOM);
}

static inline
bool datatype_isstrided(dart_datatype_struct_t* dts) {
  return (dts->kind == DART_KIND_STRIDED);
}

static inline
bool datatype_isindexed(dart_datatype_struct_t* dts) {
  return (dts->kind == DART_KIND_INDEXED);
}

static inline
size_t datatype_sizeof(dart_datatype_struct_t* dts) {
  if(datatype_iscontiguous(dts))
      return dts->contiguous.size;

  return -1;
}

static inline
bool datatype_samebase(
  dart_datatype_struct_t* lhs_type,
  dart_datatype_struct_t* rhs_type)
{
  return (datatype_base(lhs_type) == datatype_base(rhs_type));
}

static inline
size_t datatype_num_elem(dart_datatype_struct_t* dts) {
  return dts->num_elem;
}

#endif /* DART_GASPI_TYPES_H_INCLUDED */
