#include <dash/internal/Math.h>

long long dash::math::div_ceil(long long i, long long k) {
  if (i % k == 0)
    return i / k;
  else
    return i / k + 1;
}

