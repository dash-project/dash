
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
  dash::Pattern<1> pat(10, dash::BLOCKED, t);
  dash::Array<int, 1> arr(10, t);
  
  cout<<"Hello world: I'm global "<<myid<<" of "<<size<<" and "
    "I'm "<<t.myid()<<" of "<<t.size()<<" in my sub-team "<< arr.pattern().nunits() << " per unit " << endl;
  
  if( t.position()==1 ) {
    std::fill(arr.lbegin(), arr.lend(), myid);
//printf("unit %d write pos %d\n" ,myid, myid);
//arr[myid]=myid;
  }

  dash::Team::All().barrier();
  
  if( myid==size-1) {
    for( auto v: arr ) {
//	for(int i=0;i<10;i++){
	
	cout<<v<<endl;
    }
  }

  dash::finalize();
}

