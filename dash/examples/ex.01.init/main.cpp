#ifdef DASH_DART_MPI

#define DASH_ENABLE_LOGGING 1
#define DASH_ENABLE_TRACE_LOGGING 1

// #include <libdash.h>
#include <dash/dart/if/dart.h>

#include <mpi.h>

int main(int argc, char * argv[])
{
//  DASH_LOG_DEBUG("MPI_Init ...");
  MPI_Init(&argc, &argv);
//  DASH_LOG_DEBUG("dash::init ...");
// dash::init(&argc, &argv);
  dart_init(&argc, &argv);
//  DASH_LOG_DEBUG("dash::finalize ...");
// dash::finalize();
  dart_exit();
//  DASH_LOG_DEBUG("MPI_Finalize ...");
  MPI_Finalize();
//  DASH_LOG_DEBUG("Exit");
  return 0;
}

#else

int main(int argc, char * argv[])
{
  // Need MPI for this example
  return -1;
}

#endif
