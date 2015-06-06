#include <libdash.h>
#include "CartesianTest.h"

TEST_F(CartesianTest, DefaultConstrutor) {
  // 1-dimensional:
  dash::CartCoord<1> cartesian1d;
  EXPECT_EQ(cartesian1d.size(), 0);
  EXPECT_EQ(cartesian1d.rank(), 1);
  EXPECT_EQ(cartesian1d.extent(0), 0);
  EXPECT_THROW(
    cartesian1d.at(0),
    dash::exception::OutOfBounds);
  // 4-dimensional:
  dash::CartCoord<4> cartesian4d;
  EXPECT_EQ(cartesian4d.size(), 0);
  EXPECT_EQ(cartesian4d.rank(), 4);
  EXPECT_EQ(cartesian4d.extent(0), 0);
  EXPECT_EQ(cartesian4d.extent(1), 0);
  EXPECT_EQ(cartesian4d.extent(2), 0);
  EXPECT_EQ(cartesian4d.extent(3), 0);
  EXPECT_THROW(
    cartesian4d.at(0, 0, 0, 0),
    dash::exception::OutOfBounds);
}

TEST_F(CartesianTest, Conversion1Dim) {
  int extent = 42;
  dash::CartCoord<1> cartesian1d(extent);
  EXPECT_EQ(cartesian1d.rank(), 1);
  EXPECT_EQ(cartesian1d.extent(0), extent);
  for (int i = 0; i < extent; ++i) {
    EXPECT_EQ(cartesian1d.at(i), i);
  }
}
