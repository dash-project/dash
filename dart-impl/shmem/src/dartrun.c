
#define _POSIX_SOURCE


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <dirent.h>
#include <signal.h>

#include <dash/dart/shmem/dart_shmem.h>
#include <dash/dart/shmem/shmem_logger.h>
#include <dash/dart/shmem/shmem_barriers_if.h>
#include <dash/dart/shmem/shmem_mm_if.h>

typedef struct
{
  int aborted;
  pid_t pid;
} spawn_t;

spawn_t spawntable[MAXNUM_UNITS];

int  dart_start(int argc, char* argv[]);
dart_ret_t dart_usage(char *s);

pid_t dart_spawn(int id, int nprocs, int shm_id, 
		 size_t syncarea_size,
		 char *exec, int argc, char **argv,
		 int nargs);

void dartrun_cleanup(int shmem_id);


int main( int argc, char* argv[] )
{
  int i;
  for(i=0; i<MAXNUM_UNITS; i++ ) {
    spawntable[i].pid=0;
    spawntable[i].aborted=0;
  }

  int ret = dart_start(argc, argv);
  if(ret!=0) dart_usage(argv[0]);
}

int dart_start(int argc, char* argv[])
{
  DEBUG("dart_start %s", "called");
  
  int nargs=0;
  int nprocs;      // number of processes to start
  char *dashapp;   // path to app executable
  
  if( argc>=2 && !strncmp("-n", argv[1], 2) ) {
    nprocs = argc>=3?atoi(argv[2]):0;
    if (nprocs <= 0) {
      fprintf(stderr, "Error: Enter a positive integer %s\n", 
	      argv[2]?argv[2]:"");
      return 1;
    }
    nargs=2;
  } else {
    nprocs=1;
    nargs=0;
  }

  dashapp = argc>=nargs+2?argv[nargs+1]:0;
  if( !dashapp || access(dashapp, X_OK)) {
    if( dashapp ) 
      fprintf(stderr, "Error: Can't open '%s'\n", 
	      dashapp);
    return 1;
  }
  
  size_t syncarea_size = 4096*64; 
  
  int shm_id = shmem_mm_create(syncarea_size);
  void* shm_addr = shmem_mm_attach(shm_id);
  int i;
  dart_global_unit_t j;

  shmem_syncarea_init(nprocs, shm_addr, shm_id);
  
  for (i = 0; i < nprocs; i++) {
    pid_t spid;
    spid = dart_spawn(
             i,
             nprocs,
             shm_id,
             syncarea_size,
             dashapp,
             argc,
             argv,
             nargs + 1);
    spawntable[i].pid = spid;
  }
  int abort = 0;
  for (i = 0; i < nprocs; i++) {
    int status;
    pid_t pid = waitpid(-1, &status, 0);
    DEBUG("child process %d terminated\n", pid);
    
    int found = 0;
    for (j.id = 0; j.id < MAXNUM_UNITS; j.id++) {
      if (spawntable[j.id].pid == pid) {
        found = 1;
        break;
      }
    }
    if (found &&
        shmem_syncarea_getunitstate(j) != 
          UNIT_STATE_CLEAN_EXIT ) {
      ERROR("Unit %d terminated, aborting!", j);
      spawntable[j.id].aborted=1;
      abort=1;
      break;
    }
  }
  if (abort) {
    for (i = 0; i < nprocs; i++) {
      if (spawntable[i].pid != 0 && 
          !spawntable[i].aborted) {
        kill(spawntable[i].pid, SIGTERM);
      }
    }
  }
  
  shmem_syncarea_delete(nprocs, shm_addr, shm_id);

  shmem_mm_detach(shm_addr);
  shmem_mm_destroy(shm_id);

  dartrun_cleanup(shm_id);
  return 0;
}

dart_ret_t dart_usage(char *s)
{
  fprintf(stderr, 
	  "Usage: %s [-n <n>] <executable> <args> \n"
	  "       runs n copies of executable\n", s);
  return DART_OK;
}

pid_t dart_spawn(
  int id,
  int nprocs,
  int shm_id, 
  size_t syncarea_size,
  char *exec,
  int argc,
  char **argv,
  int nargs)
{
  const int maxArgLen = 256;
  pid_t pid;
  int i = 0;
  // new argc for spawned process
  int dartc;
  // don't need initial executable and <n> Parameter
  dartc = argc - nargs;
  // Dart args and NULL-Pointer
  dartc += NUM_DART_ARGS + 1;
  
  char* dartv[dartc];
  for (i = nargs; i < argc; i++)
    dartv[i - nargs] = strdup(argv[i]);
  for (i = argc - nargs; i < dartc - 1; i++)
    dartv[i] = (char*) malloc(maxArgLen + 1);
  dartv[dartc - 1] = NULL;
  
  i = argc - nargs;
  sprintf(dartv[i++], "--dart-id=%d", id);
  sprintf(dartv[i++], "--dart-size=%d", nprocs);
  sprintf(dartv[i++], "--dart-syncarea_id=%d", shm_id);
  sprintf(dartv[i++], "--dart-syncarea_size=%zu", syncarea_size);
  
  pid = fork();
  if (pid == 0) {
    int result = execv(exec, dartv);
    if (result == -1) {
      char* s = strerror(errno);
      fprintf(stderr, "execv failed: %s\n", s);
    }
    exit(result);
  }
  return pid;
}


#define DART_SYSV_TMP_DIR "/tmp/"
#define DART_SYSV_TMP_PAT "sysv-%d"

// clean up any resources that have not been 
// freed by the launched processes
void dartrun_cleanup(int shmem_id)
{
  int ret;
  DIR *dirp;
  struct dirent *dp;
  char pat[80]; char *s;
  char fname[256];

  sprintf(pat, DART_SYSV_TMP_PAT, shmem_id);
  dirp = opendir(DART_SYSV_TMP_DIR);

  // try to find leftover files and delete them
  while ((dp = readdir(dirp)) != NULL) {
    s = strstr(dp->d_name, pat);
    if (s > 0) {
      sprintf(
        fname,
        "%s/%s",
        DART_SYSV_TMP_DIR,
	      dp->d_name);
      ret = unlink(fname);
      if (ret != 0) {
        ERROR("Couldn't delete file %s", fname);
      }
    }
  }
  closedir(dirp);
}
