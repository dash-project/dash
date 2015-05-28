#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include "../test.h"
#include "../utils.h"
#include <dart.h>

// #define NASTY

//test for: Init
// - dart_init not using &arvc, &argv,
// -> behaviour not defined!
// Returntype not specified but should be not DART_OK!
// define NASTY results in an infinity loop

int main(int argc, char* argv[])
{
  char buf[80];
  dart_unit_t myid;
  size_t size;
  pid_t pid;

  for(int i = 0; i<sizeof(char*) *1;i++){
    printf("%s\n",argv[i]);
  }
#ifdef NASTY
  argv[2]="--dart-id=1";
#else
  argv[2]="x";
#endif
  for(int i = 0; i<sizeof(char*) *1;i++){
    printf("%s\n",argv[i]);
  }
  //init with wrong parameters should fail!  
  CHECK(dart_init(&argc, &argv));

  /*CHECK(dart_myid(&myid));
  CHECK(dart_size(&size));

  gethostname(buf, 80);
  pid = getpid();

  fprintf(stderr, 
          "Hello World, I'm unit %d of %d, pid=%d host=%s\n",
          myid, size, pid, buf);
*/
//  CHECK(dart_exit());

}

