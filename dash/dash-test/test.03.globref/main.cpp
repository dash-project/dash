
#include <stdio.h>
#include <iostream>
#include <algorithm>

#include <libdash.h>

using namespace std;

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);

  int myid = dash::myid();
  int size = dash::size();
  
  dash::Array<int> arr(100, dash::Team::All() );
  
  if( myid==0 ) {
    for( int i=0; i<100; i++ ) {
      arr[i]=rand();
    }
  }
  arr.barrier();

  if( myid==1 ) {
    std::sort(arr.begin(), arr.end());
    std::fill(arr.begin(), arr.end(), 16);
  }

  if( myid==1 ) {
    for( int i=0; i<100; i++ ) {
      int res = arr[i];
      fprintf(stderr, "value at %d is %d\n", 
	      i, res);
    }
    
  }
  

  if( myid==0 ) {
    //std::sort(arr.begin(), arr.end());
    
    //    arr.begin()-arr.begin();
  }
  
  //std::sort(arr.begin(), arr.end());

  cout<<"Hello world from unit "<<myid<<" of "<<size
      <<endl;

  dash::finalize();
}

