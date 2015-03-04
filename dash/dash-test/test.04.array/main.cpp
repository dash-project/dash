
#include <stdio.h>
#include <iostream>
#include <libdash.h>
#include <list>

using namespace std;

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);
  
  int myid = dash::myid();
  int size = dash::size();

  dash::Pattern<1> pat(19, dash::BLOCKCYCLIC(2));
  dash::Array<int, 1>     arr1(pat);
  dash::Array<double, 1>  arr2(pat);
  
  for( int i=0; i<arr1.size(); i++ ) {
    if( arr2.islocal(i) ) {
      assert(arr1.islocal(i));
      arr1[i]=myid;
      arr2[i]=10.0*((double)(i)+1);
      fprintf(stdout, "I'm unit %03d, element %03d is local to me\n",
	      myid, i);
    }
  }
  
  arr1.barrier();
  if( myid==size-1 ) {
    for( int i=0; i<arr1.size(); i++ ) {
      int res = arr1[i];
      fprintf(stdout, "Owner of %d: %d\n", i, res);
    }
  }
  fflush(stdout);
    
  arr2.barrier();
  if( myid==size-1 ) {
    for( int i=0; i<arr2.size(); i++ ) {
      double res = arr2[i];
      fprintf(stdout, "Value at %d: %f\n", i, res);
    }
  }
 fflush(stdout);

  dash::finalize();
}

