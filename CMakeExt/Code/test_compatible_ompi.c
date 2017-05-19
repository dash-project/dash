#include <mpi.h>

int main()
{

#if defined(OMPI_MAJOR_VERSION)  \
  && OMPI_MAJOR_VERSION > 2      \
  || ( OMPI_MAJOR_VERSION == 2   \
    && OMPI_MINOR_VERSION >= 1   \
    && OMPI_RELEASE_VERSION >= 1)
  // Open MPI >=2.1.1 is fine
  return 0;
#else 
#error "Open MPI version with broken alignment of shared memory windows detected!"
#endif 
}

