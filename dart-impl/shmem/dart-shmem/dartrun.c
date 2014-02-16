
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "dart_shmem.h"
#include "shmem_logger.h"
#include "shmem_barriers_if.h"
#include "shmem_mm_if.h"


int  dart_start(int argc, char* argv[]);
void dart_usage(char *s);

pid_t dart_spawn(int id, int nprocs, int shm_id, 
		 size_t syncarea_size,
		 char *exec, int argc, char **argv,
		 int nargs);


int main( int argc, char* argv[] )
{
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
  
  size_t syncarea_size = 4096*8; 
  
  int shm_id = shmem_mm_create(syncarea_size);
  void* shm_addr = shmem_mm_attach(shm_id);
  
  shmem_syncarea_init(nprocs, shm_addr, shm_id);
  
  for (int i = 0; i < nprocs; i++)
    {
      dart_spawn(i, nprocs, shm_id, syncarea_size, 
		 dashapp, argc, argv, nargs);
    }
  
  for (int i = 0; i < nprocs; i++)
    {
      int status;
      pid_t pid = waitpid(-1, &status, 0);
      DEBUG("child process %d terminated\n", pid);
    }
  
  shmem_syncarea_delete(nprocs, shm_addr, shm_id);

  shmem_mm_detach(shm_addr);
  shmem_mm_destroy(shm_id);
  return 0;
}

void dart_usage(char *s)
{
  fprintf(stderr, 
	  "Usage: %s [-n <n>] <executable> <args> \n"
	  "       runs n copies of executable\n", s);
}

pid_t dart_spawn(int id, int nprocs, int shm_id, 
		 size_t syncarea_size,
		 char *exec, int argc, char **argv,
		 int nargs)
{
  const int maxArgLen = 256;
  pid_t pid;
  int i = 0;
  int dartc;    // new argc for spawned process
  dartc = argc - nargs; // don't need initial executable and <n> Parameter
  dartc += NUM_DART_ARGS + 1; // Dart args and NULL-Pointer
  
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
  sprintf(dartv[i++], "--dart-syncarea_size=%d", syncarea_size);
  
  pid = fork();
  if (pid == 0)
    {
      int result = execv(exec, dartv);
      if (result == -1)
	{
	  char* s = strerror(errno);
	  fprintf(stderr, "execv failed: %s\n", s);
	}
      exit(result);
    }
  return pid;
}


