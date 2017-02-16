
#include "DARTMemAllocTest.h"
#include <dash/dart/if/dart_communication.h>
#include <dash/dart/if/dart_globmem.h>
#include <dash/Array.h>


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

  for (size_t i = 0; i < block_size; ++i) {
    baseptr[i] = _dash_id;
  }

  dash::Array<dart_gptr_t> arr(_dash_size);
  arr.local[0] = gptr;
  arr.barrier();

  value_t neighbor_val;
  size_t  neighbor_id = (_dash_id + 1) % _dash_size;
  dart_storage_t ds = dash::dart_storage<value_t>(1);
  ASSERT_EQ_U(
    DART_OK,
    dart_get_blocking(
        &neighbor_val,
        arr[neighbor_id],
        ds.nelem,
        ds.dtype));

  ASSERT_EQ_U(
    neighbor_id,
    neighbor_val);

  arr.barrier();

  ASSERT_EQ_U(
    DART_OK,
    dart_memfree(gptr));
}
