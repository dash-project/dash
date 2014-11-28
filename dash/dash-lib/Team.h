#ifndef TEAMS_H_INCLUDED
#define TEAMS_H_INCLUDED

#include <vector>

#include "dart.h"

namespace dash
{

class Team
{
  template< class U> friend class Array;
  template< class U> friend class GlobPtr;
  template< class U> friend class GlobRef;

private:
  dart_team_t         m_dartid;
  Team*               m_parent;
  size_t              m_position;
  std::vector<Team*>  m_children;

 public:
  // the only way to create a new team is by splitting an existing
  // team. The resulting hierarchy is maintained in parent and child
  // pointers. A split is a non-overlapping partitioning of the parent
  // team, therefore a unit is a member of exactly one subteam. A
  // split can result in one or more subteams being NULL
  Team() = delete;
  Team(Team const&) = delete;

public:
  // no default and no copy constructor!
  // the only way to construct a team is by 
  // passing a dart team ID (may be NULL or ALL)
  Team(dart_team_t teamID, 
       Team *parent, unsigned pos=0);

public:
  bool operator==(const Team& rhs) {
    return m_dartid==rhs.m_dartid;
  }
  bool operator!=(const Team& rhs) {
    return !(operator==(rhs));
  }

public:
  // split a team into n parts
  Team& split(unsigned int n);
  Team& parent() const;

  size_t size() const; // number of units in this team
  size_t myid() const; // calling unit's local id
  
  size_t level() const; // TeamAll has level 0
  size_t position() const; 

  void barrier();

#if 0
  dart_team_t getTeamId() const;
  bool isEmpty();
#endif

};

extern Team TeamAll;
extern Team TeamNull;

}

#endif /* TEAMS_H_INCLUDED */
