/* 
 * hello/main.cpp 
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

  const int n = 40;
  dash::Array< dash::GlobPtr<int> > arr(n);

  for( int i=0; i<arr.size(); i++ ) {
    if( arr[i].is_local() ) {
      cout<<i<<" is mine "<<myid<<endl;
    }
  }

  
  dash::finalize();
}
