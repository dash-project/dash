/* 
 * datatypes/main.cpp 
 *
 * author(s): Karl Fuerlinger, LMU Munich */
/* @DASH_HEADER@ */

#include <unistd.h>
#include <iostream>
#include <libdash.h>

using namespace std;

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);
  
  auto myid = dash::myid();
  auto size = dash::size();

  typedef pair<int, int> pair_t;
  dash::Array<pair_t> arr(100);

  if(myid==0 ) {
    for(auto i=0; i<arr.size(); i++)  {
      arr[i] = {i,i+1};
    }
  }
  
  arr.barrier();

  pair_t p;
  if(myid==size-1) {
    for(auto el: arr ) {
      cout<<((pair_t)el).second<<" ";
    }
    cout<<endl;
  }
  
  dash::finalize();
}
