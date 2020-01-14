#include <mpi.h>

#ifdef OMPI_MAJOR_VERSION
// Open MPI currently does not expose the extended grequests directly
typedef int (ompi_grequestx_poll_function)(void *, MPI_Status *);
int ompi_grequestx_start(
    MPI_Grequest_query_function *gquery_fn,
    MPI_Grequest_free_function *gfree_fn,
    MPI_Grequest_cancel_function *gcancel_fn,
    ompi_grequestx_poll_function *gpoll_fn,
    void* extra_state,
    MPI_Request* request);
#define MPIX_Grequest_start(query_fn,free_fn,cancel_fn,poll_fn,extra_state,request) \
  ompi_grequestx_start(query_fn,free_fn,cancel_fn,poll_fn,extra_state,request)

#elif defined (MPICH_VERSION)
#define MPIX_Grequest_start(query_fn,free_fn,cancel_fn,poll_fn,extra_state,request) \
  MPIX_Grequest_start(query_fn,free_fn,cancel_fn,poll_fn,NULL/*MPICH has an extra wait function*/,extra_state,request)
#endif

int main()
{
  MPI_Request req;
  MPIX_Request_on_completion(&req, NULL, NULL);

  return 0;
}

