#include <mpi.h>

int main()
{

#if MPI_VERSION >= 3
  return 0;
#else 
#error "Support for at least MPI 3.0 required!"
#endif 
}

