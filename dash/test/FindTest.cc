#include <libdash.h>
#include <gtest/gtest.h>

#include <limits>

#include "TestBase.h"
#include "FindTest.h"


TEST_F(FindTest, TestSimpleFind)
{
  _num_elem           = dash::Team::All().size();
  Element_t init_fill = 0;
  Element_t find_me	  = 24;

  // Initialize global array and fill it with init_fill:
  Array_t array(_num_elem);
  if (dash::myid() == 0) {
    for (size_t i = 0; i < array.size(); ++i) {
      LOG_MESSAGE("Setting array[%d] with init_fill", i, init_fill);
      array[i] = init_fill;
    }

    // Set element to be found in the the center position:
    index_t find_pos = array.size() / 2;
    LOG_MESSAGE("Setting array[%d] = %d (min)",
                find_pos, find_me);
    array[find_pos] = find_me;
  }
  // Wait for array initialization
  LOG_MESSAGE("Waiting for other units to initialize array values");
  array.barrier();
  LOG_MESSAGE("Finished initialization of array values");

  // Run find on complete array
  auto found_gptr = dash::find(array.begin(), array.end(), find_me);

  // Check that the element find_me has been found (found != last):
  LOG_MESSAGE("Completed dash::find");
  // Run find on complete array
  EXPECT_NE_U(found_gptr, array.end());
  // Check value found
  Element_t found_v = *found_gptr;
  LOG_MESSAGE("Expected find value: %d, found find value %d",
              find_me, found_v);
  EXPECT_EQ(find_me, found_v);

  array.barrier();
}

#if 0
TEST_F(FindTest, SimpleVaryingTest)
{
  typedef long Element_t;
  int       num_of_units = dash::Team::All().size();
  Element_t find_me	     = num_of_units + 24;
  index_t   find_pos     = dash::size() - 1;

  dash::Array<Element_t> array;

  // Array should have same length as num_of_units.
  // Therefore, array.local.size() should be 1.
  array.allocate(num_of_units, dash::BLOCKED);

  ASSERT_EQ_U(array.local.size(), 1);

  array.barrier();

  auto l_size = array.local.size();
  for (size_t l_i = 0; l_i < l_size; l_i++) {
    array.local[l_i] = dash::myid();
    LOG_MESSAGE("array.local[%d] = %d", l_i, array.local[l_i]);
  }

  array.barrier();

  if(dash::myid() == 0){
    array[find_pos] = find_me;
  }

  array.barrier();

  // Run find on complete array
  auto found_gptr = dash::find(array.begin(), array.end(), find_me);

  // Check that the element find_me has been found (found != last)
  LOG_MESSAGE("Completed dash::find");

  // Run find on complete array
  EXPECT_EQ_U(found_gptr, array.begin() + find_pos);

  // Check minimum value found
  Element_t found_v = *found_gptr;
  LOG_MESSAGE("Expected find value: %d, found find value %d",
              find_me, found_v);
  EXPECT_EQ(find_me, found_v);

  array.barrier();
}
#endif

TEST_F(FindTest, AllElementsEqualNoneMatches)
{
  _num_elem = dash::Team::All().size();
  Element_t init_fill = 0;
  Element_t find_me	  = 24;

  // Initialize global array and fill it with init_fill:
  Array_t array(_num_elem);
  if (dash::myid() == 0) {
    for (size_t i = 0; i < array.size(); ++i) {
      LOG_MESSAGE("Setting array[%d] with init_fill %d", i, init_fill);
      array[i] = init_fill;
    }
  }

  // Wait for array initialization
  LOG_MESSAGE("Waiting for other units to initialize array values");
  array.barrier();
  LOG_MESSAGE("Finished initialization of array values");

  // Run find on complete array
  auto found_gptr = dash::find(array.begin(), array.end(), find_me);

  // Check that the element find_me has not been found (found == last):
  LOG_MESSAGE("Completed dash::find");

  EXPECT_EQ(found_gptr, array.end());

  array.barrier();
}

TEST_F(FindTest, AllElementsEqualAllMatch)
{
  _num_elem           = dash::Team::All().size();
  Element_t find_me	  = 24;
  Element_t init_fill = 24;

  // Initialize global array and fill it with init_fill:
  Array_t array(_num_elem);

  if (dash::myid() == 0) {
    for (size_t i = 0; i < array.size(); ++i) {
      LOG_MESSAGE("Setting array[%d] with init_fill", i, init_fill);
      array[i] = init_fill;
    }
  }

  // Wait for array initialization
  LOG_MESSAGE("Waiting for other units to initialize array values");
  array.barrier();
  LOG_MESSAGE("Finished initialization of array values");

  // Run find on complete array
  auto found_gptr = dash::find(array.begin(), array.end(), find_me);

  // As every element is equal, array.begin() should be return value by
  // definition.
  LOG_MESSAGE("Completed dash::find");

  // Run find on complete array
  ASSERT_EQ(found_gptr, array.begin());
}

