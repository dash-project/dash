
#include <stdio.h>
#include "mpi.h"

int main(int argc, char* argv[])
{
  int i;
  int size, rank;
  char buf[100];
 
  MPI_Init(&argc, &argv);
  MPI_Status status;

  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  MPI_Barrier(MPI_COMM_WORLD); 

  for(i=0; i<10000; i++ ) {
    if( rank== 0 ) {
      MPI_Send(buf, 1, MPI_CHAR, 1, 33, MPI_COMM_WORLD);
    }
    if( rank==1 ) {
      MPI_Recv(buf, 1, MPI_CHAR, 0, 33, MPI_COMM_WORLD, &status);
    }
  }

  MPI_Barrier(MPI_COMM_WORLD); 

  fprintf(stderr, "Hello world, I'm %d of %d\n", 
	  rank, size);


  MPI_Finalize();
}
