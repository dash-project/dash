
#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_globmem.h>
#include <dash/dart/if/dart_communication.h>
#include <dash/dart/if/dart_team_group.h>
#include <dash/dart/shmem/shmem_p2p_if.h>
#include <dash/dart/shmem/shmem_logger.h>
#include <dash/dart/shmem/shmem_barriers_if.h>

dart_ret_t dart_barrier(dart_team_t teamid)
{
  dart_ret_t ret;

  if( teamid==DART_TEAM_ALL ) {
    shmem_syncarea_barrier_wait(0);
    ret = DART_OK;
  } else {
    int slot;
    slot = shmem_syncarea_findteam(teamid);
    if( 0<=slot && slot<MAXNUM_TEAMS ) {
      shmem_syncarea_barrier_wait(slot);
      ret = DART_OK;
    } else {
      ret = DART_ERR_NOTFOUND;
    }
  }
  
  return ret;
}

dart_ret_t dart_bcast(
  void *buf,
  size_t nbytes,
  dart_datatype_t dtype, 
  dart_team_unit_t root,
  dart_team_t team)
{
  //SANITY_CHECK_TEAM(team);
  dart_team_unit_t myid;
  size_t size;

  dart_team_unit_t i;
  dart_team_myid(team, &myid);
  dart_team_size(team, &size);

  // TODO: this barrier was necessary to
  // make the bcast test case working reliably
  dart_barrier(team);

  DEBUG("dart_bcast on team %d, root=%d, tsize=%d", team, root, size);
  if( myid.id == root.id ) 
    {
      for(i.id=0; i.id<size; i.id++) {
	if( i.id != root.id ) {
	  DEBUG("dart_bcast sending to %d %d bytes", i, nbytes);
	  dart_shmem_send(buf, nbytes, team, i);
	}
      } 
  } 
  else 
    {
      DEBUG("dart_bcast receiving from %d %d bytes", root, nbytes);
      dart_shmem_recv(buf, nbytes, team, root);
    }

  // TODO: this barrier was necessary to
  // make the bcast test case working reliably
  dart_barrier(team);

  return DART_OK;
}

dart_ret_t dart_scatter(
  const void *sendbuf,
  void *recvbuf,
  size_t nbytes,
  dart_datatype_t dtype,
  dart_team_unit_t root,
  dart_team_t team) DART_NOTHROW
{
  //SANITY_CHECK_TEAM(team);
  dart_team_unit_t myid;
  size_t size;
  dart_team_unit_t i;
  //explicit casts to avoid warnings!! carefully double check please!
  char* sbuf = (char*)sendbuf;
  char* rbuf = (char*)recvbuf;

  dart_team_myid( team, &myid);
  dart_team_size( team, &size);
  
  DEBUG("dart_gather on team %d, root=%d, tsize=%d", team,root,size);
  if( myid.id == root.id){
    for( i.id = 0; i.id < size; i.id++){
      if( i.id != root.id){
        DEBUG("dart_scatter sending to %d %d bytes",i,nbytes);
        dart_shmem_send(&sbuf[nbytes*i.id],nbytes,team,i);
      }else{
        memcpy(&sbuf[nbytes*i.id],rbuf,nbytes);
      }
    }
  }else{
    DEBUG("dart_scatter receiving from %d %d bytes",root, nbytes);
    dart_shmem_recv(rbuf,nbytes,team,root);
  }
  return DART_OK;
}

dart_ret_t dart_gather(
  const void *sendbuf,
  void *recvbuf,
  size_t nbytes,
  dart_datatype_t dtype,
  dart_team_unit_t root,
  dart_team_t team)
{
  //SANITY_CHECK_TEAM(team);
  dart_team_unit_t myid;
  size_t size;
  dart_team_unit_t i;
  //explicit casts to avoid warnings!! carefully double check please!
  char* sbuf=(char*)sendbuf;
  char* rbuf=(char*)recvbuf;

  dart_team_myid(team, &myid);
  dart_team_size(team, &size);
  DEBUG("dart_gather on team %d, root=%d, tsize=%d", team,root,size);
  if( myid.id == root.id ){
    for( i.id = 0; i.id < size; i.id++){
      if(i.id != root.id){
        DEBUG("dart_gather receiving from %d %d bytes", i, nbytes);
        dart_shmem_recv(&rbuf[nbytes*i.id], nbytes, team, i); 
      }else{
        memcpy(&rbuf[nbytes*i.id],sbuf,nbytes);
      }
    }
  }else{
    DEBUG("dart_gather sending to %d %d bytes", root, nbytes);
    dart_shmem_send(sbuf,nbytes,team,root);
  }
  dart_barrier(team);
  return DART_OK;
}
  
dart_ret_t dart_allgather(
  const void *sendbuf,
  void *recvbuf,
  size_t nbytes,
  dart_datatype_t dtype,
  dart_team_t team)
{ 
  dart_team_unit_t root;
  root.id = 0;
#ifdef DART_DEBUG
  size_t size;
  dart_team_size(team, &size);
  DEBUG("dart_allgather on team %d, tsize=%d", team, root, size);
#endif
  dart_gather(sendbuf,recvbuf,nbytes,dtype,root,team);
  dart_bcast(recvbuf,nbytes,dtype,root,team);
  return DART_OK;
}
