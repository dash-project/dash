#include <mpi.h>

int main()
{

#if   defined(I_MPI_VERSION)
  // Intel MPI defines both MPICH_VERSION 
  // and I_MPI_VERSION
  printf("impi %s", I_MPI_VERSION);
  return 0;
#elif defined(MVAPICH2_VERSION)
  // MVAPICH defines both MPICH_VERSION
  // and MVAPICH2_VERSION
  printf("mvapich %s", MVAPICH2_VERSION);
  return 0;
#elif defined(MPICH)
  printf("mpich %s", MPICH_VERSION);
  return 0;
#elif defined(OMPI_MAJOR_VERSION)
  printf("openmpi %d.%d.%d", 
    OMPI_MAJOR_VERSION, 
    OMPI_MINOR_VERSION, 
    OMPI_RELEASE_VERSION);
  return 0;
#else 
  // return error on unknown implementation
  printf("unknown 0.0.0");
  return 1;
#endif 
}

