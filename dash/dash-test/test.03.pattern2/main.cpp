
#include <map>
#include <iostream>
#include <libdash.h>

using namespace std;
using namespace dash;

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);
  
  int myid = dash::myid();
  int size = dash::size();

  if( myid==0 ) {
    dash::Pattern1D pat(1000);
  }

  
  dash::finalize();
}

