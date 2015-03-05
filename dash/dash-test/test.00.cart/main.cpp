
#include <iostream>
#include <libdash.h>

#include "Cart.h"
using namespace std;

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);
  
  int myid = dash::myid();
  int size = dash::size();

  dash::CartCoord<3> cc(3,4,5);

  if(myid==0) {
    for(int i=0; i<cc.extent(0); i++ ) {
      for(int j=0; j<cc.extent(1); j++ ) {
	for(int k=0; k<cc.extent(2); k++ ) {
	  fprintf(stderr, "%d %d %d %d\n", 
		  i, j, k, cc.at(i,j,k));
	}
      }
    }
  }

  dash::finalize();
}

