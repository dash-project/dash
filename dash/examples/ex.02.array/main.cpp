/* 
 * hello/main.cpp 
 *
 * author(s): Karl Fuerlinger, LMU Munich */
/* @DASH_HEADER@ */

#include <unistd.h>
#include <iostream>
#include <cstddef>

#include <libdash.h>

using namespace std;

int main(int argc, char* argv[])
{
  pid_t pid;
  char buf[100];

  dash::init(&argc, &argv);
  
  auto myid = dash::myid();
  auto size = dash::size();

  dash::Array<int> arr(100);

  if( myid==0 ) {
    for( auto i=0; i<arr.size(); i++ ) {
      arr[i]=i;
    }
  }
  arr.barrier();
  if( myid==size-1 ) {
    for( auto el: arr ) 
      cout<<(int)el<<" ";
    cout<<endl;
  }
  
  dash::finalize();
}
