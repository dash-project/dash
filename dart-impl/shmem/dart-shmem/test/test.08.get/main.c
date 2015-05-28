
#include <unistd.h>
#include <stdio.h>
#include <dart.h>

#include "../utils.h"

#define REPEAT  10

int main(int argc, char* argv[])
{
  int i;
  size_t size;
  dart_unit_t myid;
  double tstart, tstop;

  CHECK(dart_init(&argc, &argv));

  CHECK(dart_myid(&myid));
  CHECK(dart_size(&size));

  fprintf(stderr, "Hello World, I'm %d of %d\n",
	  myid, size);

  if( size!=2 ) {
    if( myid==0 )  {
      fprintf(stderr, 
	      "This program must be run with exactly 2 processes\n");
    }
    
    CHECK(dart_exit());
    return 0;
  }

  dart_gptr_t gptr = DART_GPTR_NULL;
  if(myid==0) {
    char *addr;

    CHECK(dart_memalloc(100, &gptr));
    CHECK(dart_gptr_getaddr(gptr, &addr));
    sprintf(addr, "Message from unit %d: AHOI!", myid);
  }
  CHECK(dart_bcast( &gptr, sizeof(dart_gptr_t), 0, DART_TEAM_ALL ));
  
  TIMESTAMP(tstart);
  for( i=0; i<REPEAT; i++ ) 
    {
      if( myid==1 ) {
	gptr.addr_or_offs.offset+=1;
	char buf[80];
	dart_get_blocking(buf, gptr, 80);
	
	fprintf(stderr, "[%d]: Received the following: '%s'\n",
		myid, buf);
      }
      
      
    }
  TIMESTAMP(tstop);

  CHECK(dart_exit());
}
