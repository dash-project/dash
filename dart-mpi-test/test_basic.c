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
	dart_gptr_t p;
	dart_handle_t handle1, handle2;

	dart_team_memalloc_aligned (DART_TEAM_ALL, ITEMS_PER_UNIT * sizeof (int), &gptr);
        

        DART_GPTR_COPY (p, gptr);	
	if (myid == 1)
	{
		i = 42;
		dart_put_blocking (p, &i, sizeof (int), &handle1);
		dart_fence (handle1);
	}
        
	dart_barrier (DART_TEAM_ALL);

	int *localaddr;
	dart_gptr_setunit(&p, myid);
	dart_gptr_getaddr (p, &offset);
	localaddr = mempool_globalalloc[0] + offset;

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
			value = localaddr - (int*)(mempool_globalalloc[0]) + (i%ITEMS_PER_UNIT)*sizeof(int);
			dart_gptr_setaddr(&p, value);
			dart_get (val + i, p, sizeof(int), &handle1);
	      
		}
                dart_fence (handle1);

		for (i = 0;i < ITEMS_PER_UNIT * nunits; i++)
		{
			printf ("Element %d: val = %d\n", i, val[i]);
		}
		free(val);	
	}

	dart_barrier (DART_TEAM_ALL);

	dart_team_memfree (DART_TEAM_ALL, gptr);
	dart_exit();
}
