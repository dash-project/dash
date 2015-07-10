#ifndef DASH__INTERNAL__MATH_H_
#define DASH__INTERNAL__MATH_H_

namespace dash {
namespace math {

/**
 * Ceil the quotient of \c a and \c b.
 *
 * \returns  The result of the to-zero division (a/b), incremented
 *           by 1 if the regular division (a/b) has a non-zero
 *           remainder.
 */
template<typename T1, typename T2>
constexpr T1 div_ceil(
  /// Dividend
  T1 a,
  /// Divisor
  T2 b) {
  return (a / b) + static_cast<T1>(a % b > 0);
}

} // namespace math
} // namespace dash

#endif // DASH__INTERNAL__MATH_H_
