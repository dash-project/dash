#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <test.h>
#include <dart.h>
#include <assert.h>

#include <GASPI.h>

int main(int argc, char* argv[])
{
    dart_unit_t myid;
    dart_unit_t root = 1;
    size_t size;
    const size_t send_count = 10;
    int * send_buffer;

    CHECK(dart_init(&argc, &argv));
    CHECK(dart_myid(&myid));
    CHECK(dart_size(&size));

    send_buffer = (int *) malloc(sizeof(int) * send_count);
    assert(send_buffer);

    if(myid == root)
    {
        for(int i = 0 ; i < send_count ; ++i)
        {
            send_buffer[i] = myid + 1;
        }
    }

    CHECK(dart_bcast(send_buffer, send_count * sizeof(int), root, DART_TEAM_ALL));

    for(int i = 0 ; i < send_count ; ++i)
    {
        gaspi_printf("Send buf %d\n", send_buffer[i]);
    }

    free(send_buffer);
    CHECK(dart_exit());

    return 0;
}
