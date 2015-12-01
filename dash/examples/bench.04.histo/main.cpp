#include <iostream>
#include <libdash.h>

#include "../bench.h"

//#define DBGOUT

using std::cout; 
using std::endl;

// perform test with a given number of keys and key range
template<typename T>
void perform_test(unsigned NUM_KEYS, unsigned MAX_KEY);

// original version, as in OpenMP NAS IS benchmark
template<typename T>
double test_nas_is(const dash::Array<T>& keys,
		   dash::Array<T>& histo);

// "owner computes" optimization
template<typename T>
double test_owner_computes(const dash::Array<T>& keys,
			   dash::Array<T>& histo);

// using the dash::LocalCopy object
template<typename T>
double test_local_copy(const dash::Array<T>& keys,
		       dash::Array<T>& histo);


int main(int argc, char **argv)
{
  srand(31337);
  
  dash::init(&argc, &argv);

  perform_test<int>( 1<<8, 1<<5 );
  perform_test<int>( 1<<10, 1<<7 );
  perform_test<int>( 1<<12, 1<<9 );
  perform_test<int>( 1<<15, 1<<13 );
  perform_test<int>( 1<<19, 1<<14 );
  perform_test<int>( 1<<21, 1<<15 );
  perform_test<int>( 1<<23, 1<<17 );
   
  dash::finalize();
  
  return 0;
}



template<typename T>
void perform_test(unsigned NUM_KEYS, unsigned MAX_KEY)
{
  int myid = dash::myid();
  int size = dash::size();
  
  // global array of keys and histogram
  dash::Array<T> key_array(NUM_KEYS, dash::BLOCKED);
  dash::Array<T> key_histo(MAX_KEY,  dash::BLOCKED);

  for( int i=0; i<key_array.lsize(); i++ ) {
    key_array.local[i]=rand() % MAX_KEY;
  }

  /*
  // initialize key array to random values
  if(myid==0) {
    for(int i=0; i<key_array.size(); i++ ) {
      key_array[i]=rand() % MAX_KEY;
    }
#ifdef DBGOUT
    cout<<"key_array:"<<endl;
    for(int i=0; i<key_array.size(); i++ ) {
      cout<<(int)key_array[i]<<" ";
    }
    cout<<endl;
#endif
  }

  */
  
  double t1 = test_owner_computes(key_array, key_histo);
  double t2 = test_local_copy(key_array, key_histo);
  if(myid==0) {
    cout<<"NUM_KEYS: "<<NUM_KEYS<<" "<<"MAX_KEYS: "<<MAX_KEY<<endl;//<<MAX_KEYS<<endl;
    cout<<"Owner computes: MKeys/sec: "<<(NUM_KEYS*1.0e-6)/t1<<endl;
    cout<<"Local Copy    : MKeys/sec: "<<(NUM_KEYS*1.0e-6)/t2<<endl;
    cout<<"---------------------------"<<endl;
  }
  
#ifdef DBGOUT
  dash::barrier();
  if(myid==0) {
    cout<<"key_histo:"<<endl;
    for(int i=0; i<key_histo.size(); i++ ) {
      cout<<(int)key_histo[i]<<" ";
    }
    cout<<endl;
  }
  dash::barrier();
#endif
  
}

// "owner computes" optimization
template<typename T>
double test_owner_computes(const dash::Array<T>& keys,
			   dash::Array<T>& histo)
{
  double tstart, tstop;

  int myid = dash::myid();
  int size = dash::size();
  
  // use a dash::Directory instead once its implemented...
  dash::Array<dash::GlobPtr<int>> work_buffers(size, dash::CYCLIC);
  work_buffers[myid] = dash::memalloc<int>(histo.size());
  
  dash::GlobPtr<int> gptr = work_buffers[myid];
  int* work_buf = (int*) gptr;

  for(int i=0; i<histo.size(); i++) {
    work_buf[i]=0;
  }
 
  dash::barrier();
  TIMESTAMP(tstart);

  // compute the histogram for the local keys
  for(int i=0; i<keys.lsize(); i++) {
    work_buf[ keys.local[i] ]++; 
  }

  // turn it into a cumulative histogram
  for(int i=0; i<keys.size()-1; i++ ) {
    work_buf[i+1] += work_buf[i];
  }

  // compute the offset of this unit's local part in 
  // the global key_histo array
  auto& pat = histo.pattern();
  int goffs = pat.global(0);

  // initialize my part of the global histogram with my own contribution
  for(int i=0; i<histo.lsize(); i++ ) { 
    histo.local[i] = work_buf[goffs+i];
  }

  for(int unit=1; unit<size; unit++ ) {
    dash::GlobPtr<int> remote = work_buffers[(myid+unit)%size];
    
    for(int i=0; i<histo.lsize(); i++ ) { 
      histo.local[i] += remote[goffs+i];
    }
  }
  dash::barrier();
  TIMESTAMP(tstop);

  return (tstop-tstart);
}

template<typename T>
double test_local_copy(const dash::Array<T>& keys,
		       dash::Array<T>& histo)
{
  double tstart, tstop;

  int myid = dash::myid();
  int size = dash::size();
  
  // use a dash::Directory instead once its implemented...
  dash::Array<dash::GlobPtr<int>> work_buffers(size, dash::CYCLIC);
  work_buffers[myid] = dash::memalloc<int>(histo.size());
  
  dash::GlobPtr<int> gptr = work_buffers[myid];
  int* work_buf = (int*) gptr;

  for(int i=0; i<histo.size(); i++) {
    work_buf[i]=0;
  }
 
  dash::barrier();
  TIMESTAMP(tstart);

  // compute the histogram for the local keys
  for(int i=0; i<keys.lsize(); i++) {
    work_buf[ keys.local[i] ]++; 
  }

  // turn it into a cumulative histogram
  for(int i=0; i<keys.size()-1; i++ ) {
    work_buf[i+1] += work_buf[i];
  }

  // compute the offset of this unit's local part in 
  // the global key_histo array
  auto& pat = histo.pattern();
  int goffs = pat.global(0);

  // initialize my part of the global histogram with my own contribution
  for(int i=0; i<histo.lsize(); i++ ) { 
    histo.local[i] = work_buf[goffs+i];
  }

  for(int unit=1; unit<size; unit++ ) {
    dash::GlobPtr<int> remote = work_buffers[(myid+unit)%size];
    dash::LocalCopy<int> lc(remote, histo.lsize());
    lc.get();
    
    for(int i=0; i<histo.lsize(); i++ ) { 
      histo.local[i] += lc[i];
    }
  }
  dash::barrier();
  TIMESTAMP(tstop);

  return (tstop-tstart);
}


