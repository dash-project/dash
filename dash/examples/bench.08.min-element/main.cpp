
#include <iostream>
#include <iomanip>
#include <libdash.h>

using std::cout;
using std::cerr;
using std::endl;
using std::setw;

typedef dash::util::Timer<dash::util::TimeMeasure::Clock> Timer;

template<typename T>
void perform_test(size_t NELEM, size_t REPEAT);

int main(int argc, char **argv)
{
  double duration;
  
  dash::init(&argc, &argv);
  Timer::Calibrate(0);
    
  auto myid = dash::myid();
  auto size = dash::size();
  
  perform_test<int>(100000, 100);
  perform_test<int>(1000000, 100);
  perform_test<int>(10000000, 100);
  perform_test<int>(100000000, 20);
  perform_test<int>(200000000, 20);

  dash::finalize();
  
  return 0;
}

template<typename T>
void perform_test(size_t NELEM, size_t REPEAT)
{
  Timer::timestamp_t ts_start;
  double duration_us;

  dash::Array<T> arr(NELEM);

  for( auto& el: arr.local ) {
    el=rand();
  }
  arr.barrier();

  ts_start = Timer::Now();
  for( auto i=0; i<REPEAT; i++ ) {
    auto min = dash::min_element(arr.begin(), arr.end());
  }
  duration_us = Timer::ElapsedSince(ts_start);

  if(dash::myid()==0) {
    cout<<"NELEM: "<<setw(16)<<NELEM;
    cout<<" REPEAT: "<<setw(16)<<REPEAT;
    cout<<" TIME [sec]: "<<setw(12)<<1.0e-6*duration_us<<endl;
  }
}

