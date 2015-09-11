#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include <test.h>
#include <dart.h>


int main(int argc, char* argv[])
{
    char buf[80];
    pid_t pid;
    dart_unit_t myid;
    size_t size;

    CHECK(dart_init(&argc, &argv));
    CHECK(dart_myid(&myid));
    CHECK(dart_size(&size));

    gethostname(buf, 80);
    pid = getpid();
    fprintf(stderr,"Hello World, I'm unit %d of %d, pid=%d host=%s\n", myid, size, pid, buf);

    int max = 10;
    while(max-- > 0)
    {
        CHECK(dart_barrier(DART_TEAM_ALL));
    }
    CHECK(dart_exit());
    return 0;
}
