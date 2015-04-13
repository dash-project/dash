/* 
 * array/main.cpp 
 *
 * author(s): Karl Fuerlinger, LMU Munich */
/* @DASH_HEADER@ */

#include <unistd.h>
#include <iostream>
#include <libdash.h>

#define NELEM 10

using namespace std;

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);
  
  auto myid = dash::myid();
  auto size = dash::size();

  dash::Pattern1D pat(NELEM*size);

  // testing of various constructor options
  dash::Array<int> arr1(NELEM*size);
  dash::Array<int> arr2(NELEM*size, dash::BLOCKED);
  dash::Array<int> arr3(NELEM*size, dash::Team::All() );
  dash::Array<int> arr4(NELEM*size, dash::BLOCKED, 
			dash::Team::All() );
  dash::Array<int> arr5(pat);

  if( myid==0 ) {
    for( int i=0; i<arr1.size(); i++ ) {
      arr1[i]=i;
      arr2[i]=i;
      arr3[i]=i;
      arr4[i]=i;
      arr5[i]=i;
    }
  }

  dash::Team::All().barrier();

  if( myid==size-1 ) {
    for( int i=0; i<arr1.size(); i++ ) {
      assert( (int)arr1[i]==(int)arr2[i]);
      assert( (int)arr1[i]==(int)arr3[i]);
      assert( (int)arr1[i]==(int)arr4[i]);
      assert( (int)arr1[i]==(int)arr5[i]);

      cout<<arr5[i]<<" ";
    }
    cout<<endl;
  }

  
  dash::finalize();
}
