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

  dash::Array<int> arr(NELEM*size);
  for( auto it = arr.lbegin(); it!=arr.lend(); it++ ) {
    (*it)=myid;
  }
  arr.barrier();
  
  if(myid==0 ) {  
    for( auto it = arr.begin(); it!=arr.end(); it++ ) {
      cout<<(*it)<<" ";
    }
    cout<<endl;
  }
  
  dash::finalize();
}
