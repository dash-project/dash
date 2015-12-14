#include <unistd.h>
#include <iostream>
#include <cstddef>

#include <libdash.h>

using namespace std;


// initialize arrays
template<typename T>
void jacobi_init(dash::Array<T>& v1, dash::Array<T>& v2);

// jacobi iteration using global indices
template<typename T>
void jacobi_global(const dash::Array<T>& v1,
		   dash::Array<T>& v2);

// jacobi iteration using local access
template<typename T>
void jacobi_local(const dash::Array<T>& v1,
		  dash::Array<T>& v2);


void perform_test(size_t nelem, size_t steps);


int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);
  

  perform_test(10000, 100000);

  
  dash::finalize();
}


template<typename T>
void jacobi_init(dash::Array<T>& v1, dash::Array<T>& v2)
{
  assert(v1.lsize()==v2.lsize());
  
  for(int i=0; i<v1.lsize(); i++) {
    v1.local[i] = 0.0;
    v2.local[i] = 0.0;
  }
  dash::barrier();

  // initialize border values
  if(dash::myid()==0) {
    v1[0]           = 42.0;
    v1[v1.size()-1] = 42.0;
    
    v2[0]           = 42.0;
    v2[v2.size()-1] = 42.0;
  }    

  dash::barrier();
}



void perform_test(size_t nelem, size_t steps)
{
  auto myid = dash::myid();
  auto size = dash::size();
  
  dash::Array<double> v1(nelem);
  dash::Array<double> v2(nelem);
  
  jacobi_init(v1, v2);
  
  for( int i=0; i<steps; i++ ) {
    jacobi_local(v1, v2);
    jacobi_local(v2, v1);
  }

  dash::barrier();

  if( myid==0 ) {
    for(int i=0; i<v1.size(); i++) {
      cout<<(double)v2[i]<<" ";
    }
    cout<<endl;
  }
  
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
