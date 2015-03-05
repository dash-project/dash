
#include <unistd.h>
#include <iostream>
#include <libdash.h>

using namespace std;

int main(int argc, char* argv[])
{
  char buf[100];

  dash::init(&argc, &argv);
  
  int myid = dash::myid();
  int size = dash::size();

  dash::Team& t = dash::Team::All();

  gethostname(buf, 100);
  
  cout<<"'Hello world' from unit "<<myid<<
    " of "<<size<<" on "<<buf<<endl;
  
  dash::finalize();
}

