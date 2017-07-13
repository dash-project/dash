
#include <stdio.h>

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_team_group.h>
#include <dash/dart/if/dart_globmem.h>
#include <dash/dart/if/dart_communication.h>

#include <dash/dart/shmem/dart_teams_impl.h>
#include <dash/dart/shmem/dart_groups_impl.h>
#include <dash/dart/shmem/dart_shmem.h>
#include <dash/dart/shmem/shmem_p2p_if.h>
#include <dash/dart/shmem/shmem_logger.h>
#include <dash/dart/shmem/shmem_barriers_if.h>

static struct team_impl_struct teams[MAXNUM_TEAMS];

struct newteam_msg
{
  int size;
  int newid;
  int slot;
  dart_team_t teamid;
};

dart_ret_t dart_team_create(
  dart_team_t          oldteamid, 
  const dart_group_t   group, 
  dart_team_t        * newteam)
{
  size_t oldsize, newsize, globalsize;
  dart_team_unit_t oldmyid;
  dart_global_unit_t oldmyid_global;
  dart_global_unit_t newmaster = {0};
  int i_am_member = 0; 
  int i_am_master = 0; 
  dart_global_unit_t i;

  *newteam=DART_TEAM_NULL;

  struct newteam_msg nmsg;

  /* 
     1. sanity check of old team
     2. barrier on old team 
     3. sanity check of group specificiation
        - if group is not empty, it must be the same on all 
	  participating procs
     4. find the master proc in new team 
        (the master identifies itself)
     5. the master calls "dart_shmem_team_new"
     6. the master sends the information on the new team to
        all participants in the new team (using the 
	old team communication infrastructure)
     7. *all members* of the new team call
        "dart_shmem_team_init"
     8. DONE! The new team is ready to be used

  */
  
  // STEP 1
  dart_ret_t ret;
  ret = dart_shmem_team_valid(oldteamid);
  if( ret!=DART_OK ) 
    return ret;

  // STEP 2
  dart_barrier(oldteamid);
  dart_team_size(oldteamid, &oldsize);
  dart_team_myid(oldteamid, &oldmyid);
  dart_team_size(DART_TEAM_ALL, &globalsize);
  
  // get the global old id 
  dart_team_unit_l2g(oldteamid, 
		     oldmyid, &oldmyid_global);
  
  // STEP 3
  // TODO: check santiy of group spec
  // if group is not empty, it must be the same on all 
  // participating processes 
  if( !group ) {
    dart_barrier(oldteamid);
    return DART_OK;
  }

  dart_group_size(group, &newsize);

  // TODO:
  // dart_group_sort

  // STEP 4: find new master 
  int ismember;
  for (i.id = 0; i.id<globalsize; i.id++ ) {
    dart_group_ismember(group, i, &ismember);
    if (ismember) {
      newmaster.id = i.id;
      break;
    }
  }

  if (ismember && oldmyid_global.id == newmaster.id) {
    i_am_master = 1;
  }

  dart_group_ismember(group, oldmyid_global, &i_am_member);

  if (i_am_master ) 
  {
    nmsg.slot = -1;
    nmsg.size = newsize;
    // STEP 5: master calls dart_shmem_team_new
    nmsg.slot = dart_shmem_team_new(
                  &(nmsg.teamid), 
                  newsize);
    if (!SLOT_IS_VALID(nmsg.slot)) {
      ERROR("dart_shmem_team_new failed (slot: %d)", nmsg.slot);
    }
    // STEP 6: send out info to all other members
    nmsg.newid=1;
    for (i.id = 0; i.id < globalsize; i.id++) {
      dart_group_ismember(group, i, &ismember);
      if (ismember && i.id != oldmyid_global.id) {
        // get the local id of our comm partner
        dart_team_unit_t sendto;
        dart_team_unit_g2l(
          oldteamid, 
          i,
          &sendto);
        // note: communication in old team
        dart_shmem_send(
          &nmsg,
          sizeof(struct newteam_msg),
          oldteamid,
          sendto);
        nmsg.newid++;
      }
    }
    // reset id for master to 0
    nmsg.newid = 0;
  } 
  else 
  {
    if (i_am_member) {
      // get the local id of our comm partner
      dart_team_unit_t recvfrom;
      dart_team_unit_g2l(
        oldteamid, 
        newmaster,
        &recvfrom);
      dart_shmem_recv(
        &nmsg,
        sizeof(struct newteam_msg),
        oldteamid,
        recvfrom);
      DEBUG("Received newteam_msg: %d %d %d %d", 
            nmsg.size, nmsg.newid, nmsg.slot, nmsg.teamid);
    }
  }
  // STEP 7: all new members call dart_shmem_team_init
  if (i_am_member) 
  {
    int slot;
    dart_shmem_team_init(
      nmsg.teamid,
      *((dart_team_unit_t*)(&nmsg.newid)), 
      nmsg.size,
      &group);
    slot = shmem_syncarea_findteam(nmsg.teamid);	
    if (SLOT_IS_VALID(slot)) {
      teams[slot].myid.id = nmsg.newid; 
      (*newteam)=nmsg.teamid;
    } else {
      ERROR("dart_shmem_team_init failed (slot: %d)", slot);
    }
  }
  dart_barrier(oldteamid);
  return DART_OK;
}

