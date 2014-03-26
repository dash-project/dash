
#include <unistd.h>
#include <stdio.h>

#include "../test.h"
#include "../utils.h"
#include <dart.h>

// Group test: Intersection

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

int test_intersection(int* a, int size_a, dart_group_t *g_a,
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

//normal intersection
  fprintf(stderr,"Testing: normal intersection.\n");
  int a1[3] = {0,2,4};
  int b1[6] = {17,0,4,1,5,33};
  int c1[7] = {0,4};
  EXPECT_TRUE( test_intersection( a1, 3, g_a,
                                  b1, 6, g_b,
                                  c1, 2, g_res));
  
// intersection empty
  fprintf(stderr,"Testing: intersection empty.\n");
  int a2[3] = {0,1,2};
  int b2[3] = {3,4,5};
  int c2[0];
  EXPECT_TRUE( test_intersection( a2, 3, g_a, 
                           b2, 3, g_b,
                           c2, 0, g_res));

  
//intersection with equal groups,
  fprintf(stderr,"Testing: intersection with equal groups.\n");
  int a3[3] = {0,1,2};
  int b3[3] = {0,1,2};
  int c3[3] = {0,1,2};
  EXPECT_TRUE( test_intersection( a3, 3, g_a, 
                           b3, 3, g_b,
                           c3, 3, g_res));

//Intersection with neutral element  
  fprintf(stderr,"Testing: intersection with neutral element.\n");
  // WARNING: g_all ist not the neutral element! 
  // The universe is defined globaly by teams, 
  // but the elements of the groups are not part of this universe!
  dart_group_t *g_all;
  size_t  *g_all_size;
  
  g_all = malloc(gsize);
  CHECK( dart_group_init(g_all));
  CHECK( dart_team_get_group( DART_TEAM_ALL, g_all));
  CHECK( dart_group_size( g_all, g_all_size));
  int* g_all_unitids=malloc(sizeof(int)* *g_all_size);
  CHECK( dart_group_getmembers(g_all,g_all_unitids));
  int a4[3] = {0,1,2};
  // b4 = g_all_unitids with size g_all_size
  int c4[3] = {0,1,2};
  EXPECT_TRUE(test_intersection( a4, 3, g_a, 
                                 g_all_unitids, *g_all_size, g_b,
                                 c4, 3, g_res));
  
  CHECK(dart_group_fini(g_all));  
  free(g_all_unitids);
  free(g_all);
  
//test if false intersections are detected
  fprintf(stderr,"Testing: false intersection.\n");
  int a5[4] = {0,1,2,3};
  int b5[4] = {0,1,2};
  int c5[4] = {0,1,3};
  EXPECT_FALSE( test_intersection( a5, 4, g_a,
                            b5, 4, g_b,
                            c5, 4, g_res));
 

// all tests finished 
  fprintf(stderr,"All intersection tests passed.\n");
  free(g_a);
  free(g_b);
  free(g_res); 
  CHECK(dart_exit());
}

// input arrays a and b, size of a and b, groups for a and b (both initialized)
// expected output array ex, size of expected output array, and group for the result
int test_intersection( int* a, int size_a, dart_group_t *g_a,
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
  CHECK(dart_group_intersect( g_a, g_b, g_res));
  
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
