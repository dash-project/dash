
#include <iostream>
#include <libdash.h>

using namespace std;

template<typename T>
bool alloc_array(size_t lelem);

#define REPEAT 100

int main(int argc, char* argv[]) 
{
  dash::init(&argc, &argv);

  // 100 MB per unit
  size_t nelem = 1024*1024;
  
  for(int i=0; i<REPEAT; i++ ) {
    alloc_array<int>(nelem);
  }

  double lsize = (double)nelem*sizeof(int) / ((double)(1024*1024));
  double gsize = lsize*(double)dash::size();
  
  if(dash::myid()==0 ) {
    cout<<"Tested "<<gsize<<" MB on "<<dash::size()<<" unit(s) = "<<
      lsize<<" MB per unit"<<endl;
  }


  dash::finalize();
}

template<typename T>
bool alloc_array(size_t lelem) 
{
  dash::Array<T> arr(lelem*dash::size());
  
  for( int i=0; i<arr.lsize(); i++ ) {
    arr.local[i] = 42;
  }

  dash::barrier();
}

