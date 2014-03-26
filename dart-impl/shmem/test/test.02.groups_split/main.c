
#include <unistd.h>
#include <stdio.h>

#include "../utils.h"
#include <dart.h>

#define NGROUPS 3


int main(int argc, char* argv[])
{
  size_t size;
  dart_unit_t myid;
  int pass;
  char buf[200];

  CHECK(dart_init(&argc, &argv));
  
  CHECK(dart_myid(&myid));
  CHECK(dart_size(&size));
  
  fprintf(stderr, "Hello World, I'm %d of %d\n",
	  myid, size);
  
  dart_group_t *g1;
  size_t gsize;
  CHECK(dart_group_sizeof(&gsize));

  g1 = malloc(gsize);
  dart_group_t *gout[NGROUPS];
  

  dart_group_init(g1);
  for( int i=0; i<8; i++ ) {
    CHECK(dart_group_addmember(g1, i));
  }
  
  for( int i=0; i<NGROUPS; i++ ) {
    gout[i] = malloc(gsize);
    dart_group_init(gout[i]);
  }

  CHECK(dart_group_split(g1, NGROUPS, gout)); 
  
  int i;
  for( i=0; i<NGROUPS; i++ ) {
    GROUP_SPRINTF(buf, gout[i]);
    fprintf(stderr, "%d: %s\n", i, buf);
  }

  free(g1);
  CHECK(dart_exit());
}