dart_ret_t dart_team_destroy(dart_team_t * teamid)
{
  size_t size;
  dart_team_unit_t myid;
  
  if( teamid==DART_TEAM_ALL ) {
    // can't delete the default team
    return DART_ERR_INVAL; 
  }
  
  dart_ret_t ret;
  ret = dart_shmem_team_valid(*teamid);
  if( ret!=DART_OK ) {
    fprintf(stderr, "got %d\n", ret);
    return ret;
  }

  dart_barrier(*teamid);

  dart_team_size(*teamid, &size);
  dart_team_myid(*teamid, &myid);
  
  DEBUG("dart_team_destroy team=%d, size=%d, myid=%d", 
	teamid, size, myid);
  
  ret = dart_shmem_team_delete( *teamid, myid, size);
  return ret;
}

dart_ret_t dart_team_myid(dart_team_t teamid, dart_team_unit_t *myid)
{
  dart_ret_t ret;

  if (teamid == DART_TEAM_NULL) {
    return DART_ERR_INVAL;
  }
  if (teamid == DART_TEAM_ALL) 
  {
    ret = DART_OK;
    myid->id = _glob_myid;
  } 
  else 
  {
    int slot;
    slot = shmem_syncarea_findteam(teamid);
    if( SLOT_IS_VALID(slot) ) {
      *myid = teams[slot].myid; 
      ret = DART_OK;
    } else {
      ret = DART_ERR_INVAL;
    }
  }
  return ret;
}

dart_ret_t dart_team_size(dart_team_t teamid, size_t *size)
{
  dart_ret_t ret = DART_ERR_INVAL;
  int slot = -1;
  *size = 0;

  if (teamid == DART_TEAM_NULL) {
    return ret;
  }
  if (teamid == DART_TEAM_ALL) {
    ret   = DART_OK;
    *size = _glob_size;
  } else {
    slot = shmem_syncarea_findteam(teamid);
    if (SLOT_IS_VALID(slot)) {
      dart_group_size(teams[slot].group, size);
      ret = DART_OK;
    } else {
      ret = DART_ERR_INVAL;
    }
  }
  return ret;
}

dart_ret_t dart_myid(dart_global_unit_t *myid)
{
  DART_INIT_CHECK();
  myid->id = _glob_myid;
  return DART_OK;
}

dart_ret_t dart_size(size_t *size)
{
  DART_INIT_CHECK();
  *size = _glob_size;
  return DART_OK;
}

int dart_shmem_team_new(
  dart_team_t *team, 
	size_t tsize )
{
  int slot;
  dart_team_t newteam;
  
  slot = shmem_syncarea_newteam(&newteam, tsize);
  if (SLOT_IS_VALID(slot)) {
    (*team) = newteam;
  } 
  return slot;
}

dart_ret_t dart_shmem_team_init(
  dart_team_t team,
  dart_team_unit_t myid, 
	size_t tsize, 
	const dart_group_t *group)
{
  int slot;
  dart_global_unit_t i;
  if (team == DART_TEAM_ALL)  {
    // init all data structures for all teams
    for (i.id = 0; i.id < MAXNUM_TEAMS; i.id++) {
      teams[i.id].syncslot=-1;
      teams[i.id].state=NOT_INITIALIZED;
    }
    dart_memarea_init();
    slot = 0;
  } else {
    slot = shmem_syncarea_findteam(team);
  }
  
  if (!SLOT_IS_VALID(slot)) {
    return DART_ERR_NOTFOUND;
  }
  
  teams[slot].syncslot=slot;
  teams[slot].teamid=team;
  
  // build the group for this team
  dart_group_create(&(teams[slot].group));
  if( slot==0 && !group ) {
    for( i.id=0; i.id<tsize; i.id++ ) {
      dart_group_addmember(teams[slot].group, i);
    }
  } else  {
    dart_group_clone(*group,
		    &(teams[slot].group));
  }
    
  int shmid = shmem_syncarea_get_shmid();

  // TODO: check return value of below 
  dart_shmem_p2p_init(team, tsize, myid, shmid);

  // --- from here on, we can use 
  //          communication in the new team ---

  if (team == DART_TEAM_ALL) 
  {
    int res;
    res = dart_memarea_create_mempool(
            DART_TEAM_ALL,
            tsize, 
            myid, 
            1024 * 1024,
            0 /* not aligned */
          );
    if (res < 0) {
      return DART_ERR_INVAL;
    }
  }
  teams[slot].state = VALID;
  return DART_OK;
}



