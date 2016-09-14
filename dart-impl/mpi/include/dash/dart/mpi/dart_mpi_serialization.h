#ifndef DASH_DART_IMPL_MPI_INCLUDE_DASH_DART_MPI_DART_MPI_SERIALIZATION_H_
#define DASH_DART_IMPL_MPI_INCLUDE_DASH_DART_MPI_DART_MPI_SERIALIZATION_H_

/* TODO: This should be determined by CMake */
#define HAVE_PTHREADS 1
#include <stdbool.h>

#if defined(HAVE_PTHREADS) && HAVE_PTHREADS
#include <pthread.h>

extern bool serialcomm;

extern pthread_mutex_t comm_mtx;



static void dash_set_serialcomm(bool flag) {
  serialcomm = flag;
}


static inline void dart_comm_down()
{
  if (serialcomm)
    pthread_mutex_lock(&comm_mtx);
}

static inline void dart_comm_up()
{
  if (serialcomm)
    pthread_mutex_unlock(&comm_mtx);
}


#else

static inline void dart_comm_down()
{
  // do nothing
}

static inline void dart_comm_up()
{
  // do nothing
}

#endif


#endif /* DASH_DART_IMPL_MPI_INCLUDE_DASH_DART_MPI_DART_MPI_SERIALIZATION_H_ */
