#include <cstdlib>
#include <iostream>
#include <libdash.h>

#include "../bench.h"

//#define DBGOUT

//#define CUMULATIVE

#define COLLECTIVE_ALLOCATION


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
  dash::init(&argc, &argv);

  std::srand(31337);
    
  perform_test<int>( 1<<5 , 1<<3 );
  perform_test<int>( 1<<16, 1<<11 ); // NAS class S
  perform_test<int>( 1<<20, 1<<16 ); // NAS class W
  perform_test<int>( 1<<23, 1<<19 ); // NAS class A
  perform_test<int>( 1<<25, 1<<21 ); // NAS class B
  perform_test<int>( 1<<27, 1<<23 ); // NAS class C
      
  dash::finalize();
  
  return 0;
}



template<typename T>
void perform_test(unsigned NUM_KEYS, unsigned MAX_KEY)
{
  auto myid = dash::myid();
  auto size = dash::size();
  
  // global array of keys and histogram
  dash::Array<T> key_array(NUM_KEYS, dash::BLOCKED);
  dash::Array<T> key_histo(MAX_KEY,  dash::BLOCKED);

  // initialize key array to random values
  for( auto i=0; i<key_array.lsize(); i++ ) {
    key_array.local[i]=rand() % MAX_KEY;
  }

#ifdef DBGOUT
  dash::barrier();
  if( myid==0 ) {
    cout<<"key_array:"<<endl;
    for(auto i=0; i<key_array.size(); i++ ) {
      cout<<(int)key_array[i]<<" ";
    }
    cout<<endl;
  }
#endif
    
  dash::barrier();
  double t1 = test_owner_computes(key_array, key_histo);
  dash::barrier();
  double t2 = test_local_copy(key_array, key_histo);
  
  if(myid==0) {
    cout<<"NUM_KEYS: "<<NUM_KEYS<<" -- "<<"MAX_KEY: "<<MAX_KEY<<endl;
    cout<<"Owner computes : MKeys/sec: "<<(NUM_KEYS*1.0e-6)/t1<<endl;
    cout<<"Local copy     : MKeys/sec: "<<(NUM_KEYS*1.0e-6)/t2<<endl;
    cout<<"---------------------------"<<endl;
  }
  
#ifdef DBGOUT
  dash::barrier();
  if(myid==0) {
    cout<<"key_histo:"<<endl;
    for(auto i=0; i<key_histo.size(); i++ ) {
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

  auto myid = dash::myid();
  auto size = dash::size();
  
  dash::Array<dash::GlobPtr<T>> work_buffers(size);
  
#ifdef COLLECTIVE_ALLOCATION
  dash::Array<T> work_histo(size*histo.size(), dash::BLOCKED);
  work_buffers[myid] = work_histo.begin()+ myid*histo.size();
#else
  work_buffers[myid] = dash::memalloc<T>(histo.size());
#endif
  
  dash::GlobPtr<T> gptr = work_buffers[myid];
  T* work_buf = (T*) gptr;
  
  for(auto i=0; i<histo.size(); i++) {
    work_buf[i]=0;
  }
 
  dash::barrier();
  TIMESTAMP(tstart);

  // compute the histogram for the local keys
  for(auto i=0; i<keys.lsize(); i++) {
    work_buf[ keys.local[i] ]++;
  }

#ifdef CUMULATIVE
  // turn it into a cumulative histogram
  for(auto i=0; i<histo.size()-1; i++ ) {
    work_buf[i+1] += work_buf[i];
  }
#endif
 
  // compute the offset of this unit's local part in 
  // the global histo array
  auto& pat = histo.pattern();
  auto goffs = pat.global(0);

  // initialize my part of the global histogram with my own contribution
  for(auto i=0; i<histo.lsize(); i++ ) { 
    histo.local[i] = work_buf[goffs+i];
  }

  dash::barrier();
  
  for(auto unit=1; unit<size; unit++ ) {
    dash::GlobPtr<T> remote = work_buffers[(myid+unit)%size];
    
    for(auto i=0; i<histo.lsize(); i++ ) { 
      histo.local[i] += remote[goffs+i];
    }
  }
  
  dash::barrier();
  TIMESTAMP(tstop);

#ifndef COLLECTIVE_ALLOCATION
  dash::GlobPtr<T> gp=work_buffers[myid];
  dash::memfree<T>(gp);
#endif
  
  return (tstop-tstart);
}

template<typename T>
double test_local_copy(const dash::Array<T>& keys,
		       dash::Array<T>& histo)
{
  double tstart, tstop;

  auto myid = dash::myid();
  auto size = dash::size();
  
  dash::Array<dash::GlobPtr<T>> work_buffers(size);
  
#ifdef COLLECTIVE_ALLOCATION
  dash::Array<T> work_histo(size*histo.size(), dash::BLOCKED);
  work_buffers[myid] = work_histo.begin()+ myid*histo.size();
#else
  work_buffers[myid] = dash::memalloc<T>(histo.size());
#endif
  
  dash::GlobPtr<T> gptr = work_buffers[myid];
  T* work_buf = (T*) gptr;
  
  for(auto i=0; i<histo.size(); i++) {
    work_buf[i]=0;
  }
 
  dash::barrier();
  TIMESTAMP(tstart);

  // compute the histogram for the local keys
  for(auto i=0; i<keys.lsize(); i++) {
    work_buf[ keys.local[i] ]++;
  }

#ifdef CUMULATIVE
  // turn it into a cumulative histogram
  for(auto i=0; i<histo.size()-1; i++ ) {
    work_buf[i+1] += work_buf[i];
  }
#endif
 
  // compute the offset of this unit's local part in 
  // the global histo array
  auto& pat = histo.pattern();
  auto goffs = pat.global(0);

  // initialize my part of the global histogram with my own contribution
  for(auto i=0; i<histo.lsize(); i++ ) { 
    histo.local[i] = work_buf[goffs+i];
  }

  dash::barrier();
  
  for(auto unit=1; unit<size; unit++ ) {
    dash::GlobPtr<T> remote = work_buffers[(myid+unit)%size];

    dash::LocalCopy<T> lc(remote+goffs, histo.lsize());
    lc.get();
    
    for(auto i=0; i<histo.lsize(); i++ ) { 
      histo.local[i] += lc[i];
    }
  }
  
  dash::barrier();
  TIMESTAMP(tstop);

#ifndef COLLECTIVE_ALLOCATION
  dash::GlobPtr<T> gp=work_buffers[myid];
  dash::memfree<T>(gp);
#endif
  
  return (tstop-tstart);
}


