
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
    dart_memalloc(3, DART_TYPE_INT, &gptr1));
  ASSERT_NE_U(
    DART_GPTR_NULL,
    gptr1);

  dart_gptr_t gptr2;
  ASSERT_EQ_U(
    DART_OK,
    dart_memalloc(1, DART_TYPE_INT, &gptr2));
  ASSERT_NE_U(
    DART_GPTR_NULL,
    gptr2);


  // check that different allocation receives a different pointer
  ASSERT_NE(gptr1, gptr2);

  // test for overlap of small allocations
  value_t *baseptr1;
  ASSERT_EQ_U(
    DART_OK,
    dart_gptr_getaddr(gptr1, (void**)&baseptr1));
  value_t *baseptr2;
  ASSERT_EQ_U(
    DART_OK,
    dart_gptr_getaddr(gptr2, (void**)&baseptr2));
  ASSERT_LE(baseptr1+4, baseptr2);

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
    dart_memalloc(block_size, DART_TYPE_INT, &gptr));
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
    dart_memalloc(block_size, DART_TYPE_INT, &gptr2));
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

  value_t neighbor_val;
  size_t  neighbor_id = (dash::myid().id + 1) % dash::size();
  dash::dart_storage<value_t> ds(1);
  ASSERT_EQ_U(
    DART_OK,
    dart_get_blocking(
        &neighbor_val,
        arr[neighbor_id],
        ds.nelem,
        ds.dtype,
        ds.dtype));

  ASSERT_EQ_U(
    neighbor_id,
    neighbor_val);

  arr.barrier();

  ASSERT_EQ_U(
    DART_OK,
    dart_memfree(gptr));
}


TEST_F(DARTMemAllocTest, SegmentReuseTest)
{
  const size_t block_size = 10*dash::size();
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


TEST_F(DARTMemAllocTest, AllocatorSimpleTest)
{
  dart_allocator_t allocator;
  dart_gptr_t gptr;
  constexpr size_t allocator_size = 1024;

  // collectively create a new allocator
  ASSERT_EQ_U(
    DART_OK,
    dart_allocator_new(allocator_size, DART_TEAM_ALL, &allocator));
  //ASSERT_NE_U(allocator, NULL);

  // allocate some local memory on a single unit and distribute the result
  if (dash::myid() == 0) {
    ASSERT_EQ_U(
      DART_OK,
      dart_allocator_alloc(dash::size(), DART_TYPE_INT, &gptr, allocator));
    ASSERT_NE_U(DART_GPTR_NULL, gptr);
  }
  dash::dart_storage<dart_gptr_t> ds_gptr(1);
  dart_bcast(&gptr, ds_gptr.nelem, ds_gptr.dtype, 0, DART_TEAM_ALL);
  int myid = dash::myid();

  // write to my position on unit 0
  gptr.addr_or_offs.offset += dash::myid() * sizeof(int);
  ASSERT_EQ_U(
    DART_OK,
    dart_put_blocking(gptr, &myid, 1, DART_TYPE_INT, DART_TYPE_INT));
  dash::barrier();

  // check the result
  if (dash::myid() == 0) {
    dart_gptr_t gptr_it = gptr;
    for (int i = 0; i < dash::size(); ++i) {
      int val;
      ASSERT_EQ_U(
        DART_OK,
        dart_get_blocking(&val, gptr_it, 1, DART_TYPE_INT, DART_TYPE_INT));
      ASSERT_EQ_U(i, val);
      gptr_it.addr_or_offs.offset += sizeof(int);
    }
    // free the allocated memory
    ASSERT_EQ_U(
      DART_OK,
      dart_allocator_free(&gptr, allocator));
  }

  // collectively destroy the allocator
  ASSERT_EQ_U(
    DART_OK,
    dart_allocator_destroy(&allocator));
}


TEST_F(DARTMemAllocTest, AllocatorFullAllocationTest)
{
  dart_allocator_t allocator;
  dart_gptr_t gptr;
  constexpr size_t allocator_size = 1024;

  // collectively create a new allocator
  ASSERT_EQ_U(
    DART_OK,
    dart_allocator_new(allocator_size*sizeof(int), DART_TEAM_ALL, &allocator));
  //ASSERT_NE_U(allocator, NULL);

  // allocate some local memory on a single unit and distribute the result
  if (dash::myid() == 0) {
    ASSERT_EQ_U(
      DART_OK,
      dart_allocator_alloc(allocator_size, DART_TYPE_INT, &gptr, allocator));
    ASSERT_NE_U(DART_GPTR_NULL, gptr);
    ASSERT_EQ_U(
      DART_OK,
      dart_allocator_free(&gptr, allocator));
  }

  // collectively destroy the allocator
  ASSERT_EQ_U(
    DART_OK,
    dart_allocator_destroy(&allocator));
}

TEST_F(DARTMemAllocTest, AllocatorFullChunksTest)
{
  dart_allocator_t allocator;
  constexpr size_t allocator_size  = 1024;
  constexpr size_t allocation_size = 8;

  // collectively create a new allocator
  ASSERT_EQ_U(
    DART_OK,
    dart_allocator_new(allocator_size*sizeof(int), DART_TEAM_ALL, &allocator));
  //ASSERT_NE_U(allocator, NULL);

  // allocate some local memory on a single unit and distribute the result
  if (dash::myid() == 0) {
    size_t num_allocs = allocator_size / allocation_size;
    std::vector<dart_gptr_t> gptrs;
    gptrs.reserve(num_allocs);
    // exhaust all memory through chunks
    for (size_t i = 0; i < num_allocs; ++i) {
      dart_gptr_t gptr;
      std::cout << "Allocation " << i << std::endl;
      ASSERT_EQ_U(
        DART_OK,
        dart_allocator_alloc(allocation_size, DART_TYPE_INT, &gptr, allocator));
      ASSERT_NE_U(DART_GPTR_NULL, gptr);
      gptrs.push_back(gptr);
    }
    // try allocate one more chunk
    dart_gptr_t tmp_gptr;
    ASSERT_EQ_U(
      DART_ERR_NOMEM,
      dart_allocator_alloc(allocation_size, DART_TYPE_INT, &tmp_gptr, allocator));
    ASSERT_EQ_U(DART_GPTR_NULL, tmp_gptr);

    // free all allocated chunks
    for (auto &gptr : gptrs) {
      ASSERT_EQ_U(
        DART_OK,
        dart_allocator_free(&gptr, allocator));
    }
  }

  // collectively destroy the allocator
  ASSERT_EQ_U(
    DART_OK,
    dart_allocator_destroy(&allocator));
}
