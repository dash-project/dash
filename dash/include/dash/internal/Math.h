#ifndef DASH__INTERNAL__MATH_H_
#define DASH__INTERNAL__MATH_H_

#include <map>
#include <set>
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

template<
  typename Iter>
double sigma(
  const Iter & begin,
  const Iter & end)
{
  auto   n      = std::distance(begin, end);
  double mean   = 0;
  double devsum = 0;
  double sum    = 0;
  double var    = 0;
  for (auto it = begin; it != end; ++it) {
    sum += *it;
  }
  mean = sum / n;
  for (auto it = begin; it != end; ++it) {
    auto diff = *it - mean;
    devsum += diff * diff;
  }
  var = devsum / n;
  return sqrt(var);
}

/**
 * Factorize a given integer.
 * Returns a map of prime factors to their frequency.
 *
 * Example:
 *
 * \code
 *   long number  = 2 * 2 * 2 * 4 * 7 * 7;
 *   auto factors = dash::math::factorize(number);
 *   // returns map { (2,3), (4,1), (7,2) }
 * \endcode
 */
template<typename Integer>
std::map<Integer, int> factorize(Integer n)
{
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
 * Obtains prime factors of a given integer.
 * Returns sorted set of primes.
 *
 * Example:
 *
 * \code
 *   long number  = 2 * 2 * 2 * 4 * 7 * 7;
 *   auto factors = dash::math::factors(number);
 *   // returns set { 2, 4, 7 }
 * \endcode
 */
template<typename Integer>
std::set<Integer> factors(Integer n)
{
  std::set<Integer> primes;
  // In HPC applications, we assume (multiples of) powers of 2 as the
  // most common case:
  while (n % 2 == 0) {
    n = n / 2;
    auto it = factors.find(2);
    if (it == factors.end()) {
      primes.insert(2);
    } else {
      it->second++;
    }
  }
  for (int i = 3; i <= std::sqrt(n); i = i + 2) {
    while (n  % i == 0) {
      auto it = primes.find(i);
      if (it == primes.end()) {
        primes.insert(i);
      }
      n = n/i;
    }
  }
  if (n > 2) {
    // Check if the remainder is prime:
    auto it = primes.find(n);
    if (it == primes.end()) {
      primes.insert(n);
    }
  }
  return primes;
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
