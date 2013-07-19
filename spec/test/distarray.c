
#include <stdio.h>
#include "dart.h"
#include "dart_gptr_impl.h"

#define ITEMS_PER_UNIT 5

int main( int argc, char* argv[]) 
{
  int i, myid, val;
  size_t nunits;

  dart_init(&argc, &argv);

  dart_size(&nunits);
  dart_myid(&myid);

  dart_gptr_t gptr = DART_GPTR_NULL;

  fprintf(stdout, "I'm %d of %d units in this program\n",
	  myid, nunits);

  // allocate space for ITEMS_PER_UNIT integers per unit 
  dart_team_memalloc_aligned(DART_TEAM_ALL,
			     ITEMS_PER_UNIT*sizeof(int), &gptr);  

  // dart_team_memalloc_aligned is a collective call
  // on *each unit* it sets the passed gptr to the beginning of
  // the whole allocation, so gptr will be identical on all units
  // after the call


  // one could do the following:
  if( myid==1 ) {
    // unit 1 writes the value '42' into the first element 
    // of the allocated memory (owned by unit 0)
    i=42;
    dart_put( gptr, &i, sizeof(int) );
  }

  // initialize the array in parallel (all units initialize their
  // portion of the array) a.k.a. "owner computes" 
  int *localaddr;
  DART_GPTR_SETUNIT(gptr, myid);
  localaddr = DART_GPTR_GETADDR(gptr);
  
  for(i=0; i<ITEMS_PER_UNIT; i++ ) {
    localaddr[i] = myid+i;
  }

  dart_barrier(DART_TEAM_ALL);

  // unit 3 prints the whole array
  if( myid==3 ) 
    {
      for(i=0; i<ITEMS_PER_UNIT*nunits; i++ ) {

	// here we can construct the gptr to *any* location in the 
	// allocation by simple arithmetic. This only works because the 
	// allocation was symmetric and team-aligned
	DART_GPTR_SETUNIT(gptr, i/ITEMS_PER_UNIT);
	DART_GPTR_SETADDR(gptr, localaddr+i%ITEMS_PER_UNIT);

	void *addr;	
	addr = DART_GPTR_GETADDR(gptr);
	
	dart_get( &val, gptr, sizeof(int) );
	fprintf(stdout, "Element %3d: val=%d local_addr=%p\n", 
		i, val, addr);
      }
    }    
  
  dart_exit();
}
