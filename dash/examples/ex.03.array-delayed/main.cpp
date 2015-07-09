/* 
 * array/main.cpp 
 *
 * author(s): Karl Fuerlinger, LMU Munich */
/* @DASH_HEADER@ */

#include <unistd.h>
#include <iostream>
#include <libdash.h>
#include <algorithm>

#define NELEM 10

using namespace std;

// global var
dash::Array<int> arr1;

int main(int argc, char* argv[])
{
  // before init
  dash::Array<int> arr2;

  dash::init(&argc, &argv);
  
  // after init
  dash::Array<int> arr3;

  auto myid = dash::myid();
  auto size = dash::size();

  arr1.allocate(NELEM*size, dash::BLOCKED);
  arr2.allocate(NELEM*size, dash::BLOCKED);
  arr3.allocate(NELEM*size, dash::BLOCKED);

  dash::finalize();
}
