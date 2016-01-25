#ifndef DASH__TYPES_H_
#define DASH__TYPES_H_

#include <array>
#include <dash/dart/if/dart_types.h>

namespace dash {

/**
 * Scalar type for a dimension value, with 0 indicating
 * the first dimension.
 */
typedef int dim_t;

namespace internal {
#ifdef DASH_ENABLE_DEFAULT_INDEX_TYPE_LONG
  typedef int64_t   default_signed_index;
  typedef uint64_t  default_unsigned_index;
#else
  typedef int32_t   default_signed_index;
  typedef uint32_t  default_unsigned_index;
#endif
}

/**
 * Signed integer type used as default for index values.
 */
typedef internal::default_signed_index default_index_t;

/**
 * Unsigned integer type used as default for extent values.
 */
typedef internal::default_unsigned_index default_extent_t;

/**
 * Unsigned integer type used as default for size values.
 */
typedef internal::default_unsigned_index default_size_t;

/**
 * Difference type for global pointers.
 */
typedef internal::default_signed_index gptrdiff_t;

template<
  dash::dim_t NumDimensions,
  typename IndexType = dash::default_index_t>
struct Point {
  ::std::array<IndexType, NumDimensions> coords;
};

template<
  dash::dim_t NumDimensions,
  typename SizeType = dash::default_extent_t>
struct Extent {
  ::std::array<SizeType, NumDimensions> sizes;
};

/**
 * Type traits for mapping to DART data types.
 */
template< typename Type >
struct dart_datatype {
  static const dart_datatype_t value;
};

template<>
struct dart_datatype<int> {
  static const dart_datatype_t value;
};

template<>
struct dart_datatype<float> {
  static const dart_datatype_t value;
};

template<>
struct dart_datatype<double> {
  static const dart_datatype_t value;
};

} // namespace dash

#endif // DASH__TYPES_H_
