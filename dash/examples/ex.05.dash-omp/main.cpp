#include <unistd.h>
#include <iostream>
#include <cstddef>

#include <libdash.h>

using namespace std;

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);
  
  auto myid = dash::myid();
  auto size = dash::size();

  // the equivalent to the OpenMP master construct
  // no barrier at the end
  dash::omp::master( [&]() {
    cout<<"Unit "<<myid<<" executes master"<<endl<<flush;
    sleep(1.0);
  });
  
  // the equivalent to the OpenMP single construct
  // there is a barrier at the end, unless the "nowait" variant is used
  dash::omp::single( [&]() {
    cout<<"Unit "<<myid<<" executes single"<<endl<<flush;
    sleep(1.0);
  });
  
  cout<<"Unit "<<myid<<" after single"<<endl<<flush;
  dash::barrier();
  dash::omp::master( [&]() {
    cout<<"----------------------"<<endl<<flush;
  });
  dash::barrier();
  
  // the equivalent to the OpenMP single construct with nowait clause
  dash::omp::single_nowait( [&]() {
    cout<<"Unit "<<myid<<" executes single nowait"<<endl<<flush;
    sleep(1.0);
  });
  
  cout<<"Unit "<<myid<<" after single"<<endl<<flush;
  dash::barrier();
  dash::omp::master( [&]() {
    cout<<"----------------------"<<endl<<flush;
  });
  dash::barrier();
  
  
  // the equivalent to the OpenMP critical construct
  dash::omp::critical( [&]() {
    cout<<"Unit "<<myid<<" critical"<<endl<<flush;
    sleep(1.0);
  });
  
  dash::barrier();
  dash::omp::master( [&]() {
    cout<<"----------------------"<<endl<<flush;
  });
  dash::barrier();
  
  // the equivalent to the OpenMP sections/section construct
  dash::omp::sections( [&]() {
    
    dash::omp::section( [&]() {
      cout<<"sec1 executed by "<<dash::myid()<<endl<<flush;
      sleep(1.0);
    });
    
    dash::omp::section( []() {
      cout<<"sec2 executed by "<<dash::myid()<<endl<<flush;
      sleep(2.0);
    });
  });
  
  dash::barrier();
  dash::omp::master( [&]() {
    cout<<"----------------------"<<endl<<flush;
  });
  dash::barrier();

  // the equivalent to the OpenMP for loop with schedule "static"
  dash::omp::for_loop(0, 10, 1, dash::BLOCKED, [&](int i) {
    cout<<"Unit "<<myid<<" executes iteration "<<i<<endl<<flush;
  });

  dash::barrier();
  dash::omp::master( [&]() {
    cout<<"----------------------"<<endl<<flush;
  });
  dash::barrier();

  // the equivalent to the OpenMP for loop with the "nowait" clause
  dash::omp::for_loop_nowait(0, 10, 1, dash::BLOCKED, [&](int i) {
    cout<<"Unit "<<myid<<" executes iteration "<<i<<endl<<flush;
  });
  
      
  dash::finalize();
}
