
#include <unistd.h>
#include <stdio.h>

#include <dart.h>
#include "../utils.h"

#define REPEAT 100

int main(int argc, char* argv[])
{
  dart_unit_t myid;
  size_t size;
  
  CHECK(dart_init(&argc, &argv));
  
  CHECK(dart_myid(&myid));
  CHECK(dart_size(&size));
  
  fprintf(stderr, "Hello World, I'm %d of %d\n",
	  myid, size);

  dart_group_t *geven;  
  dart_group_t *godd;  
  size_t gsize;
  dart_group_sizeof(&gsize);
  geven = malloc(gsize);
  godd = malloc(gsize);
  dart_group_init(geven);
  dart_group_init(godd);

  int i;
  for( i=0; i<size; i+=2 ) {
    CHECK(dart_group_addmember(geven, i));
  }
  for( i=1; i<size; i+=2 ) {
    CHECK(dart_group_addmember(godd, i));
  }

  dart_team_t team_even;
  dart_team_t team_odd;

  for( i=0; i<REPEAT; i++ ) {
    dart_unit_t id;

    CHECK(dart_team_create(DART_TEAM_ALL, 
			   geven, &team_even));
    
    CHECK(dart_team_create(DART_TEAM_ALL,
			   godd, &team_odd));

    // only members of the respective teams can call 
    // dart_team_destroy; therefore each unit first has 
    // to find out if it is part of the new team
    // a way to do this is by successfully getting one's
    // team id
    if( dart_team_myid(team_even, &id)==DART_OK ) {
      if( id==0 ) {
	fprintf(stderr, "New even team: %5d new master id in old team: %d\n",
		team_even, myid);
      }
      CHECK(dart_team_destroy(team_even));
    }

    if( dart_team_myid(team_odd, &id)==DART_OK ) {
      if( id==0 ) {
	fprintf(stderr, "New odd  team: %5d new master id in old team: %d\n",
		team_odd, myid);
      }
      CHECK(dart_team_destroy(team_odd));
    }

  }

  CHECK(dart_exit());
}
