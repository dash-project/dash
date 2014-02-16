
#include <stdio.h>
#include <dart.h>

void test_intersect( dart_group_t *g1, 
		     dart_group_t *g2, 
		     dart_group_t *g3 );

void test_union( dart_group_t *g1, 
		 dart_group_t *g2, 
		 dart_group_t *g3 );


int main(int argc, char* argv[])
{
  dart_unit_t myid;
  size_t size;

  dart_init(&argc, &argv);

  dart_myid(&myid);
  dart_size(&size);

  fprintf(stderr, "Hello World, I'm %d of %d\n",
	  myid, size);

  dart_group_t *g1, *g2, *g3;
  size_t gsize;
  dart_group_sizeof(&gsize);

  fprintf(stderr, "A group object is %d bytes big\n", gsize);

  g1 = malloc(gsize);
  g2 = malloc(gsize);
  g3 = malloc(gsize);

  test_union(g1, g2, g3);
  test_intersect(g1, g2, g3);

  dart_exit();
}


void test_union( dart_group_t *g1, 
		 dart_group_t *g2, 
		 dart_group_t *g3 )
{
  int i;
  int mem1[5] = {0,2,5,6,33};
  int mem2[5] = {5,1,7,11,22};
  int mem3[10];
  int mem4[10] = {0,1,2,5,6,7,11,22,33};
  int nmemb;

  dart_group_init(g1);
  dart_group_init(g2);
  dart_group_init(g3);

  for(i=0; i<5; i++) {
    dart_group_addmember(g1, mem1[i]);
  }
  for(i=0; i<5; i++) {
    dart_group_addmember(g2, mem2[i]);
  }
  
  dart_group_union(g1,g2,g3);

  dart_group_size(g3, &nmemb);
  dart_group_getmembers(g3, mem3);
  for(i=0; i<nmemb; i++ ) {
    fprintf(stderr, "%d =?= %d\n", mem3[i], mem4[i]);
  } 

  dart_group_fini(g1);
  dart_group_fini(g2);
  dart_group_fini(g3);
}


void test_intersect( dart_group_t *g1, 
		     dart_group_t *g2, 
		     dart_group_t *g3 )
{
  int i;
  int mem1[5] = {0,2,5,6,33};
  int mem2[5] = {5,1,7,11,22};
  int mem3[10];
  int mem4[10] = {5};
  int nmemb;

  dart_group_init(g1);
  dart_group_init(g2);
  dart_group_init(g3);

  for(i=0; i<5; i++) {
    dart_group_addmember(g1, mem1[i]);
  }
  for(i=0; i<5; i++) {
    dart_group_addmember(g2, mem2[i]);
  }
  
  dart_group_intersect(g1,g2,g3);

  dart_group_size(g3, &nmemb);
  dart_group_getmembers(g3, mem3);
  for(i=0; i<nmemb; i++ ) {
    fprintf(stderr, "%d =?= %d\n", mem3[i], mem4[i]);
  } 

  dart_group_fini(g1);
  dart_group_fini(g2);
  dart_group_fini(g3);
}
