
#include <iostream>
#include <libdash.h>

using namespace std;

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);
  
  dash::Shared<int> a;
  if( dash::myid==0 ) {
    a=0;
  }
  dash::Team::All().barrier();
  

  for( int i=0; i<dash::size(); i++ ) {
    if( dash::myid()==i )
      a=a+1;
  }

  dash::Team::All().barrier();

  if( dash::myid()==0 ) 
    cout<<a<<endl;
  
  dash::finalize();
}

