#include <dash/internal/Math.h>

namespace dash {
namespace math {

static const double _lrand_r_min = 3.0;
static const double _lrand_r_max = 4.0;
static double _lrand_r    = _lrand_r_min;
static double _lrand_x_n  = 0;
static double _lrand_unit = 0;

static unsigned long _xrand_x = 123456789;
static unsigned long _xrand_y = 362436069;
static unsigned long _xrand_z = 521288629;

namespace internal {

double _lrand_f(double r, double x)
{
  return (r * x * (1.0-x));
}

} // namespace internal

void slrand(unsigned seed)
{
  std::srand(seed || time(NULL));
  _lrand_unit = static_cast<double>(std::rand() / RAND_MAX);
  _lrand_x_n  = internal::_lrand_f(_lrand_r, _lrand_unit);
}

double lrand()
{
  const int num_intervals  = 10;
  const int max_iterations = 12;

  for (int i = 0; i < num_intervals; ++i) {
    _lrand_r   += (_lrand_r_max - _lrand_r_min) / num_intervals;
    _lrand_x_n  = internal::_lrand_f(_lrand_r, _lrand_unit);
    for(int j = 0; j < max_iterations; ++j) {
      _lrand_x_n = internal::_lrand_f(_lrand_r, _lrand_x_n);
    }
  }
  return _lrand_x_n;
}

void sxrand(unsigned seed)
{
  std::srand(seed || time(NULL));
  unsigned long rseed = static_cast<unsigned long>(std::rand() / RAND_MAX);
  _xrand_x ^= rseed;
}

double xrand()
{
  unsigned long t;
  double        r;

  _xrand_x ^= _xrand_x << 16;
  _xrand_x ^= _xrand_x >> 5;
  _xrand_x ^= _xrand_x << 1;
  t         = _xrand_x;
  _xrand_x  = _xrand_y;
  _xrand_y  = _xrand_z;
  _xrand_z  = t ^ _xrand_x ^ _xrand_y;
  r         = _xrand_z;
  return r;
}

}
}
