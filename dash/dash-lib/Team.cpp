
#include <stdio.h>
#include <vector>
#include <list>
#include <unistd.h>
#include <memory>

#include "Team.h"

namespace dash 
{
Team Team::m_team_all{DART_TEAM_ALL, nullptr};
Team Team::m_team_null{DART_TEAM_NULL, nullptr};
};



dash::Team& dash::Team::split(unsigned nParts) {
  dart_group_t *group;
  dart_group_t *gout[nParts];
  size_t size;
  char buf[200];
  
  dart_group_sizeof(&size);
  
  group = (dart_group_t*) malloc(size);
  for(int i=0; i<nParts; i++ ) {
    gout[i] = (dart_group_t*) malloc(size);
  }
  
  Team *result = &(dash::Team::Null());

  if( this->size()<=1 ) {
    return *result;
  }
  
  dart_group_init(group);
  dart_team_get_group(m_dartid, group);
  dart_group_split(group, nParts, gout);
  /*
    GROUP_SPRINTF(buf, group);
    fprintf(stderr, "[%d]: Splitting team[%d]: %s\n", 
    dash::myId(), m_dartid, buf);
  */
  
  dart_team_t oldteam = m_dartid;
  
  for(int i=0; i<nParts; i++) {
    dart_team_t newteam;
    
    dart_team_create(oldteam, gout[i], &newteam);
    
    /*
      GROUP_SPRINTF(buf, gout[i]);
      fprintf(stderr, "[%d] New subteam[%d]: %s\n", 
      myId(), newteam, buf);
    */
    if(newteam!=DART_TEAM_NULL) {
      result=new Team(newteam, this, i);
    }
    
    /*    
	  GROUP_SPRINTF(buf, gout[i]);
	  fprintf(stderr, "%d: %s\n", i, buf);
    */
  }

  return *result;
}
