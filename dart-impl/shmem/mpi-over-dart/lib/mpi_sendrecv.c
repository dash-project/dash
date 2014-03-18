
#include <sys/time.h>
#include <time.h>

#include "dart.h"
#include "mpi.h"


int MPI_Send(void* buf, int count, MPI_Datatype datatype,
	     int dest, int tag, MPI_Comm comm)
{
  int nbytes;
  switch( datatype ) {
  case MPI_CHAR:
    nbytes = count;
  default:
    nbytes = count;
  }

  dart_shmem_sendevt(buf, nbytes, comm, dest);
  return 0;
}



int MPI_Recv(void* buf, int count, MPI_Datatype datatype,
	     int source, int tag, MPI_Comm comm, MPI_Status *status)
{
  int nbytes;
  switch( datatype ) {
  case MPI_CHAR:
    nbytes = count;
  default:
    nbytes = count;
  }

  dart_shmem_recvevt(buf, nbytes, comm, source);
  return 0;
}



int MPI_Isend(void* buf, int count, MPI_Datatype datatype,
	      int dest, int tag, MPI_Comm comm, MPI_Request *request)
{
  int nbytes;
  switch( datatype ) {
  case MPI_CHAR:
    nbytes = count;
  default:
    nbytes = count;
  }

  dart_shmem_isend(buf, nbytes, comm, dest, request);
  return 0;
}



int MPI_Irecv(void* buf, int count, MPI_Datatype datatype,
	     int source, int tag, MPI_Comm comm, MPI_Request *request)
{
  int nbytes;
  switch( datatype ) {
  case MPI_CHAR:
    nbytes = count;
  default:
    nbytes = count;
  }

  dart_shmem_irecv(buf, nbytes, comm, source, request);
  return 0;
}

int MPI_Waitall(int count, MPI_Request array_of_requests[], 
		MPI_Status array_of_statuses[])
{
  return 0;
}

