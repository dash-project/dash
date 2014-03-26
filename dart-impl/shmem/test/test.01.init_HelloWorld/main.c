
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include "../utils.h"
#include <dart.h>

// #define NASTY

int main(int argc, char* argv[])
{
  char buf[80];
  dart_unit_t myid;
  size_t size;
  pid_t pid;

  CHECK(dart_init(&argc, &argv));

  CHECK(dart_myid(&myid));
  CHECK(dart_size(&size));

  gethostname(buf, 80);
  pid = getpid();

#ifdef NASTY
  if( myid==0 ) {
    exit(1);
  }
#endif
  
  fprintf(stderr, 
	  "Hello World, I'm unit %d of %d, pid=%d host=%s\n",
	  myid, size, pid, buf);

  CHECK(dart_exit());
}
