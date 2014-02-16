
#include <stdio.h>
#include <dart.h>

int main(int argc, char* argv[])
{
  dart_unit_t myid;
  size_t size;

  char msg[128];

  dart_init(&argc, &argv);

  dart_myid(&myid);
  dart_size(&size);

  fprintf(stderr, "Hello World, I'm %d of %d\n",
	  myid, size);

  if( size!=2 ) {
    if( myid==0 )  {
      fprintf(stderr, "This has to be called with exactly 2 processes\n");
    }
    
    dart_exit();
    return 0;
  }

  if( myid==0 ) {
    sprintf(msg, "Hello from Unit #1!");
    dart_shmem_send(msg, strlen(msg)+1, DART_TEAM_ALL, 1);
  }
  else {
    dart_shmem_recv(msg, 128, DART_TEAM_ALL, 0);
    fprintf(stderr, "Received the following: '%s'\n", msg);
  }
  
  dart_exit();
}
