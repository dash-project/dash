#include <libdash.h>
#include <gtest/gtest.h>
#include "TestBase.h"
#include "DARTOnesidedTest.h"

TEST_F(DARTOnesidedTest, GetBlockingSingleBlock)
{
  typedef int value_t;
  const size_t block_size = 10;
  size_t num_elem_total   = _dash_size * block_size;
  dash::Array<value_t> array(num_elem_total, dash::BLOCKED);
  // Array to store local copy:
  int local_array[block_size];
  // Assign initial values: [ 1000, 1001, 1002, ... 2000, 2001, ... ]
  for (auto l = 0; l < block_size; ++l) {
    array.local[l] = ((dash::myid() + 1) * 1000) + l;
  }
  array.barrier();
  // Unit to copy values from:
  dart_unit_t unit_src  = (dash::myid() + 1) % _dash_size;
  // Global start index of block to copy:
  int g_src_index       = unit_src * block_size;
  LOG_MESSAGE("Copy from unit %d -> offset %d",
              unit_src, g_src_index);
  // Copy values:
  dart_get_blocking(
    local_array,                                // lptr dest
    (array.begin() + g_src_index).dart_gptr(),  // gptr start
    block_size * sizeof(value_t)                // nbytes
  );
  for (auto l = 0; l < block_size; ++l) {
    LOG_MESSAGE("Testing local element %d = %d", l, local_array[l]);
    value_t expected = array[g_src_index + l];
    ASSERT_EQ_U(expected, local_array[l]);
  }
}

TEST_F(DARTOnesidedTest, GetBlockingTwoBlocks)
{
  typedef int value_t;
  const size_t block_size    = 10;
  const size_t num_elem_copy = 2 * block_size;
  size_t num_elem_total      = _dash_size * block_size;
  dash::Array<value_t> array(num_elem_total, dash::BLOCKED);
  if (_dash_size < 2) {
    return;
  }
  // Array to store local copy:
  int local_array[num_elem_copy];
  // Assign initial values: [ 1000, 1001, 1002, ... 2000, 2001, ... ]
  for (auto l = 0; l < block_size; ++l) {
    array.local[l] = ((dash::myid() + 1) * 1000) + l;
  }
  array.barrier();
  // Copy values from first two blocks:
  dart_get_blocking(
    local_array,                      // lptr dest
    array.begin().dart_gptr(),        // gptr start
    num_elem_copy * sizeof(value_t)   // nbytes
  );
  // Fails for elements in second block, i.e. for l < num_elem_copy:
  for (auto l = 0; l < block_size; ++l) {
    LOG_MESSAGE("Testing local element %d = %d", l, local_array[l]);
    value_t expected = array[l];
    ASSERT_EQ_U(expected, local_array[l]);
  }
}

TEST_F(DARTOnesidedTest, GetHandleAllRemote)
{
  typedef int value_t;
  const size_t block_size = 5000;
  size_t num_elem_copy    = (_dash_size - 1) * block_size;
  size_t num_elem_total   = _dash_size * block_size;
  dash::Array<value_t> array(num_elem_total, dash::BLOCKED);
  if (_dash_size < 2) {
    return;
  }
  // Array to store local copy:
  int local_array[num_elem_copy];
  // Array of handles, one for each dart_get_handle:
  std::vector<dart_handle_t> handles;
  // Assign initial values: [ 1000, 1001, 1002, ... 2000, 2001, ... ]
  for (auto l = 0; l < block_size; ++l) {
    array.local[l] = ((dash::myid() + 1) * 1000) + l;
  }
  array.barrier();

  LOG_MESSAGE("Requesting remote blocks");
  // Copy values from all non-local blocks:
  int block = 0;
  for (auto u = 0; u < _dash_size; ++u) {
    if (u != dash::myid()) {
      LOG_MESSAGE("Requesting block %d from unit %d", block, u);
      dart_handle_t handle;
      handles.push_back(handle);
      dart_get_handle(
        local_array + (block * block_size),
        (array.begin() + (u * block_size)).dart_gptr(),
        block_size * sizeof(value_t),
        &handles[block]
      );
      ++block;
    }
  }
  // Wait for completion of get operations:
  LOG_MESSAGE("Waiting for completion of async requests");
  dart_waitall_local(
    &handles[0],
    handles.size()
  );

  LOG_MESSAGE("Validating values");
  int l = 0;
  for (auto g = 0; g < array.size(); ++g) {
    auto unit = array.pattern().unit_at(g);
    if (unit != dash::myid()) {
      LOG_MESSAGE("Testing local element %d (g:%d u:%d) = %d",
                  l, g, unit, local_array[l]);
      value_t expected = array[g];
      ASSERT_EQ_U(expected, local_array[l]);
      ++l;
    }
  }
  ASSERT_EQ_U(num_elem_copy, l);
}
