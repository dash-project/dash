#include <iostream>
#include <libdash.h>

#include "../bench.h"

using std::cout;
using std::endl;

#define NUM_KEYS     (1<<29)
#define MAX_KEY      (1<<22)

/*
#define NUM_KEYS     32
#define MAX_KEY      8
#define DBGOUT
*/

int main(int argc, char **argv)
{
  double tstart, tstop;
  srand(31337);

  dash::init(&argc, &argv);

  int myid = dash::myid();
  int size = dash::size();

  // global array of keys and histogram
  dash::Array<int> key_array(NUM_KEYS, dash::BLOCKED);
  dash::Array<int> key_histo(MAX_KEY,  dash::BLOCKED);

  using glob_ptr_t = dash::GlobMemAllocPtr<int>;

  dash::Array<glob_ptr_t> work_buffers(size, dash::CYCLIC);

  work_buffers[myid] = dash::memalloc<int>(MAX_KEY);

  glob_ptr_t gptr = work_buffers[myid];
  int * work_buf = static_cast<int *>(gptr);

  if(myid==0) {
    for(int i=0; i<key_array.size(); i++ ) {
      key_array[i]=rand() % MAX_KEY;
    }

#ifdef DBGOUT
    cout<<"key_array:"<<endl;
    for(int i=0; i<key_array.size(); i++ ) {
      cout<<key_array[i]<<" ";
    }
    cout<<endl;
#endif
  }

  dash::barrier();
  TIMESTAMP(tstart);

  // compute the histogram for the local keys
  for(int i=0; i<key_array.lsize(); i++) {
    work_buf[ key_array.local[i] ]++;
  }

  // turn it into a cumulative histogram
  for(int i=0; i<MAX_KEY-1; i++ ) {
    //work_buf[i+1] += work_buf[i];
  }

  // compute the offset of this unit's local part in
  // the global key_histo array
  auto& pat = key_histo.pattern();
  int goffs = pat.global(0);

  for(int i=0; i<key_histo.lsize(); i++ ) {
    key_histo.local[i] = work_buf[goffs+i];
  }

  for(int unit=1; unit<size; unit++ ) {
    glob_ptr_t remote = work_buffers[(myid+unit)%size];

    for(int i=0; i<key_histo.lsize(); i++ ) {
      key_histo.local[i] += static_cast<int *>(remote)[goffs+i];
    }
  }
  dash::barrier();
  TIMESTAMP(tstop);

  if(myid==0) {
    cout<<"MKeys/sec: "<<(NUM_KEYS*1.0e-6)/(tstop-tstart)<<endl;
  }

#ifdef DBGOUT
  dash::barrier();
  if(myid==0) {
    cout<<"key_histo:"<<endl;
    for(int i=0; i<key_histo.size(); i++ ) {
      cout<<key_histo[i]<<" ";
    }
    cout<<endl;
  }

  dash::barrier();
#endif

  dash::finalize();

  return 0;
}
