#include <stdio.h>
#include <mpi.h>
#include "dart_types.h"
#include "dart_translation.h"
#include "dart_teamnode.h"
#include "dart_mem.h"
#include "dart_initialization.h"
#include "dart_globmem.h"
#include "dart_communication.h"
#include "dart_team_group.h"
#include "mpi_team_private.h"

dart_ret_t dart_get (void *dest, dart_gptr_t ptr, size_t nbytes, dart_handle_t *dart_req)
{
	MPI_Request mpi_req;
	dart_unit_t target_unitid = ptr.unitid;
	int offset = ptr.addr_or_offs.offset;
	int flags = ptr. flags;
	printf ("dart_get start: %d\n", target_unitid);
	MPI_Win win;
	if (flags == 1)
	{
		int begin;
	        win = dart_transtable_query (ptr.segid, offset, &begin);

		int difference = offset - begin;
		MPI_Rget (dest, nbytes, MPI_BYTE, target_unitid, difference, nbytes, MPI_BYTE, win, &mpi_req);
		dart_req -> request = mpi_req;
		dart_req -> mpi_win = win;
		printf ("dart_get is finished\n");
	
	}
	else if (flags == 0)
	{
		printf ("dart_get / local access starts\n");
		win = win_local_alloc;
		MPI_Rget (dest, nbytes, MPI_BYTE, target_unitid, offset, nbytes, MPI_BYTE, win, &mpi_req);
		dart_req -> request = mpi_req;
		dart_req -> mpi_win = win;
		printf ("dart_get / local access finishes\n");

	}
}

dart_ret_t dart_put (dart_gptr_t ptr, void *src, size_t nbytes, dart_handle_t *dart_req)
{
	MPI_Request mpi_req;
	dart_unit_t target_unitid = ptr.unitid;
	int offset = ptr.addr_or_offs.offset;
	int flags = ptr.flags;
	MPI_Win win;
	if (flags == 1)
	{
		int begin;
		win = dart_transtable_query (ptr.segid, offset, &begin);

		printf ("dart_put / team access starts\n");
		int difference = offset - begin;

		MPI_Rput (src, nbytes, MPI_BYTE, target_unitid, difference, nbytes, MPI_BYTE, win, &mpi_req);
		dart_req -> request = mpi_req;
		dart_req -> mpi_win = win;
                printf ("dart_put / team access is finished \n");
	}
	else if (flags == 0)
	{
		win = win_local_alloc;
		printf ("dart_put / local access starts \n");
		MPI_Rput (src, nbytes, MPI_BYTE, target_unitid, offset, nbytes, MPI_BYTE, win, &mpi_req);
		dart_req -> request = mpi_req;
		dart_req -> mpi_win = win;
		printf ("dart_put / local access finished\n");
	}
}

dart_ret_t dart_get_blocking (void *dest, dart_gptr_t ptr, size_t nbytes, dart_handle_t *dart_req)
{
	dart_get (dest, ptr, nbytes, dart_req);
	dart_wait (dart_req);
}

dart_ret_t dart_put_blocking (dart_gptr_t ptr, void *src, size_t nbytes, dart_handle_t *dart_req)
{
	dart_put (ptr, src, nbytes, dart_req);
	dart_wait (dart_req);
}

dart_ret_t dart_fence (dart_handle_t handle)
{
	int id;
        MPI_Win_unlock_all (handle.mpi_win);
	MPI_Win_lock_all (0, handle.mpi_win);
}

dart_ret_t dart_wait (dart_handle_t* handle)
{
	MPI_Status mpi_sta;
	MPI_Request mpi_req = handle -> request;
	MPI_Win win;
	win = handle -> mpi_win;
	MPI_Wait (&mpi_req, &mpi_sta);
	handle -> request = mpi_req; 
	printf ("dart_wait is finished\n");
}

dart_ret_t dart_test (dart_handle_t* handle)
{
	MPI_Status mpi_sta;
	int flag;
	MPI_Request mpi_req = handle -> request;
	MPI_Win win;
	win = handle -> mpi_win;
	MPI_Test (&mpi_req, &flag, &mpi_sta);
	handle -> request = mpi_req;
	printf ("dart_test is finished\n");
	return flag;
}


dart_ret_t dart_waitall (dart_handle_t *handle, int count)
{
	MPI_Request *mpi_req;
	MPI_Status *mpi_sta;
	mpi_req = (MPI_Request *)malloc (count * sizeof (MPI_Request));
	mpi_sta = (MPI_Status *)malloc (count * sizeof (MPI_Status));
	int i;
	for (i = 0; i < count; i++)
	{
		mpi_req[i] = handle[i].request;
	}
	MPI_Waitall (count, mpi_req, mpi_sta);
	for (i = 0; i < count; i++)
	{
		handle[i].request = mpi_req[i];
      
	}
}

dart_ret_t dart_testall (dart_handle_t *handle, int count)
{
	MPI_Request *mpi_req;
	MPI_Status *mpi_sta;
	mpi_req = (MPI_Request *)malloc (count * sizeof (MPI_Request));
	mpi_sta = (MPI_Status *)malloc (count * sizeof (MPI_Status));
	int i, flag;
	for (i = 0; i < count; i++)
	{
		mpi_req[i] = handle[i].request;
	}
	MPI_Testall (count, mpi_req, &flag, mpi_sta);
	for (i = 0; i < count; i++)
	{
		handle[i].request = mpi_req[i];
	}
	return flag;

}

dart_ret_t dart_barrier (dart_team_t team_id)
{
	MPI_Comm comm;

	dart_teamnode_t p = dart_teamnode_query (team_id);
        comm = p -> mpi_comm;

	if (comm != MPI_COMM_NULL)
	{
		MPI_Barrier (comm);
	}
}

dart_ret_t dart_bcast (void *buf, size_t nbytes, int root, dart_team_t team_id)
{
	MPI_Comm comm;

	dart_teamnode_t p = dart_teamnode_query(team_id);
	comm = p -> mpi_comm;

	dart_unit_t id;
	dart_team_myid (team_id, &id);
	if (comm != MPI_COMM_NULL)
	{
		MPI_Bcast (buf, nbytes, MPI_BYTE, root, comm);
	}
}

dart_ret_t dart_scatter (void *sendbuf, void *recvbuf, size_t nbytes, int root, dart_team_t team_id)
{
	MPI_Comm comm;

	dart_teamnode_t p = dart_teamnode_query(team_id);
	comm = p -> mpi_comm;

	if (comm != MPI_COMM_NULL)
	{
		MPI_Scatter (sendbuf, nbytes, MPI_BYTE, recvbuf, nbytes, MPI_BYTE, root, comm);
	}
}

dart_ret_t dart_gather (void *sendbuf, void *recvbuf, size_t nbytes, int root, dart_team_t team_id)
{
	MPI_Comm comm;

	dart_teamnode_t p = dart_teamnode_query (team_id);
	comm = p -> mpi_comm;

        if (comm != MPI_COMM_NULL)
	{
		MPI_Gather (sendbuf, nbytes, MPI_BYTE, recvbuf, nbytes, MPI_BYTE, root, comm);
	}
}

dart_ret_t dart_allgather (void *sendbuf, void *recvbuf, size_t nbytes, dart_team_t team_id)
{
	MPI_Comm comm;
	dart_teamnode_t p = dart_teamnode_query (team_id);

	comm = p -> mpi_comm;

	if (comm != MPI_COMM_NULL)
	{
		MPI_Allgather (sendbuf, nbytes, MPI_BYTE, recvbuf, nbytes, MPI_BYTE, comm);
	}
}
