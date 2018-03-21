#ifndef DASH__TYPES_H_
#define DASH__TYPES_H_

#include <array>
#include <type_traits>

#include <dash/dart/if/dart_types.h>
#include <dash/internal/Unit.h>


namespace dash {

typedef enum MemArrange {
  MEM_ARRANGE_UNDEFINED = 0,
  ROW_MAJOR,
  COL_MAJOR
} MemArrange;

namespace internal {

typedef enum DistributionType {
  DIST_UNDEFINED = 0,
  DIST_NONE,
  DIST_BLOCKED,      // = BLOCKCYCLIC(ceil(nelem/nunits))
  DIST_CYCLIC,       // = BLOCKCYCLIC(1) Will be removed
  DIST_BLOCKCYCLIC,
  DIST_TILE
} DistributionType; // general blocked distribution

} // namespace internal

/**
 * Scalar type for a dimension value, with 0 indicating
 * the first dimension.
 */
typedef int dim_t;

namespace internal {

#if defined(DASH_ENABLE_DEFAULT_INDEX_TYPE_LONG)
  typedef          long default_signed_index;
  typedef unsigned long default_unsigned_index;
#elif defined(DASH_ENABLE_DEFAULT_INDEX_TYPE_INT)
  typedef          int  default_signed_index;
  typedef unsigned int  default_unsigned_index;
#else
  typedef ssize_t       default_signed_index;
  typedef size_t        default_unsigned_index;
#endif

} // namespace internal

/**
 * Signed integer type used as default for index values.
 */
typedef internal::default_signed_index    default_index_t;

/**
 * Unsigned integer type used as default for extent values.
 */
typedef internal::default_unsigned_index default_extent_t;

/**
 * Unsigned integer type used as default for size values.
 */
typedef internal::default_unsigned_index   default_size_t;

/**
 * Difference type for global pointers.
 */
typedef internal::default_signed_index         gptrdiff_t;

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

#ifdef DOXYGEN

/**
 * Type trait for mapping to DART data types.
 */
template<typename Type>
struct dart_datatype {
  static constexpr const dart_datatype_t value;
};

/**
 * Type trait for mapping to punned DART data type for reduce operations.
 */
template <typename T>
struct dart_punned_datatype {
  static constexpr const dart_datatype_t value;
};

#else

template<typename Type>
struct dart_datatype {
  static constexpr const dart_datatype_t value = DART_TYPE_UNDEFINED;
};

template<>
struct dart_datatype<char> {
  static constexpr const dart_datatype_t value = DART_TYPE_BYTE;
};

template<>
struct dart_datatype<unsigned char> {
  static constexpr const dart_datatype_t value = DART_TYPE_BYTE;
};

template<>
struct dart_datatype<short> {
  static constexpr const dart_datatype_t value = DART_TYPE_SHORT;
};

template<>
struct dart_datatype<unsigned short> {
  static constexpr const dart_datatype_t value = DART_TYPE_SHORT;
};

template<>
struct dart_datatype<int> {
  static constexpr const dart_datatype_t value = DART_TYPE_INT;
};

template<>
struct dart_datatype<unsigned int> {
  static constexpr const dart_datatype_t value = DART_TYPE_UINT;
};

template<>
struct dart_datatype<long> {
  static constexpr const dart_datatype_t value = DART_TYPE_LONG;
};

template<>
struct dart_datatype<unsigned long> {
  static constexpr const dart_datatype_t value = DART_TYPE_ULONG;
};

template<>
struct dart_datatype<long long> {
  static constexpr const dart_datatype_t value = DART_TYPE_LONGLONG;
};

template<>
struct dart_datatype<unsigned long long> {
  static constexpr const dart_datatype_t value = DART_TYPE_ULONGLONG;
};

template<>
struct dart_datatype<float> {
  static constexpr const dart_datatype_t value = DART_TYPE_FLOAT;
};

template<>
struct dart_datatype<double> {
  static constexpr const dart_datatype_t value = DART_TYPE_DOUBLE;
};

template<>
struct dart_datatype<long double> {
  static constexpr const dart_datatype_t value = DART_TYPE_LONG_DOUBLE;
};

template<typename T>
struct dart_datatype<const    T> : public dart_datatype<T> { };

template<typename T>
struct dart_datatype<volatile T> : public dart_datatype<T> { };


namespace internal {

template <std::size_t Size>
struct dart_pun_datatype_size
: public std::integral_constant<dart_datatype_t, DART_TYPE_UNDEFINED>
{ };

template <>
struct dart_pun_datatype_size<1>
: public std::integral_constant<dart_datatype_t, DART_TYPE_BYTE>
{ };

template <>
struct dart_pun_datatype_size<2>
: public std::integral_constant<dart_datatype_t, DART_TYPE_SHORT>
{ };

template <>
struct dart_pun_datatype_size<4>
: public std::integral_constant<dart_datatype_t, DART_TYPE_INT>
{ };

template <>
struct dart_pun_datatype_size<8>
: public std::integral_constant<dart_datatype_t, DART_TYPE_LONGLONG>
{ };

} // namespace internal

template <typename T>
struct dart_punned_datatype {
  static constexpr const dart_datatype_t value
                           = std::conditional<
                               // only use type punning if T is not a DART
                               // data type:
                               dash::dart_datatype<T>::value
                                 == DART_TYPE_UNDEFINED,
                               internal::dart_pun_datatype_size<sizeof(T)>,
                               dash::dart_datatype<T>
                             >::type::value;
};

#endif // DOXYGEN

/**
 * Type trait indicating whether the specified type is eligible for
 * elements of DASH containers.
 */
template <class T>
struct is_container_compatible :
  public std::integral_constant<bool,
              std::is_standard_layout<T>::value
#ifdef DASH_HAVE_STD_TRIVIALLY_COPYABLE
              && std::is_trivially_copyable<T>::value
#elif defined DASH_HAVE_TRIVIAL_COPY_INTRINSIC
              && __has_trivial_copy(T)
#endif
         >
{ };

/**
 * Type trait indicating whether a type can be used for global atomic
 * operations.
 */
template <typename T>
struct is_atomic_compatible
: public std::integral_constant<bool, std::is_arithmetic<T>::value>
{ };

/**
 * Type trait indicating whether a type can be used for arithmetic
 * operations in global memory space.
 */
template <typename T>
struct is_arithmetic
: public std::integral_constant<
           bool,
           dash::dart_datatype<T>::value != DART_TYPE_UNDEFINED >
{ };

/**
 * Type trait indicating whether a type has a comparision operator==
 * defined.
 * \code
 * bool test = has_operator_equal<MyType>::value;
 * bool test = has_operator_equal<MyType, int>::value;
 * \endcode
 */
template<class T, class EqualTo>
struct has_operator_equal_impl
{
    template<class U, class V>
    static auto test(U*) -> decltype(std::declval<U>() == std::declval<V>());
    template<typename, typename>
    static auto test(...) -> std::false_type;

