#ifndef DASH__INTERNAL__MATH_H_
#define DASH__INTERNAL__MATH_H_

#include <map>
#include <utility>
#include <cmath>


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
  T2 b)
{
  return (a / b) + static_cast<T1>(a % b > 0);
}

template<typename T>
constexpr T max(T a, T b)
{
  return (a > b) ? a : b;
}

template<typename Integer>
std::map<Integer, int> factorize(Integer n)
{
  // Map of factors to their frequency:
  std::map<Integer, int> factors;
  while (n % 2 == 0) {
    n = n / 2;
    auto it = factors.find(2);
    if (it == factors.end()) {
      factors.insert(std::make_pair(2, 1));
    } else {
      it->second++;
    }
  }
  for (int i = 3; i <= std::sqrt(n); i = i + 2) {
    while (n  % i == 0) {
      auto it = factors.find(i);
      if (it == factors.end()) {
        factors.insert(std::make_pair(i, 1));
      } else {
        it->second++;
      }
      n = n/i;
    }
  }
  if (n > 2) {
    auto it = factors.find(n);
    if (it == factors.end()) {
      factors.insert(std::make_pair(n, 1));
    } else {
      it->second++;
    }
  }
  return factors;
}

/**
 * Seed initialization for dash::math::lrand().
 *
 * \see dash::math::lrand
 */
void slrand(unsigned seed = 0);

/**
 * More efficient alternative to std::rand, generates pseudo random
 * number based on logistic map.
 *
 * \see dash::math::slrand
 */
double lrand();

/**
 * Seed initialization for dash::math::xrand().
 *
 * \see dash::math::xrand
 */
void sxrand(unsigned seed = 0);

/**
 * More efficient alternative to std::rand, generates pseudo random
 * number based on XOR shift turns.
 *
 * \see dash::math::sxrand
 */
double xrand();

namespace internal {

double _lrand_f(double r, double x);

} // namespace internal

} // namespace math
} // namespace dash

#endif // DASH__INTERNAL__MATH_H_
