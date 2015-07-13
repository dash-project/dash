
#include <iostream>
#include <libdash.h>

using namespace std;

bool test_array(size_t size);

int main(int argc, char* argv[]) 
{
  dash::init(&argc, &argv);

  long long nelem = 1024*1024;
  while(nelem< 100ULL*1024*1024*1024) {
    test_array(nelem);
    nelem*=2;
  }

  dash::finalize();
}

bool test_array(size_t nelem) 
{
  double size = (double)nelem*sizeof(int);
  size /= ((double)(1024*1024));

  if(dash::myid()==0 ) {
    cout<<"Allocating "<<size<<" MB on "<<dash::size()<<" unit(s) = "<<
      size/(double)dash::size()<<" MB per unit";
  }
  
  dash::Array<int> arr(nelem);
  
  for( int i=0; i<arr.lsize(); i++ ) {
    arr.local[i] = 33;
  }

  dash::barrier(); 
  if(dash::myid()==0 ) {
    cout<<" -- SUCCESS!"<<endl;
  }

}
