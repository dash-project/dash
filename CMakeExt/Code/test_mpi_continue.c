#include <mpi.h>

int main()
{
  MPI_Request req;
  MPI_Continue_init(&req);

  return 0;
}

