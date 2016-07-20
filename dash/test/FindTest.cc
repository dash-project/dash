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
  if (dash::myid() == 0) 
  {	 
    for (auto i = 0; i < array.size(); ++i) 
	{
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
  // Check minimum value found
  Element_t found_v = *found_gptr;
  LOG_MESSAGE("Expected find value: %d, found find value %d",
              find_me, found_v);
  EXPECT_EQ(find_me, found_v);
}


TEST_F(FindTest, Simple)
{
    typedef long value_t;
	int num_of_units = dash::Team::All().size();
	Element_t find_me	  = 24;
	index_t find_pos = 5;
	
	ASSERT_TRUE(num_of_units < find_me);
	
    dash::Array<value_t> array;
	
	// Array should have same length as num_of_units. Therefore, array.local.size() should be 1
    array.allocate(num_of_units, dash::BLOCKED);
	
	if (dash::myid == 0)
	{
		ASSERT_EQ(array.local.size(), 1);
	}
	array.barrier();
	
	
	auto l_size = array.local.size();
	for(auto l_i = 0; l_i < l_size; l_i++)
	{
		array.local[l_i] = dash::myid();
		    LOG_MESSAGE("array.local[%d] = %d", l_i, value);
		
	}
	
	array.barrier();
	
	array.local[find_pos] = find_me;
	
	array.barrier();
	
	// Run find on complete array
	
	auto found_gptr = dash::find(array.begin(), array.end(), find_me);
	
	// Check that the element find_me has been found (found != last)
	
	LOG_MESSAGE("Completed dash::find");
	
	// Run find on complete array
	EXPECT_NE_U(found_gptr, array.end());
	  
	// Check minimum value found
	Element_t found_v = *found_gptr;
	LOG_MESSAGE("Expected find value: %d, found find value %d",
	              find_me, found_v);
	EXPECT_EQ(find_me, found_v);
	
	
	
	
	
}
