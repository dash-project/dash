
#include <sys/time.h>
#include <time.h>

#include "dart.h"
#include "mpi.h"

int dart_shmem_send(void *buf, size_t nbytes, 
		    dart_team_t teamid, dart_unit_t dest);
int dart_shmem_recv(void *buf, size_t nbytes,
		    dart_team_t teamid, dart_unit_t source);


int MPI_Init(int *argc, char ***argv)
{
  dart_init(argc, argv);
  return 0;
}

int MPI_Finalize()
{
  dart_exit();
  return 0;
}

int MPI_Comm_size(MPI_Comm comm, int *size)
{
  dart_team_size(comm, size);
  return 0;
}

int MPI_Comm_rank(MPI_Comm comm, int *rank)
{
  dart_team_myid(comm, rank);
  return 0;
}

int MPI_Barrier(MPI_Comm comm)
{
  dart_barrier(comm);
  return 0;
}


double MPI_Wtime()
{
  struct timeval tv;
  double time;

  gettimeofday( &tv, NULL );
  time=((tv.tv_sec)+(tv.tv_usec)*1.0e-6);
  return time;
}


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

  dart_shmem_send(buf, nbytes, comm, dest);
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

  dart_shmem_recv(buf, nbytes, comm, source);
  return 0;
}


