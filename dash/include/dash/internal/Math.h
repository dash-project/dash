#ifndef DASH__INTERNAL__MATH_H_
#define DASH__INTERNAL__MATH_H_

#include <dash/internal/Logging.h>

#include <map>
#include <set>
#include <array>
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
  Integer sqrt_n = std::ceil(std::sqrt(n));
  for (int i = 3; i <= sqrt_n; i = i + 2) {
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
    auto it = primes.find(2);
    if (it == primes.end()) {
      primes.insert(2);
    } else {
      it->second++;
    }
  }
  Integer sqrt_n = std::ceil(std::sqrt(n));
  for (int i = 3; i <= sqrt_n; i = i + 2) {
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

template<typename Integer, size_t NumDim>
typename std::enable_if<(NumDim > 1), std::array<Integer, NumDim> >::type
balance_extents(
  std::array<Integer, NumDim> extents)
{
  Integer size = 1;
  for (auto d = 0; d < NumDim; ++d) {
    size *= extents[d];
  }
  extents[0]      = size;
  extents[1]      = 1;
  auto factors    = dash::math::factorize(size);
  Integer surface = 0;

  DASH_LOG_TRACE_VAR("dash::math::balance_extents", factors);
  for (auto it : factors) {
    DASH_LOG_TRACE("dash::math::balance_extents",
                   "factor:", it.first, "x", it.second);
    for (auto i = 1; i < it.second + 1; ++i) {
      Integer extent_x    = it.first * i;
      Integer extent_y    = size / extent_x;
      Integer surface_new = (2 * extent_x) + (2 * extent_y);
      DASH_LOG_TRACE("dash::math::balance_extents", "testing extents",
                     extent_x, "x", extent_y, " -> surface:", surface_new);
      if (surface == 0 || surface_new < surface) {
        surface    = surface_new;
        extents[0] = extent_x;
        extents[1] = extent_y;
      }
    }
  }
  return extents;
}

template<typename Integer, size_t NumDim>
typename std::enable_if<(NumDim > 1), std::array<Integer, NumDim> >::type
balance_extents(
  std::array<Integer, NumDim> extents,
  // List of preferred blocking factors.
  std::set<Integer>           blocking)
{
  Integer size = 1;
  for (auto d = 0; d < NumDim; ++d) {
    size *= extents[d];
  }
  extents[0]        = size;
  extents[1]        = 1;
  auto size_factors = dash::math::factorize(size);
  Integer surface   = 0;

  DASH_LOG_TRACE_VAR("dash::math::balance_extents", size_factors);

  // Test if size is divisible by blocking factors:
  for (auto block_size : blocking) {
    if (size % block_size == 0) {
      auto n_blocks = size / block_size;
      // Size can be partitioned into n_blocks of size block_factor:
      DASH_LOG_TRACE("dash::math::balance_extents",
                     "blocking factor matched:", block_size,
                     "(blocks:", n_blocks, ")");
      Integer extent_x    = block_size;
      Integer extent_y    = size / extent_x;
      Integer surface_new = (2 * extent_x) + (2 * extent_y);
      DASH_LOG_TRACE("dash::math::balance_extents", "testing extents",
                     extent_x, "x", extent_y, " -> surface:", surface_new);
      if (surface == 0 || surface_new < surface) {
        surface    = surface_new;
        extents[0] = extent_x;
        extents[1] = extent_y;
      }
    }
  }
  return extents;
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
