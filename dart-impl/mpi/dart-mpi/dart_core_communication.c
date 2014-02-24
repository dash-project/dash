#include <stdio.h>
#include "dart_types.h"
#include "dart_globmem.h"
#include "dart_team_group.h"
#include "dart_communication.h"



dart_ret_t dart_get (void *dest, dart_gptr_t gptr, size_t nbytes, dart_handle_t *handle)
{
	       return dart_adapt_get (dest, nbytes, handle) ;
}

dart_ret_t dart_put (dart_gptr_t gptr, void *src, size_t nbytes, dart_handle_t *handle)
{
	       return dart_adapt_put (gptr, src, nbytes, handle);
}

dart_ret_t dart_get_blocking (void *dest, dart_gptr_t gptr, size_t nbytes)
{
	       return dart_adapt_get_blocking (dest, gptr, nbytes);
}

dart_ret_t dart_wait (dart_handle_t handle)
{
           return dart_adapt_wait (handle);
}

dart_ret_t dart_test (dart_handle_t handle)
{
           return dart_adapt_test (handle);
}

dart_ret_t dart_waitall (dart_handle_t *handle, size_t n)
{
           return dart_adapt_waitall (handle, n);
}


dart_ret_t dart_testall (dart_handle_t *handle, size_t n)
{
           return dart_adapt_testall (handle, n);
}

dart_ret_t dart_barrier (dart_team_t teamid)
{
           return dart_adapt_barrier (teamid);
}

dart_ret_t dart_bcast (void *buf, size_t nbytes, int root, dart_team_t teamid)
{
	       return dart_adapt_bcast (buf, nbytes, root, teamid);
}

dart_ret_t dart_scatter (void *sendbuf, void *recvbuf, size_t nbytes, int root, dart_team_t teamid)
{
           return dart_adapt_scatter (sendbuf, recvbuf, nbytes, root, teamid);
}

dart_ret_t dart_gather (void *sendbuf, void *recvbuf, size_t nbytes, int root, dart_team_t teamid)
{
	       return dart_adapt_gather (sendbuf, recvbuf, nbytes, root, teamid);
}

dart_ret_t dart_allgather (void *sendbuf, void *recvbuf, size_t nbytes, dart_team_t teamid)
{
           return dart_adapt_allgather (sendbuf, recvbuf, nbytes, teamid);
}
