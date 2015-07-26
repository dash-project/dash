
#include <iostream>
#include <libdash.h>

using namespace std;

template<typename T>
bool alloc_array(size_t lelem);


int main(int argc, char* argv[]) 
{
  dash::init(&argc, &argv);

  // 100 MB per unit
  size_t nelem = 25*1024*1024;
  while(nelem<1024*1024*1024) {
    alloc_array<int>(nelem);
    nelem += 25*1024*1024;
  }

  dash::finalize();
}

template<typename T>
bool alloc_array(size_t lelem) 
{
  unsigned long long nelem;
  nelem = (unsigned long long)lelem * dash::size();

  double lsize = (double)lelem*sizeof(T) / ((double)(1024*1024));
  double gsize = lsize*(double)dash::size();

  if(dash::myid()==0 ) {
    cout<<"Allocating "<<gsize<<" MB on "<<dash::size()<<" unit(s) = "<<
      lsize<<" MB per unit";
  }
  
  dash::Array<T, long long> arr(nelem);
  
  for( int i=0; i<arr.lsize(); i++ ) {
    arr.local[i] = 33;
  }

  dash::barrier(); 
  if(dash::myid()==0 ) {
    cout<<" -- SUCCESS!"<<endl;
  }

}

#if 0

#endif
