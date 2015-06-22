#ifndef DASH__INTERNAL__MATH_H_
#define DASH__INTERNAL__MATH_H_

namespace dash {
namespace math {

/**
 * TODO: Fix unsafe arithmetics
 */
static long long div_ceil(long long i, long long k) {
  if (i % k == 0)
    return i / k;
  else
    return i / k + 1;
}

} // namespace math
} // namespace dash

#endif // DASH__INTERNAL__MATH_H_
