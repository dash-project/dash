#include <libdash.h>
#include <gtest/gtest.h>

#include <limits>

#include "TestBase.h"
#include "MismatchTest.h"

Test_F(MismatchTest, TestSimpleMismatch) {
 _num_elem               = dash::Team::All().size();
 Element_t init_fill     = 0;
 Element_t mismatch_fill = 1;
 
 //Initialize global array and fill it with init_fill: 
 Array_t array_1(_num_elem);
 if (dash::myid()==0) {
   for (auto i=0; i < array.size(); ++i) {
     LOG_MESSAGE("Setting array[%d] with init_fill = ", i, init_fill);
     array[i]            = init_fill;
   }
 }
 array_1.barrier;
 
 //Copy array and modify it
 Array_t array_2        = array_1;
 array_2.barrier;
 Element_t mismatch_t   = _num_elem / 2; 
 array_2[mismatch_t]    = mismatch_fill;
 array_2.barrier;
 LOG_MESSAGE("Finished initialization of two arrays");

 //Check dash::algorithm::mismatch
 LOG_MESSAGE("Start test of mismatch");
 auto test_result = dash::mismatch(array_1.begin, array_1.end,
                                   array_2.begin, array_2.end,
                                   ==);

 EXPECT_NE_EQ(test_result.first, test_result.second);

}

