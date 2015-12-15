#include <unistd.h>
#include <iostream>
#include <cstddef>

#include <libdash.h>
#include "../bench.h"

using namespace std;


// initialize arrays
template<typename T>
void jacobi_init(dash::Array<T>& v1, dash::Array<T>& v2);


// compute residual (difference from 0)
template<typename T>
T jacobi_residual(const dash::Array<T>& v1);


template<typename T>
void print(const dash::Array<T>& v1);

// jacobi iteration using global indices
// read from 'v1', write to 'v2'
template<typename T>
void jacobi_global(const dash::Array<T>& v1,
		   dash::Array<T>& v2);


// jacobi iteration using local access
// read from 'v1', write to 'v2'
template<typename T>
void jacobi_local(const dash::Array<T>& v1,
		  dash::Array<T>& v2);


void perform_test(size_t nelem, size_t steps);


int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);
  
  perform_test(1000, 100000);
  perform_test(10000, 100000);
  perform_test(100000, 10000);
  perform_test(1000000, 1000);
  perform_test(10000000, 100);
  
  dash::finalize();
}


template<typename T>
void jacobi_init(dash::Array<T>& v1, dash::Array<T>& v2)
{
  assert(v1.lsize()==v2.lsize());
  auto& pat=v1.pattern();
  
  for(int i=0; i<v1.lsize(); i++) {
    std::array<int, 1> arr1, arr2;
    arr1[0]=i;
    arr2 = pat.global(dash::myid(), arr1);

    if( arr2[0]>v2.size()/2 )
      arr2[0]=v2.size()-1-arr2[0];
    
    v1.local[i] = (double)(arr2[0]);
    v2.local[i] = v1.local[i];
  }
  dash::barrier();
}

template<typename T>
void print(const dash::Array<T>& v1)
{
  for( int i=0; i<v1.size(); i++ ) {
    cout<<(T)v1[i]<<" ";
  }
  cout<<std::endl;
}

template<typename T>
T jacobi_residual(const dash::Array<T>& v) 
{
  T sum = (T)0.0;
  
  for( int i=0; i<v.size(); i++ ) {
    sum += (T)(v[i]);
  }
  
  return sum;
}

void perform_test(size_t nelem, size_t steps)
{
  auto myid = dash::myid();
  auto size = dash::size();
  double tstart, tstop;
  
  dash::Array<double> v1(nelem);
  dash::Array<double> v2(nelem);
  
  jacobi_init(v1, v2);
  
  TIMESTAMP(tstart);
  for( int i=0; i<steps; i++ ) {
    jacobi_local(v1, v2);
    jacobi_local(v2, v1);
    dash::barrier();
  }
  TIMESTAMP(tstop);

  if( myid==0 ) {
    cout<<"MUPS: "<<nelem*steps*1.0e-6/(tstop-tstart)<<endl;
  }
  
  dash::barrier();
}


template<typename T>
void jacobi_local(const dash::Array<T>& v1, 
		  dash::Array<T>& v2)
{
  auto myid = dash::myid();
  auto size = dash::size();
  
  auto pat = v1.pattern();

  // local index of first/last update for this unit
  size_t first = 0;
  size_t last  = pat.local_size()-1;

  // adjust for global border (not updated)
  if( myid==0 )      ++first;
  if( myid==size-1 ) --last;

  // TODO: use .async[]
  // get my left and right neighbor's values that I need for my updates
  auto left  = v1[pat.global(first)-1];
  auto right = v1[pat.global(last)+1];

  // do the stencil update v2<-v1 for the interior points
  for( auto i=first+1; i<last; ++i )
    {
      v2.local[i] = 
	0.25 * v1.local[i-1] +
	0.50 * v1.local[i]   +
	0.25 * v1.local[i+1];
    }

  // do the remaining left update
  v2.local[first] = 
    0.25*left + 0.50*v1.local[first] + 0.25*v1.local[first+1];

  // do the remaining right update
  v2.local[last] = 
    0.25*v1.local[last-1] + 0.50*v1.local[last] + 0.25*right;  
}
