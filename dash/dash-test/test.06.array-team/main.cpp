
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

  dash::Team& t = dash::Team::All().split(2);
  dash::Array<int> arr(10, dash::BLOCKCYCLIC(2), t);
  
  cout<<"Hello world: I'm global "<<myid<<" of "<<size<<" and "
    "I'm "<<t.myid()<<" of "<<t.size()<<" in my sub-team"<<endl;
  
  if( t.position()==1 ) {
    std::fill(arr.lbegin(), arr.lend(), myid);
  }

  t.barrier();
  
  if( myid==size-1) {
    for( auto v: arr ) {
      cout<<v<<endl;
    }
  }

  dash::finalize();
}

