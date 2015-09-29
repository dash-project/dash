#include <stdio.h>
#include "dart_app_privates.h"
#include "../dart/dart.h"
#define ITEMS_PER_UNIT 5

int main (int argc, char* argv[])
{
	int i, myid;
	size_t nunits;
	uint64_t offset;
	int value;
	dart_init (&argc, &argv);
	dart_size (&nunits);
	dart_myid (&myid);
	dart_gptr_t gptr = DART_GPTR_NULL;
	dart_gptr_t point;
	dart_gptr_t p;

        dart_handle_t handle[ITEMS_PER_UNIT * nunits];
	for (i = 0; i < ITEMS_PER_UNIT * nunits; i++)
	{
		handle[i] = (dart_handle_t) malloc (sizeof (struct dart_handle_struct));
	}

	dart_team_memalloc_aligned (DART_TEAM_ALL, ITEMS_PER_UNIT * sizeof (int), &gptr);
        
	dart_barrier (DART_TEAM_ALL);
	DART_GPTR_COPY (p, gptr);	
	if (myid == 1)
	{
		i = 42;
		dart_put_blocking (p, &i, sizeof (int));
	}
        
	dart_barrier (DART_TEAM_ALL);

	int *localaddr;

	offset = p.addr_or_offs.offset;

	dart_gptr_getaddr (gptr, &localaddr);

	for (i = 0; i<ITEMS_PER_UNIT; i++)
	{
		localaddr[i] = myid + i;
	}

	dart_barrier (DART_TEAM_ALL);
	
	if (myid == 3)
	{
		int *val;
		val = (int *)malloc(ITEMS_PER_UNIT * nunits * sizeof(int));

		for (i = 0;i < ITEMS_PER_UNIT * nunits; i++)
		{
			dart_gptr_setunit(&p, i/ITEMS_PER_UNIT);
			
			value = offset + (i%ITEMS_PER_UNIT)*sizeof(int);
			p.addr_or_offs.offset = value;
				
			dart_get (val + i, p, sizeof(int), &handle[i]);
	      
		}
                dart_waitall (handle, ITEMS_PER_UNIT * nunits);

		for (i = 0;i < ITEMS_PER_UNIT * nunits; i++)
		{
			printf ("%2d: (Element %d) <=> (val = %d)\n", myid, i, val[i]);
		}
		free(val);	
	}

	dart_barrier (DART_TEAM_ALL);

	dart_team_memfree (DART_TEAM_ALL, gptr);
	dart_exit();
}
