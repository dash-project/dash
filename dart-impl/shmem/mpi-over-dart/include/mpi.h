#ifndef MPI_OVER_DART_H_INCLUDED
#define MPI_OVER_DART_H_INCLUDED

#include "dart.h"

typedef int MPI_Status;

typedef dart_team_t MPI_Comm;

typedef dart_handle_t MPI_Request;

typedef enum 
  {
    MPI_CHAR = 0,
  } MPI_Datatype;

#define MPI_COMM_WORLD DART_TEAM_ALL

int MPI_Init(int *argc, char ***argv);
int MPI_Finalize();

int MPI_Comm_size(MPI_Comm comm, int *size);
int MPI_Comm_rank(MPI_Comm comm, int *rank);

int MPI_Barrier(MPI_Comm comm);

double MPI_Wtime();

int MPI_Send(void* buf,int count,MPI_Datatype datatype,
	     int dest,int tag,MPI_Comm comm);

int MPI_Recv( void *buf, int count, MPI_Datatype datatype, int source,
	      int tag, MPI_Comm comm, MPI_Status *status );



#endif /* MPI_OVER_DART_H_INCLUDED */
