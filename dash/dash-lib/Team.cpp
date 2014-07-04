
#include "Team.h"
#include "Array.h"


namespace dash 
{
  Team TeamAll(DART_TEAM_ALL, 0);
  Team TeamNull(DART_TEAM_NULL, 0);
};

dash::Team::Team(dart_team_t teamid, 
		 Team* parent, unsigned pos) {
  m_parent   = parent;
  m_dartid   = teamid;
  m_position = pos;
}

void dash::Team::barrier() {
  dart_barrier(m_dartid);
}

size_t dash::Team::myid() const {
  dart_unit_t res;
  dart_team_myid(m_dartid, &res);
  return res;
}

size_t dash::Team::size() const {
  size_t size;
  dart_team_size(m_dartid, &size);
  return size;
}

unsigned dash::Team::position() const {
  return m_position;
}

dash::Team& dash::Team::parent() const {
  if( m_parent!=0 ) {
    return (*m_parent);
  } 
  return dash::TeamNull;
}

dash::Team& dash::Team::split( unsigned nParts) {
  dart_group_t *group;
  dart_group_t *gout[nParts];
  size_t size;
  char buf[200];

  dart_group_sizeof(&size);

  group = (dart_group_t*) malloc(size);
  for(int i=0; i<nParts; i++ ) {
    gout[i] = (dart_group_t*) malloc(size);
  }

  Team *result = &(dash::TeamNull);

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

    Team *newTeam = new Team(newteam, this, i);
    if(newteam!=DART_TEAM_NULL) {
      result=newTeam;
    }
    
    /*    
	  GROUP_SPRINTF(buf, gout[i]);
	  fprintf(stderr, "%d: %s\n", i, buf);
    */
  }

  return *result;
}
