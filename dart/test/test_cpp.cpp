
#include <stdio.h>
#include <stdlib.h>

#include "dart.h"

int main( int argc, char* argv[])
{
  fprintf(stderr, "This is DART %s (%s)\n", 
	  DART_VERSION_STR, DART_BUILD_STR);

  dart_group_t *group;
  size_t sz;
  dart_group_sizeof(&sz);

  fprintf(stderr, "A dart group object is %d bytes big\n", sz);

  group = (dart_group_t*) malloc(sz);
  dart_group_init(group);

  dart_gptr_t ptr;

  fprintf(stderr, "A dart global pointer is %d bytes big\n", 
	  sizeof(dart_gptr_t));

}
