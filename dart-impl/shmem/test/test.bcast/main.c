
#include <stdio.h>
#include <dart.h>

int main(int argc, char* argv[])
{
  dart_unit_t myid;
  size_t size;
  int buf=0;

  dart_init(&argc, &argv);

  dart_myid(&myid);
  dart_size(&size);

  if( myid==0 ) {
    buf=42;
  }
  dart_bcast(&buf, sizeof(int), 0, DART_TEAM_ALL);

  fprintf(stderr, "Hello World, I'm %d of %d -- received %d\n",
	  myid, size, buf);


  dart_exit();
}
