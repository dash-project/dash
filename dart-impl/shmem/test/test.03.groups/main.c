
#include <unistd.h>
#include <stdio.h>

#include "../utils.h"
#include <dart.h>

int test_intersect( dart_group_t *g1, 
		    dart_group_t *g2, 
		    dart_group_t *g3 );

int test_union( dart_group_t *g1, 
		dart_group_t *g2, 
		dart_group_t *g3 );


int main(int argc, char* argv[])
{
  size_t size;
  dart_unit_t myid;
  int pass;

  CHECK(dart_init(&argc, &argv));
  
  CHECK(dart_myid(&myid));
  CHECK(dart_size(&size));
  
  fprintf(stderr, "Hello World, I'm %d of %d\n",
	  myid, size);
  
  dart_group_t *g1, *g2, *g3;
  size_t gsize;
  CHECK(dart_group_sizeof(&gsize));

  fprintf(stderr, "Unit %d: a group object is %d bytes big\n", 
	  myid, gsize);

  g1 = malloc(gsize);
  g2 = malloc(gsize);
  g3 = malloc(gsize);

  pass=1;
  pass = pass && test_union(g1, g2, g3);
  pass = pass && test_intersect(g1, g2, g3);

  fprintf(stderr, "Unit %d: test %s\n", 
	  myid, pass?"PASSED":"FAILED");

  CHECK(dart_exit());
}


int test_union( dart_group_t *g1, 
		dart_group_t *g2, 
		dart_group_t *g3 )
{
  int i;
  int mem1[5] = {0,2,5,6,33};
  int mem2[5] = {5,1,7,11,22};
  int mem3[10];
  int mem4[10] = {0,1,2,5,6,7,11,22,33};
  int nmemb;

  CHECK(dart_group_create(g1));
  CHECK(dart_group_create(g2));
  CHECK(dart_group_create(g3));

  for(i=0; i<5; i++) {
    CHECK(dart_group_addmember(g1, mem1[i]));
  }
  for(i=0; i<5; i++) {
    CHECK(dart_group_addmember(g2, mem2[i]));
  }
  
  CHECK(dart_group_union(g1,g2,g3));

  CHECK(dart_group_size(g3, &nmemb));
  CHECK(dart_group_getmembers(g3, mem3));
  
  int pass=1;
  for(i=0; i<nmemb; i++ ) {
    // fprintf(stderr, "%d =?= %d\n", mem3[i], mem4[i]);
    if( mem3[i]!=mem4[i] ) pass=0;
  } 

  CHECK(dart_group_destroy(g1));
  CHECK(dart_group_destroy(g2));
  CHECK(dart_group_destroy(g3));
  
  return pass;
}


int test_intersect( dart_group_t *g1, 
		    dart_group_t *g2, 
		    dart_group_t *g3 )
{
  int i;
  int mem1[5] = {0,2,5,6,33};
  int mem2[5] = {5,1,7,11,22};
  int mem3[10];
  int mem4[10] = {5};
  int nmemb;

  CHECK(dart_group_create(g1));
  CHECK(dart_group_create(g2));
  CHECK(dart_group_create(g3));

  for(i=0; i<5; i++) {
    CHECK(dart_group_addmember(g1, mem1[i]));
  }
  for(i=0; i<5; i++) {
    CHECK(dart_group_addmember(g2, mem2[i]));
  }
  
  CHECK(dart_group_intersect(g1,g2,g3));

  CHECK(dart_group_size(g3, &nmemb));
  CHECK(dart_group_getmembers(g3, mem3));

  int pass=1;
  for(i=0; i<nmemb; i++ ) {
    // fprintf(stderr, "%d =?= %d\n", mem3[i], mem4[i]);
    if( mem3[i]!=mem4[i] ) pass=0;
  } 

  CHECK(dart_group_destroy(g1));
  CHECK(dart_group_destroy(g2));
  CHECK(dart_group_destroy(g3));

  return pass;
}
