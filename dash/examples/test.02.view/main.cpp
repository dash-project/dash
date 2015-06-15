/* 
 * view/main.cpp 
 *
 * author(s): Karl Fuerlinger, LMU Munich */
/* @DASH_HEADER@ */

#include <iostream>
#include <deque>
#include <vector>
#include <libdash.h>

#define SIZE_X  3
#define SIZE_Y  5

using namespace std;

int main(int argc, char* argv[]) {
  dash::init(&argc, &argv);
  
  auto myid = dash::myid();
  auto size = dash::size();

  if (myid == 0) {
    std::deque<int> v;
    for (int i = 0; i < SIZE_X * SIZE_Y; i++) {
      v.push_back(i);
    }
    
    dash::CartView<decltype(v)::iterator, 2> cv(
      v.begin(), SIZE_X, SIZE_Y);
    
    for (int i = 0; i < cv.extent(0); i++) {
      for (int j = 0; j < cv.extent(1); j++) {
        fprintf(stderr, "(%d, %d) - %d\n", i, j, cv.at(i,j));
        cv.at(i,j) = 33+(i+j);
      }
    }
    for (int i = 0; i < v.size(); i++) {
      fprintf(stderr, "%d - %d\n", i, v[i]);
    }
  }
  dash::finalize();
}

