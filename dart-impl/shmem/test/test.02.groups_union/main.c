
#include <unistd.h>
#include <stdio.h>

#include "../test.h"
#include "../utils.h"
#include <dart.h>

// Group test: Union

static int comparInt(const void *a,const void *b){
  const int* aI = (int*)a;
  const int* bI = (int*)b;
  if( *aI == *bI ){
    return 0;
  }else if( *aI < *bI ){
    return -1;
  }else{
    return 1;
  }
}

int test_union(int* a, int size_a, dart_group_t *g_a,
               int* b, int size_b, dart_group_t *g_b,
               int* ex, int size_ex, dart_group_t *g_res);


int main(int argc, char* argv[]){
  size_t size;
  dart_unit_t myid;
  
  CHECK(dart_init(&argc, &argv));
  CHECK(dart_myid(&myid));
  CHECK(dart_size(&size));
  
  dart_group_t *g_a, *g_b, *g_res;
  size_t gsize;
  CHECK(dart_group_sizeof(&gsize)); //returns size of the group datatype 
  //(similiar to sizeof(int))
  fprintf(stderr, "Unit %d: a group object is %d bytes big\n",myid, gsize); 
  
  g_a = malloc(gsize);
  g_b = malloc(gsize);
  g_res = malloc(gsize);

  //normal union
  int a1[3] = {0,2,4};
  int b1[6] = {17,0,4,1,5,33};
  int c1[7] = {0,1,2,4,5,17,33};
  EXPECT_TRUE( test_union( a1, 3, g_a,
                           b1, 6, g_b,
                           c1, 7, g_res));
  
  // union with no intersection 
  int a2[3] = {0,1,2};
  int b2[3] = {3,4,5};
  int c2[6] = {0,1,2,3,4,5};
  EXPECT_TRUE( test_union( a2, 3, g_a, 
                           b2, 3, g_b,
                           c2, 6, g_res));

  
  //union with equal groups,
  int a3[3] = {0,1,2};
  int b3[3] = {0,1,2};
  int c3[3] = {0,1,2};
  EXPECT_TRUE( test_union( a3, 3, g_a, 
                           b3, 3, g_b,
                           c3, 3, g_res));

  //union with neutral group
  int a4[3] = {0,1,2};
  int b4[0];
  int c4[3] = {0,1,2};
  EXPECT_TRUE( test_union( a4, 3, g_a, 
                           b4, 0, g_b,
                           c4, 3, g_res));
  
  //test if false unions are detected
  int a5[4] = {0,1,2,3};
  int b5[4] = {0,1,2};
  int c5[4] = {0,1,3};
  EXPECT_FALSE( test_union( a5, 4, g_a,
                            b5, 4, g_b,
                            c5, 4, g_res));
 

  // all tests finished 
  free(g_a);
  free(g_b);
  free(g_res); 
  CHECK(dart_exit());
}

// input arrays a and b, size of a and b, groups for a and b (both initialized)
// expected output array ex, size of expected output array, and group for the result
int test_union(int* a, int size_a, dart_group_t *g_a,
               int* b, int size_b, dart_group_t *g_b,
               int* ex, int size_ex, dart_group_t *g_res){
  int i, j;
  int size_res;
  CHECK(dart_group_init(g_a));
  CHECK(dart_group_init(g_b));
  CHECK(dart_group_init(g_res)); 

  for(i=0; i < size_a; i++){
    CHECK(dart_group_addmember( g_a, a[i]));
  }
  for(i=0; i < size_b; i++){
    CHECK(dart_group_addmember( g_b, b[i]));
  }
  CHECK(dart_group_union( g_a, g_b, g_res));
  
  CHECK(dart_group_size(g_res, &size_res));
  int* res=malloc(sizeof(int)* size_res);
  CHECK(dart_group_getmembers(g_res, res));
  
  if(size_res != size_ex){
    return 0;
  }
  qsort( res, size_res, sizeof(int), comparInt);
  qsort( ex, size_ex, sizeof(int), comparInt);
  for( i = 0; i < size_ex; i++) {
    if(res[i] != ex[i]){
      return 0;
    }
  }
  
  CHECK(dart_group_fini(g_a));  
  CHECK(dart_group_fini(g_b));  
  CHECK(dart_group_fini(g_res));  

  free(res); 
  return 1;
}
