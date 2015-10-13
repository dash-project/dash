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
    dart_gptr_t g1;
    size_t size;
    void * ptr;

    CHECK(dart_init(&argc, &argv));
    CHECK(dart_myid(&myid));
    CHECK(dart_size(&size));
    CHECK(dart_memalloc(1024, &g1));
    CHECK(dart_gptr_getaddr(g1, &ptr));
    *((int *) ptr) = 42;

    gaspi_pointer_t gaspi_ptr;
    gaspi_segment_ptr(1, &gaspi_ptr);

    int * gaspi_int_ptr = (int *) ((char *) gaspi_ptr + g1.addr_or_offs.offset);
    if(*((int *) ptr) != *gaspi_int_ptr)
    {
        fprintf(stderr,"Error: Wrong value in segment or offset invalid\n");
    }

    CHECK(dart_memfree(g1));
    CHECK(dart_exit());
    return 0;
}
