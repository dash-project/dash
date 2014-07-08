
#include <stdio.h>
#include <iostream>
#include <iterator>
#include <libdash.h>

using namespace std;

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);
  
  int myid = dash::myid();
  int size = dash::size();

  dash::Team& t = dash::TeamAll.split(2);
  dash::array<int> arr(10, t);
  
  cout<<"Hello world: I'm global "<<myid<<" of "<<size<<" and "
    "I'm "<<t.myid()<<" of "<<t.size()<<" in my sub-team"<<endl;
  
  if( t.position()==1 ) {
    std::fill(arr.lbegin(), arr.lend(), myid);
  }

  t.barrier();
  
  if( myid==2) {
    for( auto v: arr ) {
      cout<<v<<endl;
    }
  }

  dash::finalize();
}

