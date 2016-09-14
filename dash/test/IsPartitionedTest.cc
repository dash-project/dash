#include <libdash.h>
#include <gtest/gtest.h>

#include <limits>

#include "TestBase.h"
#include "IsPartitionedTest.h"

TEST_F(IsPartitionedTest, TestSimple)
{
  _num_elem           = dash::Team::All().size();
  Element_t init_fill = 0;

  // Initialize global array and fill it with init_fill:
  Array_t array(_num_elem);
  if (dash::myid() == 0) {
    for (size_t i = 0; i < array.size(); ++i) {
      LOG_MESSAGE("Setting array[%d] with init_fill", i, init_fill);
      array[i] = init_fill;
    }
  }

  // Wait for array initialization
  array.barrier();
  LOG_MESSAGE("Finished initialization of array values");

  LOG_MESSAGE("Completed dash::find");
  // Run find on complete array
  /*
  EXPECT_TRUE(dash::is_partitioned(array.begin(),
                                     array.end(),
                                    [](x) {return (x == 0);} );
  */
  }

