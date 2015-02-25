
#include <iostream>
#include <libdash.h>

#include <deque>
#include <vector>

using namespace std;

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);
  
  int myid = dash::myid();
  int size = dash::size();

  if( myid==0 ) {
    std::vector<int> v(20);
    for( int i=0; i<v.size(); i++ ) {
      v[i]=i;
    }
    
    dash::CartView<decltype(v)::iterator, 2> cv(v.begin(), 3,4);
    
    for( int i=0; i<cv.extent(0); i++ ) {
      for( int j=0; j<cv.extent(1); j++ ) {
	fprintf(stderr, "%d %d - %d\n", i, j, cv.at(i,j));
	
	cv.at(i,j) = 33+(i+j);
      }
    }

    for( int i=0; i<v.size(); i++ ) {
      fprintf(stderr, "%d\n", v[i]);
    }
  }
  
  dash::finalize();
}

