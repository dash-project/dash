#ifndef DASH__UTIL__FUNCTIONAL_EXPR_H__INCLUDED
#define DASH__UTIL__FUNCTIONAL_EXPR_H__INCLUDED

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
 * Compile-time equivalent to `std::multiplies()`
 */
template <typename T>
constexpr T
multiplies(const T x, const T y) {
  return x * y;
}

/**
 * Compile-time equivalent to `std::accumulate()`
 */
template <typename T,         ///< result type
          size_t   N,         ///< length of the array
          typename ReduceOp   ///< binary reduce operation
          >
constexpr T accumulate(
    const std::array<T, N> &arr, ///< array to accumulate
    const size_t first,          ///< starting position of accumulation
    const size_t length,         ///< number of values to accumulate
    const T initialValue,        ///< initial accumulator value
    const ReduceOp & op          ///< binary operation
    ) {
  return (first < (first + length))
             ? op(arr[first],
                  dash::ce::accumulate(
                    arr,
                    first  + 1,   length - 1,
                    initialValue, op))
             : initialValue;
}

/**
 * Compile-time equivalent to `std::inner_product()`.
 */
template <typename T,     ///< result type
          typename T_1,   ///< first array type
          size_t   N_1,   ///< length of the first array
          typename T_2,   ///< second array type
          size_t   N_2,   ///< length of the second array
          typename SumOp, ///< summation operation type
          typename MulOp  ///< multiplication operation type
          >
constexpr T inner_product(
    /// inner product of this array
    const std::array<T_1, N_1> & arr_1,
    /// from this position
    const size_t   first_1,
    /// with this array
    const std::array<T_2, N_2> & arr_2,
    /// from this position
    const size_t   first_2,
    /// using this many elements from both arrays
    const size_t   length,
    /// let this be the summation's initial value
    const T        initialValue,
    /// use this as the summation operator
    const SumOp  & op_sum,
    /// use this as the multiplication operator
    const MulOp & op_prod) {
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

// TODO: Add dash::ce::transform to fmap two instances of std::array.
//       Note, however, that std::array::operator[] is not constexpr
//       in C++11.

} // namespace ce
} // namespace dash

#endif // DASH__UTIL__FUNCTIONAL_EXPR_H__INCLUDED
