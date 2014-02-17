#ifndef DART_TEAMS_IMPL_H_INCLUDED
#define DART_TEAMS_IMPL_H_INCLUDED

#include <unistd.h>
#include "dart_types.h"
#include "dart_team_group.h"
#include "dart_groups_impl.h"

#include "extern_c.h"
EXTERN_C_BEGIN

// max number of simultaneously existing teams
#define MAXNUM_TEAMS     64

enum state_enum
{
  VALID,
  NOT_INITIALIZED
};

struct team_impl_struct 
{
  dart_team_t teamid;
  dart_unit_t myid;

  //KF 
  enum state_enum state;

  // the location of this team's barrier in the syncarea
  int syncslot;
  
  // the team members;
  dart_group_t group;
  
  // associated mempools (may be DART_MEMPOOL_NULL)
  // KF dart_mempool mempools[2];
  // KF int unique_id;
};


#define SLOT_IS_VALID(slot_) \
  ((0<=slot_) && (slot_<MAXNUM_TEAMS))


// create a new team - called only once by the master
// - returns the new teams' syncslot if succesful
// - sets the new team id if succesful
int dart_shmem_team_new(dart_team_t *team, 
			size_t tsize );

// init the local data structures associated with a team
dart_ret_t dart_shmem_team_init(dart_team_t team,
				dart_unit_t myid, size_t tsize );

dart_ret_t dart_shmem_team_delete(dart_team_t team,
				  dart_unit_t myid, size_t tsize );

dart_ret_t dart_shmem_team_valid(dart_team_t team);


EXTERN_C_END

#endif /* DART_TEAMS_IMPL_H_INCLUDED */
