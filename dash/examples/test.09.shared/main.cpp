/* 
 * shared/main.cpp 
 *
 * author(s): Karl Fuerlinger, LMU Munich */
/* @DASH_HEADER@ */

#include <iostream>
#include <libdash.h>

using namespace std;

int main(int argc, char* argv[]) {
  dash::init(&argc, &argv);
  
  auto myid = dash::myid();
  auto size = dash::size();

  dash::Array< dash::GlobPtr<int> > arr(size);

  dash::Shared<int> shared;
  shared.set(100);

/*
  arr[myid] = shared.begin();
  
  if (myid == 0) {
    dash::GlobPtr<int> ptr = arr[size-1];
    for( int i=0; i < 100; i++ ) {
      ptr[i] = i;
    }
  }

  dash::barrier();

  for (auto it = shared.begin(); it != shared.end(); it++) {
    cout << myid << ":" << *it << " ";
  }
  cout << endl;
*/
  dash::finalize();
}

