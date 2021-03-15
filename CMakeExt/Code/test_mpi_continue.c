#include <mpi.h>

int main()
{
  MPI_Request req;
  MPIX_Continue_init(&req, MPI_INFO_NULL);

  return 0;
}

