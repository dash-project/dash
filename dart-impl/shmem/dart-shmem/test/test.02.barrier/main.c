
#include <unistd.h>
#include <stdio.h>

#include "../utils.h"
#include <dart.h>

#define NUMBARR 1000

int main(int argc, char* argv[])
{
  int i;
  dart_unit_t myid;
  size_t size;
  double tstart, tstop;

  CHECK(dart_init(&argc, &argv));

  CHECK(dart_myid(&myid));
  CHECK(dart_size(&size));

  fprintf(stderr, "Hello World, I'm %d of %d\n",
	  myid, size);

  if( myid==0 ) {
    sleep(1.0);
  }
  CHECK(dart_barrier(DART_TEAM_ALL));

  fprintf(stderr, "Unit %d after barrier!\n", myid);

  if( myid==0 ) {
    fprintf(stderr, "Doing %d barriers now...\n", NUMBARR);
  }

  TIMESTAMP(tstart);
  for(i=0; i<NUMBARR; i++ ) {
    CHECK(dart_barrier(DART_TEAM_ALL));
  }
  TIMESTAMP(tstop);

  if( myid==0 ) {
    fprintf(stderr, "Done in %f secs!\n", tstop-tstart);
  }

  CHECK(dart_exit());
}
