#ifndef TEAMS_H_INCLUDED
#define TEAMS_H_INCLUDED

#include <vector>

#include "dart.h"

namespace dash
{
  // return the global id 
  unsigned myId(); 

class Team
{
 private:
  dart_team_t teamId;
  Team       *parentTeam;
  unsigned    pos;
  std::vector<Team*> childTeams;

 public:
  // no default and no copy constructor!
  // the only way to construct a team is by 
  // passing a dart team ID (may be NULL or ALL)
  //
  // the only way to create a new team is by splitting an
  // existing team. The resulting hierarchy is maintained in parent and 
  // child pointers. A split is a non-overlapping partitioning
  // of the parent team, therefore a unit is a member of exactly one 
  // subteam. A split can result in one or more subteams being NULL
  Team() = delete;
  Team(Team const&) = delete;
  Team(dart_team_t teamID, 
       Team *parent, unsigned pos=0);

  // split a team into n parts
  Team* split(unsigned int n);

  
  Team* parent();

  size_t    getSize() const;
  unsigned  myUnitId() const;
  unsigned  getPos() const;

  dart_team_t getTeamId() const;

  void barrier();


#if 0 
  bool isEmpty();
#endif
};

extern Team TeamAll;
extern Team TeamNull;

}

#endif /* TEAMS_H_INCLUDED */