TEST_F(FindTest, SingleMatchInSingleUnit)
{
  int       num_of_units      = dash::Team::All().size();

  if (num_of_units < 2) {
    LOG_MESSAGE("Test case requires nunits > 1");
    return;
  }

  Element_t find_me	          = 1;
  index_t   find_pos          = 5;
  Element_t init_fill         = 0;
  dash::global_unit_t unit_cntng_find(dash::Team::All().size() % 2);

  dash::Array<Element_t> array;

  // Array should have same length as num_of_units, therefore,
  // array.local.size() should be 1
  array.allocate(num_of_units * 7, dash::BLOCKED);

  if (dash::myid() == 0) {
    for (size_t i = 0; i < array.size(); ++i) {
      LOG_MESSAGE("Setting array[%d] with init_fill", i, init_fill);
      array[i] = init_fill;
    }
  }

  array.barrier();

  if (dash::myid() == unit_cntng_find) {
    array.local[find_pos] = find_me;
  }

  array.barrier();
  LOG_MESSAGE("Finished initialization of array values");

  // Run find on complete array
  auto found_gptr = dash::find(array.begin(), array.end(), find_me);
  LOG_MESSAGE("Completed dash::find");

  Element_t found_v = *found_gptr;
  LOG_MESSAGE("Expected find value: %d, found find value %d",
              find_me, found_v);
  EXPECT_EQ(find_me, found_v);

  array.barrier();
}

/*
 * This TEST_F does not yet make sense to test what it is supposed to test.
 */
TEST_F(FindTest, SingleMatchInEveryUnit)
{
  int       num_of_units    = dash::Team::All().size();
  Element_t find_me	        = 1;
  index_t   find_pos        = 5;
  Element_t init_fill       = 0;

  dash::Array<Element_t> array;
  array.allocate(num_of_units * 7, dash::BLOCKED);

  if (dash::myid() == 0) {
    for (size_t i = 0; i < array.size(); ++i) {
      LOG_MESSAGE("Setting array[%d] with init_fill %d", i, init_fill);
      array[i] = init_fill;
    }
  }
  array.barrier();
  LOG_MESSAGE("Finished initialization of array values");

  array.local[find_pos] = find_me;

  array.barrier();
  LOG_MESSAGE("In every local array postion %d set to value %d", find_pos,
              find_me);


  // Run find on complete array
  auto found_gptr = dash::find(array.begin(), array.end(), find_me);

  LOG_MESSAGE("Completed dash::find");

  // Run find on complete array

  Element_t found_v = *found_gptr;
  LOG_MESSAGE("Expected find value: %d, found find value %d",
              find_me, found_v);
  EXPECT_EQ(find_me, found_v);

  array.barrier();
}

TEST_F(FindTest, EmptyContainer)
{
  Element_t find_me = 1;

  dash::Array<Element_t> array;
  array.allocate(0, dash::BLOCKED);

  // Run find on complete array
  auto found_gptr = dash::find(array.begin(), array.end(), find_me);

  LOG_MESSAGE("Completed dash::find");

  // Run find on complete array
  EXPECT_EQ(array.end(), found_gptr);
}

TEST_F(FindTest, LessElementsThanUnits)
{
  int num_of_units  = dash::Team::All().size();

  LOG_MESSAGE("Number of units is %d", num_of_units);

  if (num_of_units < 2) {
    LOG_MESSAGE("Test case requires nunits > 1");
    return;
  }

  Element_t find_me	    = 1;
  Element_t init_fill     = 0;

  Array_t array(num_of_units - 1);

  index_t find_pos = static_cast<index_t>(array.size() / 2);

  if (dash::myid() == 0) {
    for (size_t i = 0; i < array.size(); ++i) {
      LOG_MESSAGE("Setting array[%d] with init_fill", i, init_fill);
      array[i]      = init_fill;
    }

    array[find_pos] = find_me;
  }

  array.barrier();

  // Run find on complete array

  auto found_gptr = dash::find(array.begin(), array.end(), find_me);

  // Check that the element find_me has been found

  LOG_MESSAGE("Completed dash::find");

  // Run find on complete array
  EXPECT_NE_U(found_gptr, array.end());

  // Check minimum value found
  Element_t found_v = *found_gptr;
  LOG_MESSAGE("Expected find value: %d, found find value %d",
              find_me, found_v);
  EXPECT_EQ(find_me, found_v);

  array.barrier();
}

