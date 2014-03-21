
#include <unistd.h>
#include <stdio.h>

#include <dart.h>
#include "../utils.h"

#include "teams.h"

namespace dash 
{
  Team all(DART_TEAM_ALL, 0);
  Team null(DART_TEAM_NULL, 0);
};

dash::Team::Team(dart_team_t teamid, 
		 Team* parent, unsigned p) {
  parentTeam=parent;
  teamId=teamid;
  pos=p;
}

void dash::Team::barrier() {
  dart_barrier(teamId);
}

dash::Team* dash::Team::split( unsigned nParts) {
  dart_group_t *group;
  dart_group_t *gout[nParts];
  size_t size;
  char buf[200];

  CHECK(dart_group_sizeof(&size));

  group = (dart_group_t*) malloc(size);
  for(int i=0; i<nParts; i++ ) {
    gout[i] = (dart_group_t*) malloc(size);
  }

  Team *result = &(dash::null);

  dart_group_init(group);
  dart_team_get_group(teamId, group);
  dart_group_split(group, nParts, gout);

  /*
  GROUP_SPRINTF(buf, group);
  fprintf(stderr, "[%d]: Splitting team[%d]: %s\n", 
	  dash::myId(), teamId, buf);
  */
  
  dart_team_t oldteam = teamId;
  
  for(int i=0; i<nParts; i++) {
    dart_team_t newteam;
    
    dart_team_create(oldteam, gout[i], &newteam);

    /*
    GROUP_SPRINTF(buf, gout[i]);
    fprintf(stderr, "[%d] New subteam[%d]: %s\n", 
	    myId(), newteam, buf);
    */

    Team *newTeam = new Team(newteam, this, i);
    if(newteam!=DART_TEAM_NULL) {
      result=newTeam;
    }
       
    /*    GROUP_SPRINTF(buf, gout[i]);
	  fprintf(stderr, "%d: %s\n", i, buf);
    */
  }

  return result;
}

size_t dash::Team::getSize() const {
  size_t size;
  dart_team_size(teamId, &size);
  return size;
}

unsigned dash::Team::getPos() const {
  return pos;
}

unsigned dash::Team::myId() const {
  dart_unit_t myId;
  dart_team_myid(teamId, &myId);
  return myId;
}

unsigned dash::myId() {
  dart_unit_t myId;
  dart_myid(&myId);
  return myId;
} 


int main(int argc, char* argv[])
{
  dart_unit_t myid;
  size_t size;

  CHECK(dart_init(&argc, &argv));

  dash::Team &t = dash::all;

  t.barrier();

  dash::Team *sub = t.split(2);


  if( sub->myId()==0 ) {
    fprintf(stderr, "[%d] I'm the master of a new team at pos: %d!\n",
	    dash::myId(), sub->getPos());
  }

  if( sub->getPos()==0 ) {
    sleep(2.0);

    fprintf(stderr, "I'm %d: before second split\n", 
	    dash::myId());
    dash::Team *sub2 = sub->split(2);

    sub2->barrier();
  }
    

#if 0

    sub->barrier();

    /*

    */



    /*

    */

    sub->barrier();
  }
#endif 

  //  fprintf(stderr, "%d after barrier\n", t.myId());
  
  CHECK(dart_exit());
}
