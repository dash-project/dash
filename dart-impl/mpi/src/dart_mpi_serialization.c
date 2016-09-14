
#include <dash/dart/mpi/dart_mpi_serialization.h>
#if defined(HAVE_PTHREADS) && HAVE_PTHREADS

bool serialcomm = true;

pthread_mutex_t comm_mtx = PTHREAD_MUTEX_INITIALIZER;

#else
bool serialcomm = false;
#endif
