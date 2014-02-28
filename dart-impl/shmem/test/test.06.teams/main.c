
#include <unistd.h>
#include <stdio.h>

#include <dart.h>
#include "../utils.h"

#define REPEAT 100

dart_ret_t split_even_odd_by_local_ids(dart_team_t teamin, 
				       dart_team_t *teameven, 
				       dart_team_t *teamodd)
{
  char buf[200];
  
  dart_group_t *gin;
  dart_group_t *geven;  
  dart_group_t *godd;  
  size_t gsize;

  dart_group_sizeof(&gsize);
  gin   = malloc(gsize);
  geven = malloc(gsize);
  godd  = malloc(gsize);
  
  CHECK(dart_group_init(gin));
  CHECK(dart_group_init(geven));
  CHECK(dart_group_init(godd));
  
  CHECK(dart_team_get_group(teamin, gin));

  /*  
  GROUP_SPRINTF(buf, gin);
  fprintf(stderr, "Group to split: %s\n", buf);
  */

  size_t insize;
  CHECK(dart_team_size(teamin, &insize));
  //  fprintf(stderr, "Teamsize: %d\n", insize);
  
  for(int i=0; i<insize; i++ ) {
    dart_unit_t globid;
    CHECK(dart_team_unit_l2g(teamin, i, &globid));
    if(i%2==0) {
      // add to even team
      CHECK(dart_group_addmember(geven, globid));
    } else {
      // add to odd team
      CHECK(dart_group_addmember(godd, globid));
    }
  }
  
  /*
  GROUP_SPRINTF(buf, geven);
  fprintf(stderr, "Group even: %s\n", buf);
  */  

  /*
  GROUP_SPRINTF(buf, godd);
  fprintf(stderr, "Group odd: %s\n", buf);
  
  */

  CHECK(dart_team_create(teamin, geven, teameven));
  CHECK(dart_team_create(teamin, godd, teamodd));

  /*
  GROUP_SPRINTF(buf, godd);
  fprintf(stderr, "Group: %s\n", buf);
  */

  free(geven); 
  free(godd);
  free(gin);
  
  return DART_OK;
} 


void recursive_split(int level, 
		     dart_team_t tin)
{
  size_t insize;
  dart_unit_t oldid;
  dart_team_t team1, team2;

  fprintf(stderr, "splitting team %d on level %d\n", tin, level);

  dart_team_size(tin, &insize);
  dart_team_myid(tin, &oldid);

  if( insize<2 ) 
    return;

  team1=DART_TEAM_ALL;
  team2=DART_TEAM_ALL;
  split_even_odd_by_local_ids(tin, &team1, &team2);

  dart_unit_t id;

  
  if( dart_team_myid(team1, &id)==DART_OK ) {
    //  if( level<3 ) {
      recursive_split(level+1, team1);
      //}
  }
  
  
  if( dart_team_myid(team2, &id)==DART_OK ) {
    //if( level<3 ) {
      recursive_split(level+1, team2);
      //}
  }

  dart_barrier(tin);
}



int main(int argc, char* argv[])
{
  dart_unit_t myid;
  size_t size;
  
  CHECK(dart_init(&argc, &argv));
  
  CHECK(dart_myid(&myid));
  CHECK(dart_size(&size));
  
  fprintf(stderr, "Hello World, I'm %d of %d\n",
	  myid, size);

  recursive_split(1, DART_TEAM_ALL);

  CHECK(dart_exit());
}
