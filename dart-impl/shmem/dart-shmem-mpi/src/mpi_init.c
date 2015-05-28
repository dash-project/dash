
#include <sys/time.h>
#include <time.h>

#include <dash/dart/if/dart.h>
#include <dash/dart/shmem/mpi/mpi.h>

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
  dart_team_size(comm, (size_t*)(size));
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

