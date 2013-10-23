#include <stdio.h>
#include <mpi.h>
#include "dart_types.h"
#include "dart_mem.h"
#include "dart_translation.h"
#include "dart_teamnode.h"
#include "dart_globmem.h"
#include "dart_team_group.h"
#include "mpi_team_private.h"

dart_gptr_t dart_gptr_inc_by (dart_gptr_t ptr, int inc)
{
	printf ("dart_gptr_inc_by starts\n");
	int sort;
	sort = ptr.flags;
	switch (sort)
	{
		case 0: ptr.addr_or_offs.offset += inc;
			break;
		case 1: break;
	}
	return ptr;
}

dart_ret_t dart_gptr_getaddr (const dart_gptr_t gptr, uint64_t *offset)
{
	*offset = gptr.addr_or_offs.offset;

}

dart_ret_t dart_gptr_setaddr (dart_gptr_t* gptr, uint64_t offset)
{
	gptr->addr_or_offs.offset = offset;
}

dart_ret_t dart_gptr_setunit (dart_gptr_t* gptr, dart_unit_t unit_id)
{
	gptr->unitid = unit_id;
}

dart_ret_t dart_memalloc (size_t nbytes, dart_gptr_t *gptr)
{
        dart_unit_t id;
	dart_myid(&id);
	gptr->unitid = id;
	gptr->segid = MAX_TEAM_NUMBER;
	gptr->flags = 0;
	gptr->addr_or_offs.offset = dart_mempool_alloc (localpool, nbytes);
	printf ("dart_alloc: unitid %d, the %d bytes have been allocated\n",id, nbytes);
}

dart_ret_t dart_memfree (dart_gptr_t gptr)
{	
	dart_mempool_free (localpool, gptr.addr_or_offs.offset);
	printf ("dart_memfree: flags=%d, offset=%d, unitid=%d\n", gptr.flags, gptr.addr_or_offs.offset, gptr.unitid);
}

dart_ret_t dart_team_memfree (dart_team_t team_id, dart_gptr_t gptr)
{	
	int intra_id;
       	dart_team_myid (team_id, &intra_id);
	int unique_id;
	dart_team_uniqueid (team_id, &unique_id);

	if (intra_id == 0)
	{
		dart_mempool_free (globalpool[unique_id], gptr.addr_or_offs.offset);
	}
	if (intra_id >= 0)
	{
		printf ("dart_team_memfree: flags = %d, offset = %d, unitid = %d\n", gptr.flags, gptr.addr_or_offs.offset, gptr.unitid);
		dart_transtable_remove (unique_id, gptr.addr_or_offs.offset);
	}
}

dart_ret_t dart_team_memalloc_aligned (dart_team_t team_id, size_t nbytes, dart_gptr_t *gptr)
{
 	dart_unit_t id;
	dart_team_myid(team_id, &id);
	int maximum;
	if (id >= 0)
	{
		MPI_Win win;
		MPI_Win win1;
		MPI_Comm team_t;
                int unique_id;
		dart_team_uniqueid (team_id, &unique_id);
		dart_teamnode_t val = dart_teamnode_query (team_id);
		team_t = val -> mpi_comm;
	        if (id == 0)
		{
			maximum = dart_mempool_alloc (globalpool[unique_id], nbytes);
		}
		MPI_Bcast (&maximum, 1, MPI_INT, 0, team_t);
	
		MPI_Win_create (mempool_globalalloc[unique_id] + maximum, nbytes, sizeof (char), MPI_INFO_NULL, team_t, &win);
		gptr->unitid = 0;
		gptr->segid = unique_id;
		gptr->flags = 1;
		gptr->addr_or_offs.offset = maximum;
	        info_t item;
		item.offset = maximum;
                GMRh handler;
                handler.win = win;
		item.handle = handler;
		dart_transtable_add (unique_id, item);
        	printf ("dart_team_aligned: unitid %d, the %d bytes have been allocated globally\n", id, nbytes);
	        MPI_Win_lock_all (0, handler.win);
	}
	else 
	{
		gptr->addr_or_offs.offset = -1;
		gptr->unitid = id;
		gptr->segid = -1;
		gptr->flags = -1;
	}
}

