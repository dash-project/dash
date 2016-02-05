
#include <iostream>
#include <libdash.h>

#include "../bench.h"

using namespace std;

template<typename T>
bool init_array(size_t lelem);

template<typename T>
void perform_test(size_t nlelem, size_t repeat);

#define REPEAT 100

int main(int argc, char * argv[])
{
  dash::init(&argc, &argv);

  perform_test<int>(1024 * 1024, 100);

  dash::finalize();
}

template<typename T>
void perform_test(size_t nlelem, size_t repeat)
{
  double tstart, tstop;

  TIMESTAMP(tstart);
  for (int i = 0; i < repeat; i++ ) {
    init_array<T>(nlelem);
  }
  TIMESTAMP(tstop);

  double lsize = (double)nlelem * sizeof(T) / ((double)(1024 * 1024));
  double gsize = lsize * (double)dash::size();

  if (dash::myid() == 0 ) {
    cout << dash::size() << ", " << lsize << ", "
         << 1000.0 * (tstop - tstart) / repeat
         << endl
         << "Initialized " << gsize << " MB on "
         << dash::size() << " unit(s) = "
         << lsize << " MB per unit "
         << endl;
  }
}


template<typename T>
bool init_array(size_t nlelem)
{
  dash::Array<T> arr(nlelem * dash::size());

  for (int i = 0; i < arr.lsize(); i++) {
    arr.local[i] = 42;
  }

  dash::barrier();

  return true;
}

