
#include <assert.h>
#include <stdio.h>


#include "dart_initialization.h"
#include "dart_team_group.h"

#include "dart_shmem.h"
#include "shmem_mm_if.h"
#include "dart_teams_impl.h"

#include "shmem_logger.h"
#include "shmem_barriers_if.h"


int _glob_myid=-1;
int _glob_size=-1;

int dart_init_shmem(int *argc, char ***argv)
{
  int i;
  int itmp; size_t stmp;
  size_t syncarea_size;
  int myid = -1;
  int team_size = -1;
  int shm_id = -1;

  DEBUG("dart_init parsing args... %s", "");
  for (i = 0; i < (*argc); i++)   {
    if (sscanf((*argv)[i], "--dart-id=%d", &itmp) > 0) {
      myid = itmp;
      DEBUG("dart_init got %d for --dart-id", myid);
      _glob_myid=myid;
    }
    
    if (sscanf((*argv)[i], "--dart-size=%d", &itmp) > 0) {
      team_size = itmp;
      DEBUG("dart_init got %d for --dart-size", team_size);
      _glob_size=team_size;
    }  

    if (sscanf((*argv)[i], "--dart-syncarea_id=%d", &itmp) > 0) {
      shm_id = itmp;
      DEBUG("dart_init got %d for --dart-syncarea_id", shm_id);
    }
    
    if (sscanf((*argv)[i], "--dart-syncarea_size=%zu", &stmp) > 0) {
      syncarea_size = stmp;
      DEBUG("dart_init got %d for --dart-syncarea_size", 
	    syncarea_size);
    }
  }
  
  // DART args are passed at the end
  *argc -= NUM_DART_ARGS;
  (*argv)[*argc] = NULL;
  
  if (myid < 0 || team_size < 1)  {
    return DART_ERR_OTHER;
  }
  
  DEBUG("dart_init attaching shm %d...", shm_id);
  void* syncarea = shmem_mm_attach(shm_id);
  DEBUG("dart_init attached to %p", syncarea);

  DEBUG("dart_init initializing interal sync area...%s", "");
  shmem_syncarea_setaddr(syncarea);

  assert(DART_TEAM_ALL==0);
  dart_shmem_team_init(DART_TEAM_ALL,
		       myid, team_size);

  /* KF
  DEBUG("dart_init %s", "teams_init");
  dart_teams_init(myid, team_size);

  DEBUG("dart_init %s", "calling barrier");
  DART_SAFE(dart_barrier(DART_TEAM_ALL));
  */

  DEBUG("dart_init %s", "done");
  return DART_OK;
}


dart_ret_t dart_exit_shmem()
{
  size_t tsize;
  dart_unit_t myid;

  DEBUG("in dart_exit_shmem%s", "");
  dart_size(&tsize);
  dart_myid(&myid);
    
  assert(DART_TEAM_ALL==0);
  DART_SAFE(dart_barrier(DART_TEAM_ALL));

  DART_SAFE(
	    dart_shmem_team_delete(DART_TEAM_ALL,
				   myid, tsize)
	    );

  /* KF
  int size = dart_team_size(DART_TEAM_ALL);
  dart_teams_cleanup(_glob_myid, size);
  shmif_barriers_destroy();
  */

  return DART_OK;
}

