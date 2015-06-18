/* 
 * cart/main.cpp 
 *
 * author(s): Karl Fuerlinger, LMU Munich */
/* @DASH_HEADER@ */


#include <iostream>
#include <libdash.h>

#include <dash/Cartesian.h>
using std::cout;
using std::endl;

int main(int argc, char* argv[]) {
  dash::init(&argc, &argv);
  
  auto myid = dash::myid();
  auto size = dash::size();

  dash::CartesianIndexSpace<3, dash::ROW_MAJOR, size_t> cR(3, 3, 3);
  dash::CartesianIndexSpace<3, dash::COL_MAJOR, size_t> cC(3, 3, 3);
  
  if (myid == 0) {
    for(auto z = 0; z < cR.extent(2); z++) {
      for(auto y = 0; y < cR.extent(1); y++) {
        for(auto x = 0; x < cR.extent(0); x++) {
          cout << x << " " << y << " " << z << " ->"
               << " row major: " << cR.at(x,y,z)
               << endl;
        }
      }
    }
    for(auto x = 0; x < cC.extent(0); x++) {
      for(auto y = 0; y < cC.extent(1); y++) {
        for(auto z = 0; z < cC.extent(2); z++) {
          cout << x << " " << y << " " << z << " ->"
               << " col major: " << cC.at(x,y,z)
               << endl;
        }
      }
    }
  }

  dash::finalize();
}

