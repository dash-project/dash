
#include <stdio.h>
#include <iostream>
#include <libdash.h>

using namespace std;

int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);
  
  int myid = dash::myid();
  int size = dash::size();
  
  cout<<"Hello world from unit "<<myid<<" of "<<size
      <<endl;


  dash::finalize();
}