dart_ret_t dart_shmem_team_delete(dart_team_t teamid,
				  dart_team_unit_t myid, size_t tsize )
{
  int slot;
  dart_ret_t ret;

  ret = dart_shmem_team_valid(teamid);
  if (ret != DART_OK) { 
    return ret;
  }
  int shmid = shmem_syncarea_get_shmid();
  slot = shmem_syncarea_findteam(teamid);
  if (!SLOT_IS_VALID(slot)) {
    return DART_ERR_INVAL;
  }

#if 0
  dart_memarea_destroy_mempool( &(teams[slot].mem),
				0,
				teamid,
				myid );
#endif 
  
  // todo: check return value of below
  dart_shmem_p2p_destroy(
    teamid,
    tsize,
    myid,
    shmid);
  dart_barrier(teamid);
  if (myid.id == 0) {
    shmem_syncarea_delteam(teamid, tsize);
  }
  return DART_OK;
}

dart_ret_t dart_team_get_group(dart_team_t teamid, dart_group_t *group)
{
  dart_ret_t ret;
  int slot;
  
  if( teamid==DART_TEAM_ALL )  {
    slot=0;
  } 
  else {
    slot = shmem_syncarea_findteam(teamid);
  }
      
  if( SLOT_IS_VALID(slot) ) {
    ret = dart_group_clone( (teams[slot].group), group);
  } else {
    ret = DART_ERR_INVAL;
  }

  return ret;
}

dart_ret_t dart_shmem_team_valid(dart_team_t team)
{
  int i;

  for( i=0; i<MAXNUM_TEAMS; i++ ) {
    if( teams[i].state==VALID && 
	teams[i].teamid==team ) {
      return DART_OK;
    }
  }

  return DART_ERR_NOTFOUND;
}

#if 0
dart_memarea_t *dart_shmem_team_get_memarea(dart_team_t team) 
{
  int i;
  dart_memarea_t *ret=0;
  
  for( i=0; i<MAXNUM_TEAMS; i++ ) {
    if( teams[i].state==VALID && 
	teams[i].teamid==team ) {
      ret = &(teams[i].mem);
      break;
    }
  }

  return ret;
}
#endif

dart_ret_t dart_team_unit_l2g(dart_team_t teamid, 
			      dart_team_unit_t localid,
			      dart_global_unit_t *globalid)
{
  dart_ret_t ret;
  int slot;
  
  if( teamid==DART_TEAM_ALL )  {
    slot=0;
  } else {
    slot = shmem_syncarea_findteam(teamid);
  }
  
  ret=DART_ERR_INVAL;

  if( SLOT_IS_VALID(slot) ) {
    dart_group_t *group;
    group = &(teams[slot].group);
    if (group && (0<=localid.id) && (localid.id<=((*group)->nmem)))
    {
      (*globalid).id = (*group)->l2g[localid.id];
      ret = DART_OK;
    }
  }

  return ret;  
}

dart_ret_t dart_team_unit_g2l(
  dart_team_t teamid, 
  dart_global_unit_t globalid,
  dart_team_unit_t *localid)
{
  dart_ret_t ret;
  int slot;
  
  if( teamid==DART_TEAM_ALL )  {
    slot=0;
  } else {
    slot = shmem_syncarea_findteam(teamid);
  }
  
  ret=DART_ERR_INVAL;
  if( SLOT_IS_VALID(slot) ) {
    dart_group_t *group;
    group = &(teams[slot].group);

    if( group && (0<=globalid.id) ) 
      {
	(*localid).id = (*group)->g2l[globalid.id];
	ret = DART_OK;
      }
  }

  return ret;  
}
