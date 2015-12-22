
#include <iostream>
#include <iomanip>

#include <libdash.h>

using namespace std;

template<typename T>
void test_array_alloc(size_t lelem);


int main(int argc, char* argv[]) 
{
  dash::init(&argc, &argv);

  // 100 MB per unit, if sizeof(int)==4
  size_t nelem = 25*1024*1024;
  while(nelem<1000*1024*1024) {
    test_array_alloc<int>(nelem);
    nelem += 25*1024*1024;
  }
  
  dash::finalize();
}


//
// test the allocation and initialization 
// of an array with lelem local elements
//
template<typename T>
void test_array_alloc(size_t lelem) 
{
  auto myid = dash::myid();
  unsigned long long nelem;
  nelem = (unsigned long long)lelem * dash::size();

  // local size of allocation in megabytes
  double lsize = (double)lelem*sizeof(T) / ((double)(1024*1024));
  
  // global size of allocation in megabytes
  double gsize = lsize*(double)dash::size();
  
  if(myid==0 ) {
    cout << "Allocating " << setw(20) << gsize << " MB on ";
    cout << setw(14) << dash::size() << " unit(s) = ";
    cout << setw(20) << lsize << " MB per unit";
    cout.flush();
  }
  
  dash::Array<T, long long> arr(lelem);
  
  for( int i=0; i<arr.lsize(); i++ ) {
    arr.local[i] = myid;
  }

  dash::barrier(); 

  if(myid==0 ) {
    cout<<" -- SUCCESS!"<<endl;
  }

  dash::barrier(); 
}

