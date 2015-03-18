/* 
 * rangefor-local/main.cpp 
 *
 * author(s): Karl Fuerlinger, LMU Munich */
/* @DASH_HEADER@ */

#include <stdio.h>
#include <iostream>
#include <libdash.h>
#include <list>

using namespace std;

void test1();
void test2();

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);
  
  test1();
  test2();

  dash::finalize();
}


void test1() {
  int myid = dash::myid();
  int size = dash::size();
  
  dash::Array<int> arr1(31);
  
  for( auto it=arr1.lbegin(); it!=arr1.lend(); it++ ) {
    *it=myid;
  }
  
  arr1.barrier();
  
  if(myid==size-1) {
    fprintf(stderr, "Test1: ");
    for( auto it: arr1 ) {
      cout<<it<<" ";
    }
    cout<<endl;
  }  
}

void test2() {
  int myid = dash::myid();
  int size = dash::size();

  dash::Array<int> arr1(31);

  for( auto& it : arr1.local ) {
    it=myid;
  }
  arr1.barrier();
  
  if(myid==size-1) {
    fprintf(stderr, "Test2: ");
    for( auto it: arr1 ) {
      cout<<it<<" ";
    }
    cout<<endl;
  }
}
