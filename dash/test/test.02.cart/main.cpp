/* 
 * cart/main.cpp 
 *
 * author(s): Karl Fuerlinger, LMU Munich */
/* @DASH_HEADER@ */


#include <iostream>
#include <libdash.h>

#include "Cartesian.h"
using namespace std;

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);
  
  auto myid = dash::myid();
  auto size = dash::size();

  dash::CartCoord<3> cc(3, 4, 2);
  
  if(myid==0) {
    for(auto i=0; i<cc.extent(0); i++) {
      for(auto j=0; j<cc.extent(1); j++) {
	for(auto k=0; k<cc.extent(2); k++) {
	  cout<<i<<" "<<j<<" "<<k<<" "<<
	    cc.at(i,j,k)<<endl;
	}
      }
    }
  }

  dash::finalize();
}

