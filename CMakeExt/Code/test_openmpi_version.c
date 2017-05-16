#include <mpi.h>


/** 
 * OpenMPI <2.1.1 breaks alignment of 
 * allocated memory in shared memory
 * windows.
 */

#if   (OMPI_MAJOR_VERSION == 1   \
    && OMPI_MINOR_VERSION < 11   \
    && OMPI_RELEASE_VERSION < 7) \
  ||  (OMPI_MAJOR_VERSION  == 2  \
    && OMPI_MINOR_VERSION   < 2  \
    && OMPI_RELEASE_VERSION < 1)

#error "At least vesion 1.10.7 or 2.1.1 of OpenMPI required!"

#endif


int main()
{
  return 0;
}
