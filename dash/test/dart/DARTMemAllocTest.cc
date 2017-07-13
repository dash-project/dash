
#include "DARTMemAllocTest.h"
#include <dash/dart/if/dart_communication.h>
#include <dash/dart/if/dart_globmem.h>
#include <dash/Array.h>

TEST_F(DARTMemAllocTest, SmallLocalAlloc)
{

  typedef int value_t;
  dart_gptr_t gptr1;
  ASSERT_EQ_U(
    DART_OK,
    dart_memalloc(sizeof(value_t), DART_TYPE_LONG, &gptr1));
  ASSERT_NE_U(
    DART_GPTR_NULL,
    gptr1);
  value_t *baseptr;
  ASSERT_EQ_U(
    DART_OK,
    dart_gptr_getaddr(gptr1, (void**)&baseptr));


  // check that different allocation receives a different pointer
  dart_gptr_t gptr2;
  ASSERT_EQ_U(
    DART_OK,
    dart_memalloc(sizeof(value_t), DART_TYPE_LONG, &gptr2));

  ASSERT_NE(gptr1, gptr2);

  ASSERT_EQ_U(
    DART_OK,
    dart_memfree(gptr2));
  ASSERT_EQ_U(
    DART_OK,
    dart_memfree(gptr1));
}

TEST_F(DARTMemAllocTest, LocalAlloc)
{
  typedef int value_t;
  const size_t block_size = 10;

  dart_gptr_t gptr;
  ASSERT_EQ_U(
    DART_OK,
    dart_memalloc(block_size * sizeof(value_t), DART_TYPE_LONG, &gptr));
  ASSERT_NE_U(
    DART_GPTR_NULL,
    gptr);
  value_t *baseptr;
  ASSERT_EQ_U(
    DART_OK,
    dart_gptr_getaddr(gptr, (void**)&baseptr));


  // check that different allocation receives a different pointer
  dart_gptr_t gptr2;
  ASSERT_EQ_U(
    DART_OK,
    dart_memalloc(block_size * sizeof(value_t), DART_TYPE_LONG, &gptr2));
  ASSERT_NE(gptr, gptr2);

  ASSERT_EQ_U(
    DART_OK,
    dart_memfree(gptr2));

  for (size_t i = 0; i < block_size; ++i) {
    baseptr[i] = dash::myid().id;
  }

  dash::Array<dart_gptr_t> arr(dash::size());
  arr.local[0] = gptr;
  arr.barrier();

  // read from the neighbor
  value_t neighbor_val;
  size_t  rneighbor_id = (dash::myid().id + 1) % dash::size();
  size_t  lneighbor_id = (dash::myid().id + dash::size() - 1) % dash::size();
  dart_storage_t ds = dash::dart_storage<value_t>(1);
  ASSERT_EQ_U(
    DART_OK,
    dart_get_blocking(
        &neighbor_val,
        arr[rneighbor_id],
        ds.nelem,
        ds.dtype,
        DART_FLAG_NONE));

  ASSERT_EQ_U(
    rneighbor_id,
    neighbor_val);

  arr.barrier();

  // write to the neighbor val


  int val = dash::myid().id;
  ASSERT_EQ_U(
    DART_OK,
    dart_put_blocking(
        arr[rneighbor_id],
        &val,
        ds.nelem,
        ds.dtype));

  arr.barrier();

  // read local value
  ASSERT_EQ_U(
    DART_OK,
    dart_get_blocking(&val, gptr, ds.nelem, ds.dtype, DART_FLAG_NONE));

  ASSERT_EQ_U(
    lneighbor_id,
    val);

  ASSERT_EQ_U(
    DART_OK,
    dart_memfree(gptr));
}


TEST_F(DARTMemAllocTest, SegmentReuseTest)
{
  const size_t block_size = 10;
  dart_gptr_t gptr;
  ASSERT_EQ_U(
    DART_OK,
    dart_team_memalloc_aligned(DART_TEAM_ALL, block_size, DART_TYPE_INT, &gptr)
  );
  int16_t segid = gptr.segid;

  // check that all allocations have the same segment ID
  dash::Array<dart_gptr_t> arr(dash::size());
  arr.local[0] = gptr;
  arr.barrier();
  if (dash::myid() == 0) {
    for (int i = 0; i < dash::size(); ++i) {
      EXPECT_EQ_U(
          gptr.segid,
          static_cast<dart_gptr_t>(arr[i]).segid);
    }
  }

  // check that consecutive allocations do not share segment IDs
  dart_gptr_t gptr2;
  ASSERT_EQ_U(
    DART_OK,
    dart_team_memalloc_aligned(
        DART_TEAM_ALL, block_size, DART_TYPE_INT, &gptr2)
  );

  ASSERT_NE_U(gptr2.segid, gptr.segid);
  arr.local[0] = gptr2;
  arr.barrier();
  if (dash::myid() == 0) {
    for (int i = 0; i < dash::size(); ++i) {
      ASSERT_EQ_U(
          gptr2.segid,
          static_cast<dart_gptr_t>(arr[i]).segid);
    }
  }

  // check that a released segment ID is re-used
  ASSERT_EQ_U(
    DART_OK,
    dart_team_memfree(gptr));

  ASSERT_EQ_U(
    DART_OK,
    dart_team_memalloc_aligned(DART_TEAM_ALL, block_size, DART_TYPE_INT, &gptr)
  );

  ASSERT_EQ_U(gptr.segid, segid);

  // tear-down
  ASSERT_EQ_U(
    DART_OK,
    dart_team_memfree(gptr));

  ASSERT_EQ_U(
    DART_OK,
    dart_team_memfree(gptr2));
}
