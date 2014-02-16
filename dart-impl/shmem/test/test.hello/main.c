
#include <stdio.h>
#include <dart.h>

int main(int argc, char* argv[])
{
  dart_unit_t myid;
  size_t size;

  dart_init(&argc, &argv);

  dart_myid(&myid);
  dart_size(&size);

  fprintf(stderr, "Hello World, I'm unit %d of %d\n",
	  myid, size);
  
  if( myid==0 ) 
    sleep(1.0);

  dart_barrier(DART_TEAM_ALL);

  

  dart_exit();
}
