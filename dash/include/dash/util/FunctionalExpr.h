#ifndef DASH__UTIL__FUNCTIONAL_EXPR_H__INCLUDED
#define DASH__UTIL__FUNCTIONAL_EXPR_H__INCLUDED

#include <dash/util/IndexSequence.h>

#include <functional>
#include <type_traits>


namespace dash {
namespace ce {

template <typename T, typename... Ts>
struct are_integral
    : std::integral_constant<
        bool,
        std::is_integral<T>::value && are_integral<Ts...>::value> {};

template <typename T>
struct are_integral<T> : std::is_integral<T> { };

/**
 * Compile-time equivalent to `std::plus()`
 */
template <typename T>
constexpr T
plus(const T x, const T y) {
  return x + y;
}

/**
 * Compile-time equivalent to `std::minus()`
 */
template <typename T>
constexpr T
minus(const T x, const T y) {
  return x - y;
}

/**
 * Compile-time equivalent to `std::multiplies()`
 */
template <typename T>
constexpr T
multiplies(const T x, const T y) {
  return x * y;
}

/**
 * Compile-time equivalent to `std::divides()`
 */
template <typename T>
constexpr T
divides(const T x, const T y) {
  return x / y;
}

/**
 * Compile-time equivalent to `std::pow()` for integral data types
 */
template <typename BaseT, typename ExpT>
constexpr BaseT pow(BaseT base, ExpT exp) {
  static_assert(std::is_integral<BaseT>::value, "Base must be an integer.");
  static_assert(std::is_integral<ExpT>::value && std::is_unsigned<ExpT>::value,
                "Exponent must be an unsigned integer.");

  return (exp == 0 ? 1 : base * pow(base, exp - 1));
}

/**
 * Compile-time equivalent to `std::accumulate()`
 */
template <typename T,         ///< result type
          size_t   N,         ///< array length
          typename ReduceOp > ///< binary reduce operation
constexpr T accumulate(
    const std::array<T, N> &arr, ///< array to accumulate
    const size_t first_idx,      ///< start index for accumulation
    const size_t final_idx,      ///< index past last element to accumulate
    const T initialValue,        ///< initial accumulation value
    const ReduceOp & op          ///< binary operation
  ) {
  return (first_idx < final_idx)
             ? op(arr[first_idx],
                  dash::ce::accumulate(
                    arr,
                    first_idx  + 1, final_idx,
                    initialValue,
                    op))
             : initialValue;
}

/**
 * Compile-time equivalent to `std::inner_product()`.
 */
template <typename T,       ///< result type
          typename T_1,     ///< first array type
          size_t   N_1,     ///< size of first array
          typename T_2,     ///< second array type
          size_t   N_2,     ///< size of second array
          typename SumOp,   ///< summation operation type
          typename MulOp    ///< multiplication operation type
>
constexpr T inner_product(
    /// array to calculate inner product
    const std::array<T_1, N_1> & arr_1,
    /// start index in first array
    const size_t                 first_1,
    /// second array
    const std::array<T_2, N_2> & arr_2,
    /// start index in second array
    const size_t                 first_2,
    /// number of elements to use in both arrays
    const size_t                 length,
    /// summation initial value
    const T                      initialValue,
    /// summation operator
    const SumOp                & op_sum,
    /// multiplication operator
    const MulOp                & op_prod) {
  return (first_1 < (first_1 + length))
             ? op_sum(op_prod(
                        arr_1[first_1],
                        arr_2[first_2]),
                      dash::ce::inner_product(
                        arr_1, first_1 + 1,
                        arr_2, first_2 + 1,
                        length - 1,
                        initialValue,
                        op_sum,
                        op_prod))
             : initialValue;
}

#if 0
/**
 * Compile-time equivalent to `std::transform()`.
 */
template <typename T_1,   ///< first array value type
          size_t   N_1,   ///< size of first array
          typename T_2,   ///< second array value type
          size_t   N_2,   ///< size of second array
          typename MapOp  ///< reduce operation
          >
constexpr std::array<T_1, N> transform(
    /// first input array
    const std::array<T_1, N_1> & arr_1,
    /// start index in first array
    const size_t                 first_1,
    /// second input array
    const std::array<T_2, N_2> & arr_2,
    /// start index in second array
    const size_t                 first_2) {
  return ...
}
#endif

} // namespace ce
} // namespace dash

#endif // DASH__UTIL__FUNCTIONAL_EXPR_H__INCLUDED
