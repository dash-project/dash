
#include <assert.h>
#include <stdio.h>

#ifdef USE_HELPER_THREAD
#include <pthread.h>
#include <dash/dart/shmem/dart_helper_thread.h>
#endif 

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_globmem.h>
#include <dash/dart/if/dart_initialization.h>
#include <dash/dart/if/dart_team_group.h>
#include <dash/dart/if/dart_communication.h>

#include <dash/dart/shmem/dart_shmem.h>
#include <dash/dart/shmem/dart_teams_impl.h>
#include <dash/dart/shmem/shmem_mm_if.h>
#include <dash/dart/shmem/shmem_logger.h>
#include <dash/dart/shmem/shmem_barriers_if.h>

#ifdef USE_HELPER_THREAD
pthread_t _helper_thread;
#endif

int _glob_myid=-1;
int _glob_size=-1;
int _glob_state=DART_STATE_NOT_INITIALIZED;


int dart_init_shmem(int *argc, char ***argv)
{
  int i;
  int itmp; size_t stmp;
  size_t syncarea_size;
  dart_team_unit_t myid = {DART_UNDEFINED_UNIT_ID};
  int team_size = -1;
  int shm_id = -1;

  DEBUG("dart_init parsing args... %d", *argc);
  for (i = 0; i < (*argc); i++)   {
    if (sscanf((*argv)[i], "--dart-id=%d", &itmp) > 0) {
      myid.id = itmp;
      DEBUG("dart_init got %d for --dart-id", myid);
      _glob_myid=myid.id;
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

  if (myid.id < 0 || team_size < 1)  {
    fprintf(stderr, "ABORT: This program must be started with dartrun!\n");
    fprintf(stderr, "\n");
    exit(1);
    //    return DART_ERR_OTHER;
  }
  
  // DART args are passed at the end
  *argc -= NUM_DART_ARGS;
  (*argv)[*argc] = NULL;
  
  DEBUG("dart_init attaching shm %d...", shm_id);
  void* syncarea = shmem_mm_attach(shm_id);
  DEBUG("dart_init attached to %p", syncarea);

  DEBUG("dart_init initializing interal sync area...%s", "");
  shmem_syncarea_setaddr(syncarea);

  // we can pass a zero pointer as a group 
  // spec, because dart_shmem_team_init will 
  // take care of initializing the group for
  // dart_team_all!
  assert(DART_TEAM_ALL==0);
  dart_shmem_team_init(DART_TEAM_ALL,
		       myid, team_size,
		       0);

  DART_SAFE(dart_barrier(DART_TEAM_ALL));

#ifdef USE_HELPER_THREAD
  dart_work_queue_init();
  pthread_create(&_helper_thread, 0, dart_helper_thread, 0 );
#endif // USE_HELPER_THREAD

  DEBUG("dart_init %s", "done");
  return DART_OK;
}


dart_ret_t dart_exit_shmem()
{
  size_t tsize;
  dart_global_unit_t myid;

  DEBUG("in dart_exit_shmem%s", "");
  dart_size(&tsize);
  dart_myid(&myid);
    
  assert(DART_TEAM_ALL==0);
  DART_SAFE(dart_barrier(DART_TEAM_ALL));

  DART_SAFE(
	    dart_shmem_team_delete(DART_TEAM_ALL,
				   *((dart_team_unit_t*)(&myid)), tsize)
	    );

  shmem_syncarea_setunitstate(myid, 
			      UNIT_STATE_CLEAN_EXIT);

  dart_work_queue_shutdown();
  
#ifdef USE_HELPER_THREAD
  pthread_join(_helper_thread, 0);
#endif 


  /* KF
  int size = dart_team_size(DART_TEAM_ALL);
  dart_teams_cleanup(_glob_myid, size);
  shmif_barriers_destroy();
  */

  return DART_OK;
}

