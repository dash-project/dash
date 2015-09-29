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
    size_t size;
    const size_t send_count = 10;
    int * send_buffer;
    int * recv_buffer;

    CHECK(dart_init(&argc, &argv));
    CHECK(dart_myid(&myid));
    CHECK(dart_size(&size));

    send_buffer = (int *) malloc(sizeof(int) * send_count);
    assert(send_buffer);

    recv_buffer = (int *) malloc(sizeof(int) * send_count * size);
    assert(recv_buffer);

    for(int i = 0 ; i < send_count ; ++i)
    {
        send_buffer[i] = myid + 1;
    }

    CHECK(dart_allgather(send_buffer, recv_buffer, send_count * sizeof(int), DART_TEAM_ALL));

    for(int i = 0 ; i < size ; ++i)
    {
        for(int j = 0 ; j < send_count ; ++j)
        {
            if(recv_buffer[(i * send_count) + j] != (i + 1))
            {
                fprintf(stderr, "Wrong value received on rank %d\n", myid);
            }
        }
    }

    free(send_buffer);
    free(recv_buffer);
    CHECK(dart_exit());

    return 0;
}
