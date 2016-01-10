
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
    
  perform_test<int>(100);

  dash::finalize();
  
  return 0;
}

template<typename T>
void perform_test(size_t REPEAT)
{
  T sum;
  Timer::timestamp_t ts_start;
  double duration_unit0;
  double duration_local;
  double duration_neigh;

  auto myid = dash::myid();
  auto size = dash::size();
  dash::Array<T> arr(size);


  for( auto& el: arr.local ) {
    el=rand();
  }

  arr.barrier();
  ts_start = Timer::Now();
  for( auto i=0; i<REPEAT; i++ ) {
    auto val = arr[0];
    sum+=val;
  }
  arr.barrier();
  duration_unit0 = Timer::ElapsedSince(ts_start);

  arr.barrier();
  ts_start = Timer::Now();
  for( auto i=0; i<REPEAT; i++ ) {
    auto val = arr[myid];
    sum+=val;
  }
  arr.barrier();
  duration_local = Timer::ElapsedSince(ts_start);

  arr.barrier();
  ts_start = Timer::Now();
  for( auto i=0; i<REPEAT; i++ ) {
    auto val = arr[(myid+1)%size];
    sum+=val;
  }
  arr.barrier();
  duration_neigh = Timer::ElapsedSince(ts_start);
  
  
  if(dash::myid()==0) {
    cout<<"NUNITS: "<<setw(8)<<size;
    cout<<" REPEAT: "<<setw(8)<<REPEAT;
    cout<<" unit0 [sec]: "<<setw(14)<<1.0e-6*duration_unit0;
    cout<<" local [sec]: "<<setw(14)<<1.0e-6*duration_local;
    cout<<" neigh [sec]: "<<setw(14)<<1.0e-6*duration_neigh<<endl;
  }
}