    using type = typename std::is_same<bool, decltype(test<T, EqualTo>(0))>::type;
};

template<class T, class EqualTo = T>
struct has_operator_equal : has_operator_equal_impl<T, EqualTo>::type {};

/**
 * Convencience wrapper to determine the DART type and number of elements
 * required for the given template parameter type \c T and the desired number of
 * values \c nvalues.
 */
template<typename T>
struct dart_storage {
                   const size_t          nelem;
  static constexpr const dart_datatype_t dtype =
                                (dart_datatype<T>::value == DART_TYPE_UNDEFINED)
                                  ? DART_TYPE_BYTE : dart_datatype<T>::value;

  constexpr
  dart_storage(size_t nvalues) noexcept
  : nelem(
      (dart_datatype<T>::value == DART_TYPE_UNDEFINED)
      ? nvalues * sizeof(T) : nvalues)
  { }
};

/**
 * Unit ID to use for team-local IDs.
 *
 * Note that this is returned by calls to dash::Team::myid(),
 * including \c dash::Team::All().myid() as it is handled as
 * a team as well.
 *
 * \see unit_id
 * \see global_unit_t
 */
typedef struct
dash::unit_id<dash::local_unit, dart_team_unit_t>
team_unit_t;

/**
 * Unit ID to use for global IDs.
 *
 * Note that this typed is returned by \c dash::myid()
 * and \c dash::Team::GlobalUnitID().
 *
 * \see unit_id
 * \see team_unit_t
 */
typedef struct
dash::unit_id<dash::global_unit, dart_global_unit_t>
global_unit_t;

/**
 * Invalid local unit ID.
 *
 * This is a typed version of \ref DART_UNDEFINED_UNIT_ID.
 */
constexpr team_unit_t   UNDEFINED_TEAM_UNIT_ID{DART_UNDEFINED_UNIT_ID};

/**
 * Invalid global unit ID.
 *
 * This is a typed version of \ref DART_UNDEFINED_UNIT_ID.
 */
constexpr global_unit_t UNDEFINED_GLOBAL_UNIT_ID{DART_UNDEFINED_UNIT_ID};

} // namespace dash

#endif // DASH__TYPES_H_
