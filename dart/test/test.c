
#include <stdio.h>
#include <stdlib.h>

#include "dart.h"

void test_gptr();


int main( int argc, char* argv[])
{
  char buf[100];
  
  fprintf(stderr, "This is DART %s (build date: %s)\n", 
	  DART_VERSION_STR, DART_BUILD_STR);
  
  if( dart_init(&argc, &argv)!=DART_OK ) {
    fprintf(stderr, "Error starting DART\n");
    exit(1);
  }

  size_t sz;
  dart_group_t *group;
  dart_group_sizeof(&sz);

  fprintf(stderr, "A DART group object is %zu bytes\n", sz);
  group = (dart_group_t*) malloc(sz);
  dart_group_init(group);

  int iam;
  size_t size;
  dart_myid(&iam);
  dart_size(&size);

  fprintf(stderr, "I'm %d of %zu in this DART world\n",
	  iam, size);

  dart_team_t newteam;
  dart_team_create(DART_TEAM_ALL, group, &newteam); 

  dart_gptr_t ptr;
  fprintf(stderr, "A DART global pointer is %zu bytes\n", 
	  sizeof(dart_gptr_t));

  
  dart_handle_t hdle;
  fprintf(stderr, "A DART handle is %zu bytes\n", 
	  sizeof(dart_handle_t));  

  
  ptr = DART_GPTR_NULL;
  dart_get_nb(buf, ptr, 100, &hdle);
  dart_wait(hdle);

  dart_team_memalloc_aligned(newteam, 100, &ptr);


  dart_lock_t lock;

  dart_team_lock_init(newteam, &lock);


  dart_lock_acquire(lock);

  test_gptr();
}


void test_gptr()
{
  int localvar;
  dart_gptr_t ptr = DART_GPTR_NULL;
  dart_gptr_t ptr2;

  ptr2 = DART_GPTR_NULL;
  
  if( DART_GPTR_ISNULL(ptr) ) {
    fprintf(stderr, "ptr is a nullpoitner\n");
  }
  if( DART_GPTR_ISNULL(ptr2) ) {
    fprintf(stderr, "ptr2 is a nullpoitner\n");
  }

  DART_GPTR_SETADDR(ptr, &localvar);
  DART_GPTR_SETOFFS(ptr, 234234);

  if( DART_GPTR_EQUAL(ptr,ptr2) ) {
    fprintf(stderr, "ptr and ptr2 are the same (they shouldn't be!)\n");
  }

  ptr2=ptr;
  if( DART_GPTR_EQUAL(ptr,ptr2) ) {
    fprintf(stderr, "ptr and ptr2 are the same (they should be!)\n");
  }



}
