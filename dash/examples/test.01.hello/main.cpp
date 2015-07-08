/* 
 * hello/main.cpp 
 *
 * author(s): Karl Fuerlinger, LMU Munich */
/* @DASH_HEADER@ */

#include <unistd.h>
#include <iostream>
#include <cstddef>

#include <libdash.h>
#include "mpi.h"

using namespace std;

int main(int argc, char* argv[])
{
  pid_t pid;
  char buf[100];

  dash::init(&argc, &argv);
  
  auto myid = dash::myid();
  auto size = dash::size();

  gethostname(buf, 100);
  pid = getpid();

#ifdef MPI_VERSION
  if(myid==0 ) {
    cout<<"MPI_VERSION    : "<<MPI_VERSION<<endl;
    cout<<"MPI_SUBVERSION : "<<MPI_SUBVERSION<<endl;
#ifdef MPICH
    cout<<"MPICH          : "<<MPICH<<endl;
    cout<<"MPICH_NAME     : "<<MPICH_NAME<<endl;
    cout<<"MPICH_HAS_C2F  : "<<MPICH_HAS_C2F<<endl;
#endif
  }
#endif

  cout<<"'Hello world' from unit "<<myid<<
    " of "<<size<<" on "<<buf<<" pid="<<pid<<endl;

  dash::finalize();
}
