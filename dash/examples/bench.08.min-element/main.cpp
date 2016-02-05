
#include <iostream>
#include <iomanip>
#include <libdash.h>

using std::cout;
using std::cerr;
using std::endl;
using std::setw;

typedef dash::util::Timer<dash::util::TimeMeasure::Clock> Timer;

template<typename T>
void perform_test(long long NELEM, int REPEAT);

int main(int argc, char **argv)
{
  dash::init(&argc, &argv);
  Timer::Calibrate(0);

  perform_test<int>(100000ll,         1000);
  perform_test<int>(1000000ll,        1000);
  perform_test<int>(10000000ll,       1000);
  perform_test<int>(100000000ll,      1000);
  perform_test<int>(10000000000ll,    1000);
  perform_test<int>(100000000000ll,   100);
  perform_test<int>(200000000000ll,   50);

  dash::finalize();

  return 0;
}

template<typename T>
void perform_test(long long NELEM, int REPEAT)
{
  Timer::timestamp_t ts_start;
  double duration_us;

  dash::Array<T, long long> arr(NELEM);

  for (auto & el: arr.local) {
    el = rand();
  }
  arr.barrier();

  ts_start = Timer::Now();
  for( auto i=0; i<REPEAT; i++ ) {
    auto min = dash::min_element(arr.begin(), arr.end());
    dash__unused(min);
  }
  duration_us = Timer::ElapsedSince(ts_start);

  if (dash::myid() == 0) {
    cout << "NUNIT: "        << setw(10) << dash::size()
         << " NELEM: "       << setw(16) << arr.size()
         << " REPEAT: "      << setw(16) << REPEAT
         << " TIME [msec]: " << setw(12) << 1.0e-3 * duration_us
         << endl;
  }
}

