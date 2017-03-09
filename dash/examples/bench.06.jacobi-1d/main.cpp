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

template<typename T>
double test_global(dash::Array<T>& v1, dash::Array<T>& v2, size_t steps);

template<typename T>
double test_local(dash::Array<T>& v1, dash::Array<T>& v2, size_t steps);

void perform_test(size_t nelem, size_t steps);


int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);

  //perform_test(100, 1);
  perform_test(100, 10);
  //  perform_test(10000, 100000);
  //  perform_test(100000, 10000);
  //  perform_test(1000000, 1000);
  //  perform_test(10000000, 100);

  dash::finalize();
}


template<typename T>
void jacobi_init(dash::Array<T>& v1, dash::Array<T>& v2)
{
  assert(v1.lsize()==v2.lsize());
  auto& pat=v1.pattern();
  typedef typename dash::Array<T>::index_type index_t;

  for(int i=0; i<v1.lsize(); i++) {
    std::array<index_t, 1> arr1, arr2;
    arr1[0]=i;
    arr2 = pat.global(pat.team().myid(), arr1);

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

  dash::Array<double> v1(nelem);
  dash::Array<double> v2(nelem);

  jacobi_init(v1, v2);
  double local = test_local(v1, v2, steps);
  if( myid==0 ) {
    cout<<jacobi_residual(v1)<<endl;
    cout<<"Local: MUPS: "<<nelem*steps*1.0e-6/local<<endl;
  }

  jacobi_init(v1, v2);
  double global = test_global(v1, v2, steps);
  if( myid==0 ) {
    cout<<jacobi_residual(v1)<<endl;
    cout<<"Global: MUPS: "<<nelem*steps*1.0e-6/global<<endl;
  }

  dash::barrier();
}

template<typename T>
double test_local(dash::Array<T>& v1,
		  dash::Array<T>& v2,
		  size_t steps)
{
  double tstart, tstop;

  TIMESTAMP(tstart);
  for( int i=0; i<steps; i++ ) {
    jacobi_local(v1, v2);
    jacobi_local(v2, v1);
    dash::barrier();
  }
  TIMESTAMP(tstop);

  return (tstop-tstart);
}

template<typename T>
double test_global(dash::Array<T>& v1,
		   dash::Array<T>& v2,
		   size_t steps)
{
  double tstart, tstop;

  TIMESTAMP(tstart);
  for( int i=0; i<steps; i++ ) {
    jacobi_global(v1, v2);
    jacobi_global(v2, v1);
    dash::barrier();
  }
  TIMESTAMP(tstop);

  return (tstop-tstart);
}


template<typename T>
void jacobi_local(const dash::Array<T>& v1,
		  dash::Array<T>& v2)
{
  auto myid = dash::myid();
  auto size = dash::size();

  auto pat = v1.pattern();

#if 0
  static_assert(std::is_same<decltype(pat), bool>::value,
		"retval must be bool");

  //static_assert(decltype(pat)::dummy_error, "DUMP MY TYPE" );
#endif

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



template<typename T>
void jacobi_global(const dash::Array<T>& v1,
		   dash::Array<T>& v2)
{
  auto myid = dash::myid();
  auto size = dash::size();

  auto pat = v1.pattern();

  // global index of first/last update for this unit
  size_t first = pat.global(0);
  size_t last  = pat.global(pat.local_size()-1);

  // adjust for global border (not updated)
  if( myid==0 )      ++first;
  if( myid==size-1 ) --last;

  // TODO: use .async[]
  // get my left and right neighbor's values that I need for my updates
  auto left  = v1[first];
  auto right = v1[last];

  // do the stencil update v2<-v1 for the interior points
  for( auto i=first+1; i<last; ++i )
    {
      v2[i] =
	0.25 * v1[i-1] +
	0.50 * v1[i]   +
	0.25 * v1[i+1];
    }

  // do the remaining left update
  v2[first] =
    0.25*left + 0.50*v1[first] + 0.25*v1[first+1];

  // do the remaining right update
  v2[last] =
    0.25*v1[last-1] + 0.50*v1[last] + 0.25*right;
}
