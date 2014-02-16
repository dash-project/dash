
#include <unistd.h>
#include <stdio.h>
#include <dart.h>

int main(int argc, char* argv[])
{
  dart_unit_t myid;
  size_t size;
  
  dart_init(&argc, &argv);
  
  dart_myid(&myid);
  dart_size(&size);
  
  fprintf(stderr, "Hello World, I'm %d of %d\n",
	  myid, size);
  
  dart_group_t *g1;  
  size_t gsize;
  dart_group_sizeof(&gsize);
  g1 = malloc(gsize);
  dart_group_init(g1);

  int i;
  for( i=0; i<size; i+=2 ) {
    dart_group_addmember(g1, i);
  }

  dart_team_t even;
  for( i=0; i<100; i++ ) {
    dart_team_create(DART_TEAM_ALL, g1, &even);

    if( myid==0 ) {
      fprintf(stderr, "new team id is %d\n", even);
    }
     
    dart_team_destroy(even);
  }


  dart_exit();
}
