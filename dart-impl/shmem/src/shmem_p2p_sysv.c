
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>

#include <dash/dart/shmem/shmem_p2p_if.h>
#include <dash/dart/shmem/sysv/shmem_p2p_sysv.h>
#include <dash/dart/shmem/shmem_logger.h>
#include <dash/dart/shmem/shmem_barriers_if.h>

#ifdef DART_USE_HELPER_THREAD
#include <dash/dart/shmem/dart_helper_thread.h>
#endif

int dart_shmem_mkfifo(char *pname) {
  if (mkfifo(pname, 0666) < 0)
    {
      ERROR("Error creating fifo: '%s'\n", pname);
      return DART_ERR_OTHER;
    }
  return 0;
}

int dart_shmem_p2p_init(dart_team_t teamid, size_t tsize,
			dart_team_unit_t myid, int ikey ) 
{
  int i, slot;
  char buf[128];
  char key[128];
  
  slot = shmem_syncarea_findteam(teamid);
  sprintf(key, "%s-%d", "sysv", ikey);

  for (i = 0; i < tsize; i++) {
    team2fifos[slot][i].readfrom.id = DART_UNDEFINED_UNIT_ID; 
    team2fifos[slot][i].writeto.id  = DART_UNDEFINED_UNIT_ID;
    team2fifos[slot][i].pname_read  = 0;
    team2fifos[slot][i].pname_write = 0;
  }
  
  // the unit 'myid' is responsible for creating all named pipes
  // from any other unit to 'myid' ('i'->'myid' for all i)
  for (i = 0; i < tsize; i++)
    {
      // pipe for sending from <i> to <myid>
      sprintf(buf, "/tmp/%s-team-%d-pipe-from-%d-to-%d", 
	      key, teamid, i, myid);
      
      team2fifos[slot][i].pname_read = strdup(buf);
      
      DEBUG("creating this pipe: '%s'", team2fifos[slot][i].pname_read);
      dart_shmem_mkfifo(team2fifos[slot][i].pname_read);
      
      // pipe for sending from <myid> to <i>
      // mkpipe will be called on the receiver side for those
      sprintf(buf, "/tmp/%s-team-%d-pipe-from-%d-to-%d", 
	      key, teamid, myid, i);
      
      team2fifos[slot][i].pname_write = strdup(buf);
    }
  return DART_OK;
}


int dart_shmem_p2p_destroy(dart_team_t teamid, size_t tsize,
			   dart_team_unit_t myid, int ikey )
{
  int i, slot;
  char *pname;

  DEBUG("dart_shmem_p2p_destroy called with %d %d %d %d\n",
	teamid, tsize, myid, ikey);

  slot = shmem_syncarea_findteam(teamid);  

  for (i = 0; i < tsize; i++)
    {
      if ((pname = team2fifos[slot][i].pname_read))
	{
	  DEBUG("unlinking '%s'", pname);
	  if(pname && unlink(pname) == -1)
	    ERRNO("unlink '%s'", pname);
	  pname=0;
	}
    }
  return DART_OK;
}

int dart_shmem_send(void *buf, size_t nbytes, 
		    dart_team_t teamid, dart_team_unit_t dest)
{
  int ret, slot;

  slot = shmem_syncarea_findteam(teamid);

  if (team2fifos[slot][dest.id].writeto.id < 0)
    {
      ret = team2fifos[slot][dest.id].writeto.id = 
	open( team2fifos[slot][dest.id].pname_write, O_WRONLY);
      if (ret < 0)
	{
	  fprintf(stderr, "Error sending to %d (pipename: '%s') ret=%d\n",
		  dest, team2fifos[slot][dest.id].pname_write, ret);
	  return -1;
	}
    }
  ret = write(team2fifos[slot][dest.id].writeto.id, buf, nbytes);
  
  return ret;
}

int dart_shmem_sendevt(void *buf, size_t nbytes, 
		       dart_team_t teamid, dart_team_unit_t dest)
{
  int evtfd;
  long long value=42;
  evtfd = shmem_syncarea_geteventfd();
  
  write( evtfd, &value, 8);
  //  fprintf(stderr, "after write\n");
  return nbytes;
}

int dart_shmem_recvevt(void *buf, size_t nbytes,
		       dart_team_t teamid, dart_team_unit_t source)
{
  int evtfd;
  long long value;
  evtfd = shmem_syncarea_geteventfd();

  read( evtfd, &value, 8);
  //  fprintf(stderr, "after read\n");  

  return nbytes;
}

int dart_shmem_recv(void *buf, size_t nbytes,
		    dart_team_t teamid, dart_team_unit_t source)
{
  int offs;
  int ret  = 0;
  int slot = shmem_syncarea_findteam(teamid);
  
  if (team2fifos[slot][source.id].readfrom.id < 0 ) {
    team2fifos[slot][source.id].readfrom.id = 
      open(team2fifos[slot][source.id].pname_read, O_RDONLY);
    if (team2fifos[slot][source.id].readfrom.id < 0 ) {
      fprintf(stderr,
              "Error opening fifo for reading: '%s'\n",
              team2fifos[slot][source.id].pname_read);
      return -999;
    }
  }
  offs = 0; 
  while (offs<nbytes) {
    ret = read(team2fifos[slot][source.id].readfrom.id, 
	       buf+offs, nbytes-offs);
    if (ret < 0) 
      break;
     offs+=ret;
  }
  if (offs != nbytes) {
    ERROR("read only %d bytes error=%s\n", 
	  offs, strerror(errno));
  }
  return (ret != nbytes) ? -999 : 0;
}


int dart_shmem_isend(void *buf, size_t nbytes, 
		     dart_team_t teamid, dart_team_unit_t dest, 
		     dart_handle_t *handle)
{
  int ret;
#ifdef DART_USE_HELPER_THREAD
  work_item_t item;

  item.buf=buf;
  item.nbytes=nbytes;
  item.team=teamid;
  item.unit=dest;
  item.handle=handle;
  
  item.selector = WORK_NB_SEND;

  dart_work_queue_push_item(&item);
  
#else
  ret = dart_shmem_send(buf, nbytes, teamid, dest);
#endif
  return ret;
}

int dart_shmem_irecv(void *buf, size_t nbytes,
		     dart_team_t teamid, dart_team_unit_t source,
		     dart_handle_t *handle)
{
  int ret;
#ifdef DART_USE_HELPER_THREAD
  work_item_t item;

  item.buf=buf;
  item.nbytes=nbytes;
  item.team=teamid;
  item.unit=source;
  item.handle=handle;
  
  item.selector = WORK_NB_RECV;
  
  dart_work_queue_push_item(&item);
  
#else
  ret = dart_shmem_recv(buf, nbytes, teamid, source);
#endif
  return ret;
}
