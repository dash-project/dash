#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include "../test.h"
#include "../utils.h"
#include <dart.h>

//test for: Init scope
// - dart_call before init,
// - dart_call after exit
// - mutliple init/exit
// !!Returntype not specified but should be not DART_OK!

int main(int argc, char* argv[])
{
  char buf[80];
  dart_unit_t myid;
  size_t size;
  pid_t pid;

  //calling function before init
  EXPECT_NE(dart_size(&size),DART_OK);

  CHECK(dart_init(&argc, &argv));

  CHECK(dart_myid(&myid));
  CHECK(dart_size(&size));

  gethostname(buf, 80);
  pid = getpid();

  fprintf(stderr,
          "Hello World, I'm unit %d of %d, pid=%d host=%s\n",
          myid, size, pid, buf);

  CHECK(dart_exit());

  //calling function after dart_exit()
  EXPECT_NE(dart_size(&size),DART_OK);

  EXPECT_NE(dart_init(&argc, &argv),DART_OK);
  EXPECT_NE(dart_exit(),DART_OK);
}
