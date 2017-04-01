/**
 * \example ex.01.hello-mpi/main.cpp
 * A simple "hello world" example in which every unit sends a string to 
 * \c std::cout containing it's \c dash::myid() , the \c dash::size(),
 * the name of the host, the version of the underlying runtime system, 
 * and it's process id.
 */


#include <unistd.h>
#include <iostream>
#include <cstddef>
#include <sstream>

#include <libdash.h>

#ifdef DART_MPI
#include "mpi.h"
#endif

// #define INIT_TEST

using namespace std;

int main(int argc, char* argv[])
{
  pid_t pid;
  char buf[100];

#if defined(DART_MPI) && defined(INIT_TEST)
  MPI_Init(&argc, &argv);
#endif

  dash::init(&argc, &argv);

  auto myid = dash::myid();
  auto size = dash::size();

  gethostname(buf, 100);
  pid = getpid();

  if (myid == 0) {
    cout << "-------------------------"
         << endl;
#ifdef DART_MPI
    cout << argv[0] << " " << "built with DART_MPI"
         << endl;
#endif
#ifdef DART_SHMEM
    cout << argv[0] << " " << "built with DART_SHMEM"
         << endl;
#endif

#ifdef MPI_VERSION
    cout << "-------------------------" << endl;
    cout << "MPI_VERSION    : " << MPI_VERSION << endl;
    cout << "MPI_SUBVERSION : " << MPI_SUBVERSION << endl;
#ifdef MPICH
    cout << "MPICH          : " << MPICH << endl;
    cout << "MPICH_NAME     : " << MPICH_NAME << endl;
    cout << "MPICH_HAS_C2F  : " << MPICH_HAS_C2F << endl;
#endif // MPICH
#ifdef OPEN_MPI
    cout << "OPEN_MPI       : " << OPEN_MPI << endl;
#endif // OPEN_MPI
#endif // MPI_VERSION
    cout << "-------------------------" << endl;
  }

  dash::barrier();

  // To avoid interleaving output:
  std::ostringstream os;
  os << "'Hello world' from "
     << "unit " << myid << " of " << size << " "
     << "on "   << buf  << " pid=" << pid
     << endl;

  cout << os.str();

  dash::finalize();

#if defined(DART_MPI) && defined(INIT_TEST)
  MPI_Finalize();
#endif
}
