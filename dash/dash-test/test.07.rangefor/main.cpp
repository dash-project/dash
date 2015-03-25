/* 
 * rangefor/main.cpp 
 *
 * author(s): Karl Fuerlinger, LMU Munich */
/* @DASH_HEADER@ */

#include <stdio.h>
#include <iostream>
#include <libdash.h>
#include <list>

using namespace std;

void test();

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);
  
  test();

  dash::finalize();
}


void test() {
  int myid = dash::myid();
  int size = dash::size();

  dash::Array<int> arr1(3113);

  if(myid==0) {
    for( auto it: arr1 ) {
      it=33;
    }
  }
  arr1.barrier();
  
  if(myid==size-1) {
    for( auto it: arr1 ) {
      cout<<it<<" ";
    }
    cout<<endl;
  }  
}

