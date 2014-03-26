
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

int test_cpy(	dart_group_t *g1,
		dart_group_t *g2,
		dart_group_t *g3);


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
  pass = pass && test_cpy(g1,g2,g3);

  fprintf(stderr, "Unit %d: test %s\n", 
	  myid, pass?"PASSED":"FAILED");

  CHECK(dart_exit());
}


int test_union( dart_group_t *g1, 
		dart_group_t *g2, 
		dart_group_t *g3 )
{
  int i, j;
  int mem1[5] = {0,2,5,6,33};
  int mem2[5] = {5,1,7,11,22};
  int mem3[10];
  int mem4[10] = {0,1,2,5,6,7,11,22,33};
  int nmemb;

  CHECK(dart_group_init(g1));
  CHECK(dart_group_init(g2));
  CHECK(dart_group_init(g3));

  for(i=0; i<5; i++) {
    CHECK(dart_group_addmember(g1, mem1[i]));
  }
  for(i=0; i<5; i++) {
    CHECK(dart_group_addmember(g2, mem2[i]));
  }
  
  CHECK(dart_group_union(g1,g2,g3));

  CHECK(dart_group_size(g3, &nmemb));
  CHECK(dart_group_getmembers(g3, mem3));
  
  if( nmemb!=9 )
    return 0;

  int pass=1;
  for(i=0; i<nmemb; i++ ) {
    int found=0;
    for( j=0; j<nmemb; j++ ) {
      if( mem3[i]==mem4[j] ) {
	found=1;
	break;
      }
    }
    if(!found) {
      pass=0;
      break;
    }
  } 

  CHECK(dart_group_fini(g1));
  CHECK(dart_group_fini(g2));
  CHECK(dart_group_fini(g3));
  
  return pass;
}


int test_intersect( dart_group_t *g1, 
		    dart_group_t *g2, 
		    dart_group_t *g3 )
{
  int i, j;
  int mem1[5] = {0,2,5,6,33};
  int mem2[5] = {5,1,7,11,22};
  int mem3[10];
  int mem4[10] = {5};
  int mem5[10] = {1,3,4,7,8};
  int nmemb;

  CHECK(dart_group_init(g1));
  CHECK(dart_group_init(g2));
  CHECK(dart_group_init(g3));

  for(i=0; i<5; i++) {
    CHECK(dart_group_addmember(g1, mem1[i]));
  }
  for(i=0; i<5; i++) {
    CHECK(dart_group_addmember(g2, mem2[i]));
  }
  
//normal intersect 
  CHECK(dart_group_intersect(g1,g2,g3));

  CHECK(dart_group_size(g3, &nmemb));
  CHECK(dart_group_getmembers(g3, mem3));

  if( nmemb!=1 )
    return 0;

  int pass=1;
  for(i=0; i<nmemb; i++ ) {
    int found=0;
    for( j=0; j<nmemb; j++ ) {
      if( mem3[i]==mem4[j] ) {
	found=1;
	break;
      }
    }
    if(!found) {
      pass=0;
      break;
    }
  } 

//intersection == empty 
  printf("Intersection==empty\n");
  for(i=0;i<nmemb;i++){
    CHECK(   dart_group_delmember(g2,mem2[i]));
  } 
  for( i=0; i<5; i++){
    CHECK(dart_group_addmember(g2,mem5[i]));
  }
  CHECK(dart_group_intersect(g1,g2,g3));
  dart_group_size(g3,&nmemb);
  if(nmemb!=0){
    pass=0;
  }
  dart_group_getmembers(g3,mem3);
  for(i=0;i<nmemb;i++){
    printf("%d,",mem3[i]);
  }
  printf("\nMembercount:%d\n",nmemb);
  

  CHECK(dart_group_fini(g1));
  CHECK(dart_group_fini(g2));
  CHECK(dart_group_fini(g3));

  return pass;
}

int test_cpy(	dart_group_t *g1,
		dart_group_t *g2,
		dart_group_t *g3)
{
  int pass=1;
  int i, j;
  int mem1[5] = {0,2,5,6,33};
  int mem2[5] = {5,1,7,11,22};
  int mem3[10];
  int mem4[10] = {5};
  int nmemb;
  
  CHECK(dart_group_init(g1));
  CHECK(dart_group_init(g2));
  CHECK(dart_group_init(g3));
  
  for(i=0; i<5; i++) {
    CHECK(dart_group_addmember(g1, mem1[i]));
  }
  size_t size1,size2;
  CHECK(dart_group_copy(g1,g2));
  CHECK(dart_group_size(g1,&size1));
  CHECK(dart_group_size(g2,&size2));
  dart_unit_t unitids1[size1],unitids2[size2]; 
   
  dart_group_getmembers(g1,unitids1);
  dart_group_getmembers(g2,unitids2);
  for(int i;i<size1;i++){
    if(unitids1[i]!=unitids2[i]){
     pass = 0;
    }
  }

  CHECK(dart_group_fini(g1));
  CHECK(dart_group_fini(g2));
  CHECK(dart_group_fini(g3));
  return pass;
}
