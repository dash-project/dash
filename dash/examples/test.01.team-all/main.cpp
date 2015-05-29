/* 
 * team-all/main.cpp 
 *
 * author(s): Karl Fuerlinger, LMU Munich */
/* @DASH_HEADER@ */

#include <assert.h>
#include <unistd.h>
#include <iostream>
#include <libdash.h>

using namespace std;

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);
  
  auto myid = dash::myid();
  auto size = dash::size();
  
  dash::Team& t = dash::Team::All();
  assert( myid == t.myid() );
  assert( size == t.size() );

  cerr<<"Unit "<<myid<<" before barrier..."<<endl;
  if( myid==size-1 ) {
    cerr<<"Unit "<<myid<<" sleeping..."<<endl;
    sleep(2);
  }
  t.barrier();

  cerr<<"Unit "<<myid<<" after barrier!"<<endl;

  
  dash::finalize();
}
