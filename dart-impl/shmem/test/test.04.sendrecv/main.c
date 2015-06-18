
#include <unistd.h>
#include <stdio.h>
#include <dart.h>

#include "../utils.h"

#define MSGLEN  800000
#define REPEAT  1000

int dart_shmem_send(void *buf, size_t nbytes, 
		    dart_team_t teamid, dart_unit_t dest);
int dart_shmem_recv(void *buf, size_t nbytes,
		    dart_team_t teamid, dart_unit_t source);

int main(int argc, char* argv[])
{
  int i;
  size_t size;
  dart_unit_t myid;
  char *buf;
  double tstart, tstop;

  CHECK(dart_init(&argc, &argv));

  CHECK(dart_myid(&myid));
  CHECK(dart_size(&size));

  fprintf(stderr, "Hello World, I'm %d of %d\n",
	  myid, size);

  buf = (char*) malloc(MSGLEN);
  if( !buf ) {
    fprintf(stderr, "malloc failed!\n");
    CHECK(dart_exit());
    return 0;
  }

  if( size!=2 ) {
    if( myid==0 )  {
      fprintf(stderr, 
	      "This program must be run with exactly 2 processes\n");
    }
    
    CHECK(dart_exit());
    return 0;
  }
  
  TIMESTAMP(tstart);
  for( i=0; i<REPEAT; i++ ) 
    {
      buf[MSGLEN-1]=0;
      if( myid==0 ) {
	buf[MSGLEN-1]=42;
	dart_shmem_send(buf, MSGLEN, DART_TEAM_ALL, 1);
      }
      else {
	dart_shmem_recv(buf, MSGLEN, DART_TEAM_ALL, 0);
	char val = buf[MSGLEN-1];
	//	fprintf(stderr, "Received the following: '%d'\n", val);
      }
    }
  TIMESTAMP(tstop);

  double vol = 1.0e-6 * ((double)MSGLEN) * ((double)REPEAT);

  dart_barrier(DART_TEAM_ALL);
  if(myid==0 ) {
    fprintf(stderr, "Transferred %.2f MB in %.3f secs\n", 
	    vol, (tstop-tstart));
  }
  
  CHECK(dart_exit());
}
