/* 
 * globref/main.cpp 
 *
 * author(s): Karl Fuerlinger, LMU Munich */
/* @DASH_HEADER@ */

#include <unistd.h>
#include <iostream>
#include <libdash.h>

#define SIZE 10

using namespace std;

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);
  
  auto myid = dash::myid();
  auto size = dash::size();

  dash::Array<int> arr(SIZE);
  
  if(myid==0) {
    dash::GlobRef<int> r1 = arr[SIZE-4];
    dash::GlobRef<int> r2 = arr[SIZE-3];
    dash::GlobRef<int> r3 = arr[SIZE-2];
    dash::GlobRef<int> r4 = arr[SIZE-1];
 
    r1=33; r3=10; r4=20; // on lhs
    int a = r4;          // on rhs

    assert(a==20);
    r2 = r1;             // lhs and rhs     
    r3+=r1;
  }
  
  arr.barrier();

  if(myid==0) {
    for( auto i=0; i<arr.size(); i++ ) cout<<arr[i]<<" ";
    cout<<endl;
  }
  
  dash::finalize();
}
