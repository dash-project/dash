#include <libdash.h>
#include "CartesianTest.h"

TEST_F(CartesianTest, DefaultConstrutor1Dim) {
  dash::CartCoord<1> cartesian;
  EXPECT_EQ(cartesian.size(), 0);
  EXPECT_EQ(cartesian.rank(), 1);
  EXPECT_EQ(cartesian.extent(0), 0);
}

