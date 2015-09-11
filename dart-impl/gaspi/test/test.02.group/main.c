#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>

#include <test.h>
#include <dart.h>


int main(int argc, char* argv[])
{
    dart_unit_t myid;
    size_t size;

    CHECK(dart_init(&argc, &argv));
    CHECK(dart_myid(&myid));
    CHECK(dart_size(&size));

    size_t gsize;
    CHECK(dart_group_sizeof(&gsize));

    dart_group_t *even_group = (dart_group_t *) malloc(gsize);
    assert(even_group);
    dart_group_t *odd_group  = (dart_group_t *) malloc(gsize);
    assert(odd_group);

    CHECK(dart_group_init(even_group));
    CHECK(dart_group_init(odd_group));

    for(int i = 0 ; i < size ;++i)
    {
        if(i % 2 == 0)
        {
            CHECK(dart_group_addmember(even_group, i));
        }
        else
        {
            CHECK(dart_group_addmember(odd_group, i));
        }
    }

    size_t odd_size, even_size;

    CHECK(dart_group_size(even_group, &even_size));
    CHECK(dart_group_size(odd_group, &odd_size));

    dart_unit_t * even_ids = (dart_unit_t *) malloc(sizeof(dart_unit_t) * even_size);
    assert(even_ids);

    dart_unit_t * odd_ids =  (dart_unit_t *) malloc(sizeof(dart_unit_t) * odd_size);
    assert(odd_ids);

    CHECK(dart_group_getmembers(even_group, even_ids));
    CHECK(dart_group_getmembers(odd_group, odd_ids));

    for(int i = 0; i < even_size; ++i)
    {
        if(even_ids[i] % 2 != 0)
        {
            fprintf(stderr, "even group has wrong member ids %d \n", even_ids[i]);
        }
    }

    for(int i = 0; i < odd_size; ++i)
    {
        if(odd_ids[i] % 2 == 0)
        {
            fprintf(stderr, "odd group has wrong member\n");
        }
    }

    dart_group_t *all_group = (dart_group_t *) malloc(gsize);
    assert(all_group);

    CHECK(dart_group_init(all_group));

    CHECK(dart_group_union(even_group, odd_group, all_group));

    size_t all_size;
    CHECK(dart_group_size(all_group, &all_size));

    if(all_size != size)
    {
        fprintf(stderr,"group union failed\n");
    }

    dart_unit_t * all_ids =  (dart_unit_t *) malloc(sizeof(dart_unit_t) * all_size);
    assert(all_ids);

    CHECK(dart_group_getmembers(all_group, all_ids));

    for(int i = 0; i < size; ++i)
    {
        if(i != all_ids[i])
        {
            fprintf(stderr, "wrong member in union group\n");
        }
    }

    free(even_ids);
    free(odd_ids);
    free(all_ids);

    CHECK(dart_group_fini(even_group));
    CHECK(dart_group_fini(odd_group));
    CHECK(dart_group_fini(all_group));

    free(even_group);
    free(odd_group);
    free(all_group);

    CHECK(dart_exit());
    return 0;
}
