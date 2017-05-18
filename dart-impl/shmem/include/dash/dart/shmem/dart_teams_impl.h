#ifndef DART_TEAMS_IMPL_H_INCLUDED
#define DART_TEAMS_IMPL_H_INCLUDED

#include <unistd.h>
#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_team_group.h>
#include <dash/dart/shmem/dart_groups_impl.h>
#include <dash/dart/shmem/dart_memarea.h>

#include <dash/dart/shmem/extern_c.h>
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
  dart_team_unit_t myid;

  //KF 
  enum state_enum state;

  // the location of this team's barrier in the syncarea
  int syncslot;
  
  // the team members;
  dart_group_t group;

#if 0 /* TODO: remove me */
  int mempoolid_aligned[MAXNUM_MEMPOOLS];
  int mempoolid_unaligned[MAXNUM_MEMPOOLS];
#endif  

  //  dart_memarea_t mem;
  
};


#define SLOT_IS_VALID(slot_) \
  ((0<=slot_) && (slot_<MAXNUM_TEAMS))


// create a new team - called only once by the master
// - returns the new teams' syncslot if succesful
// - sets the new team id if succesful
int dart_shmem_team_new(dart_team_t *team, 
			size_t tsize );

// init the local data structures associated with a team
dart_ret_t dart_shmem_team_init(dart_team_t team, dart_team_unit_t myid, 
				size_t tsize, const dart_group_t *group);

dart_ret_t dart_shmem_team_delete(dart_team_t team,
				  dart_team_unit_t myid, size_t tsize );

dart_ret_t dart_shmem_team_valid(dart_team_t team);

//dart_memarea_t *dart_shmem_team_get_memarea(dart_team_t team);

EXTERN_C_END

#endif /* DART_TEAMS_IMPL_H_INCLUDED */
