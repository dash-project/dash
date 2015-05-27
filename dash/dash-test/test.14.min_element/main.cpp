/* 
 * hello/main.cpp 
 *
 * author(s): Karl Fuerlinger, LMU Munich */
/* @DASH_HEADER@ */

#include <mpi.h>
#include <unistd.h>
#include <iostream>
#include <libdash.h>

using namespace std;

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);

  auto myid = dash::myid();
  auto size = dash::size();

  dash::Array<int> arr(100);

  if(myid==0) {
    for(int i=0; i<arr.size(); i++ ) {
      arr[i]=200-i;
    }
  }

  if( myid==1 ) {
    auto lptr = arr.local.begin();
    auto gptr = arr.local.globptr(lptr);
    
    cout<<gptr<<*gptr<<endl;
    
  }

  // arr.barrier();
  // arr.min_element();

  dash::finalize();
}
