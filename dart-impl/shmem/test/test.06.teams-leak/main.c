
#include <unistd.h>
#include <stdio.h>

#include <dart.h>
#include "../utils.h"

#define REPEAT 70

int main(int argc, char* argv[])
{
  dart_unit_t myid;
  size_t size;
  
  CHECK(dart_init(&argc, &argv));
  
  CHECK(dart_myid(&myid));
  CHECK(dart_size(&size));
  
  fprintf(stderr, "Hello World, I'm %d of %d\n",
	  myid, size);

  dart_group_t *group;  
  size_t gsize;
  dart_group_sizeof(&gsize);
  group = malloc(gsize);
  dart_group_create(group);

  int i;
  for( i=0; i<size; i+=1 ) {
    CHECK(dart_group_addmember(group, i));
  }

  dart_team_t newteam;

  for( i=0; i<REPEAT; i++ ) {
    dart_unit_t id;

    CHECK(dart_team_create(DART_TEAM_ALL, 
			   group, &newteam));
    
    if( dart_team_myid(newteam, &id)==DART_OK ) {
      if( id==0 ) {
	fprintf(stderr, "New even team: %5d new master id in old team: %d\n",
		newteam, myid);
      }
      //CHECK(dart_team_destroy(newteam));
    }
  }

  CHECK(dart_exit());
}
