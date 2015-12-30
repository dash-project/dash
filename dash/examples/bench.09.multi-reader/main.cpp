
#include <iostream>
#include <iomanip>
#include <libdash.h>

using std::cout;
using std::cerr;
using std::endl;
using std::setw;

typedef dash::util::Timer<dash::util::TimeMeasure::Clock> Timer;

template<typename T>
void perform_test(size_t REPEAT);

int main(int argc, char **argv)
{
  double duration;
  
  dash::init(&argc, &argv);
  Timer::Calibrate(0);
    
  auto myid = dash::myid();
  auto size = dash::size();
  
  perform_test<int>(100);

  dash::finalize();
  
  return 0;
}

template<typename T>
void perform_test(size_t REPEAT)
{
  Timer::timestamp_t ts_start;
  double duration_us;

  dash::Array<T> arr(dash::size());

  for( auto& el: arr.local ) {
    el=rand();
  }
  
  arr.barrier();
  ts_start = Timer::Now();
  for( auto i=0; i<REPEAT; i++ ) {
    auto val = arr[0];
  }
  arr.barrier();
  
  duration_us = Timer::ElapsedSince(ts_start);
  
  if(dash::myid()==0) {
    cout<<"NUNITS: "<<setw(8)<<dash::size();
    cout<<" REPEAT: "<<setw(16)<<REPEAT;
    cout<<" TIME [sec]: "<<setw(12)<<1.0e-6*duration_us<<endl;
  }
}

