#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>

#include <dash/dart/if/dart.h>
#include <assert.h>
#include <GASPI.h>

#include "gtest/gtest.h"

#define CHECK(func) EXPECT_EQ(DART_OK, func)

const int transfer_val_count = 1000;
const int transfer_val_begin = 42;

TEST(Get_Blocking, different_segment)
{

    dart_unit_t myid;
    size_t size;
    dart_gptr_t gptr_team;
    dart_gptr_t gptr_priv;

    CHECK(dart_myid(&myid));
    CHECK(dart_size(&size));

    CHECK(dart_barrier(DART_TEAM_ALL));

    CHECK(dart_memalloc(transfer_val_count * sizeof(int), &gptr_priv));
    CHECK(dart_team_memalloc_aligned(DART_TEAM_ALL, transfer_val_count * sizeof(int), &gptr_team));

    if(myid == 1)
    {
        void * g_ptr = NULL;

        CHECK(dart_gptr_setunit(&gptr_team, myid));
        CHECK(dart_gptr_getaddr(gptr_team, &g_ptr));

        for(size_t i = 0 ; i < transfer_val_count ; ++i)
        {
            ((int *) g_ptr)[i] = transfer_val_begin + i;
        }
    }

    CHECK(dart_barrier(DART_TEAM_ALL));

    if(myid == 0)
    {
        dart_gptr_t g_dest = gptr_team;
        CHECK(dart_gptr_setunit(&g_dest, myid + 1));

        CHECK(dart_get_gptr_blocking(g_dest, gptr_priv, transfer_val_count * sizeof(int)));

        void * g_ptr = NULL;
        CHECK(dart_gptr_getaddr(gptr_priv, &g_ptr));
        for(size_t i = 0 ; i < transfer_val_count ; ++i)
        {
            EXPECT_EQ(((int *) g_ptr)[i], (int) transfer_val_begin + i);
        }
    }

    CHECK(dart_memfree(gptr_priv));
    CHECK(dart_barrier(DART_TEAM_ALL));
    CHECK(dart_team_memfree(DART_TEAM_ALL, gptr_team));
}

TEST(Get_Blocking, same_segment)
{
    dart_unit_t myid;
    size_t size;
    dart_gptr_t g;

    CHECK(dart_myid(&myid));
    CHECK(dart_size(&size));

    CHECK(dart_barrier(DART_TEAM_ALL));

    CHECK(dart_team_memalloc_aligned(DART_TEAM_ALL, transfer_val_count * sizeof(int), &g));
    if(myid == 1)
    {
        void * g_ptr = NULL;

        CHECK(dart_gptr_setunit(&g, myid));
        CHECK(dart_gptr_getaddr(g, &g_ptr));

        for(size_t i = 0 ; i < transfer_val_count ; ++i)
        {
            ((int *) g_ptr)[i] = transfer_val_begin + i;
        }
    }
    CHECK(dart_barrier(DART_TEAM_ALL));
    if(myid == 0)
    {
        dart_gptr_t g_src = g, g_dest = g;
        CHECK(dart_gptr_setunit(&g_dest, myid + 1));
        CHECK(dart_gptr_setunit(&g_src, myid));

        CHECK(dart_get_gptr_blocking(g_dest, g_src, transfer_val_count * sizeof(int)));

        void * g_ptr = NULL;
        CHECK(dart_gptr_getaddr(g_src, &g_ptr));
        for(size_t i = 0 ; i < transfer_val_count ; ++i)
        {
            EXPECT_EQ(((int *) g_ptr)[i], (int) transfer_val_begin + i);
        }
    }
    CHECK(dart_barrier(DART_TEAM_ALL));
    CHECK(dart_team_memfree(DART_TEAM_ALL, g));
}


int main(int argc, char* argv[])
{
    ::testing::InitGoogleTest(&argc, argv);
    dart_init(&argc, &argv);
    int ret = RUN_ALL_TESTS();
    dart_exit();

    return ret;
}
