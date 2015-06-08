/* 
 * hello/main.cpp 
 *
 * author(s): Karl Fuerlinger, LMU Munich */
/* @DASH_HEADER@ */

#include <unistd.h>
#include <iostream>
#include <libdash.h>

using namespace std;

int main(int argc, char* argv[]) {
  dash::init(&argc, &argv);

  auto myid = dash::myid();
  auto size = dash::size();

  dash::Array<int> arr(100);

  if (myid == 0) {
    for (int i = 0; i < arr.size(); i++) {
      arr[i] = 1000-i;
    }
  }
  
  arr.barrier();

  auto min =  arr.min_element();
  cout<<min<<" "<<*min<<endl;

  dash::finalize();
}
