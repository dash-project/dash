
#include <stdio.h>
#include <dart.h>

#include "../utils.h"

#define MSGSIZE 100000000
#define REPEAT  10

int check_bcast(size_t size, dart_unit_t root);

int main(int argc, char* argv[])
{
  dart_unit_t myid;
  size_t size;
  int i, buf=0;

  CHECK(dart_init(&argc, &argv));

  CHECK(dart_myid(&myid));
  CHECK(dart_size(&size));

  if( myid==0 ) {
    buf=42;
  }
  CHECK(dart_bcast(&buf, sizeof(int), 0, DART_TEAM_ALL));

  fprintf(stderr, "Hello World, I'm %d of %d -- received %d\n",
	  myid, size, buf);

  
  for(i=0; i<size; i++ ) {
    check_bcast(MSGSIZE, i);
  }

  CHECK(dart_exit());
}


int check_bcast(size_t size, dart_unit_t root)
{
  int i;
  char *buf;
  double tstart, tstop;

  buf = (char*)malloc(size);

  TIMESTAMP(tstart);
  for( i=0; i<REPEAT; i++ ) {
    CHECK(dart_bcast(buf, size, root, DART_TEAM_ALL));
  }
  TIMESTAMP(tstop);

  free(buf);

  fprintf(stderr, "Did %d bcasts of %d bytes in %.3f secs\n",
	  REPEAT, size, (tstop-tstart));
}
