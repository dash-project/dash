
#include "CartesianTest.h"

#include <dash/Cartesian.h>

#include <array>
#include <numeric>
#include <functional>


TEST_F(CartesianTest, DefaultConstructor) {
  DASH_TEST_LOCAL_ONLY();
  // 1-dimensional:
  dash::CartesianIndexSpace<1> cartesian1d;
  EXPECT_EQ(cartesian1d.size(), 0);
  EXPECT_EQ(cartesian1d.rank(), 1);
  EXPECT_EQ(cartesian1d.extent(0), 0);
#if defined(DASH_ENABLE_ASSERTIONS)
  dash::internal::logging::disable_log();
  EXPECT_THROW(
    cartesian1d.at(0),
    dash::exception::OutOfRange);
  EXPECT_THROW(
    cartesian1d.coords(0),
    dash::exception::OutOfRange);
  dash::internal::logging::enable_log();
#endif
  // 4-dimensional:
  dash::CartesianIndexSpace<4> cartesian4d;
  EXPECT_EQ(cartesian4d.size(), 0);
  EXPECT_EQ(cartesian4d.rank(), 4);
  EXPECT_EQ(cartesian4d.extent(0), 0);
  EXPECT_EQ(cartesian4d.extent(1), 0);
  EXPECT_EQ(cartesian4d.extent(2), 0);
  EXPECT_EQ(cartesian4d.extent(3), 0);
#if defined(DASH_ENABLE_ASSERTIONS)
  dash::internal::logging::disable_log();
  EXPECT_THROW(
    cartesian4d.at(0, 0, 0, 0),
    dash::exception::OutOfRange);
  EXPECT_THROW(
    cartesian4d.coords(0),
    dash::exception::OutOfRange);
  dash::internal::logging::enable_log();
#endif
}

TEST_F(CartesianTest, Conversion1Dim) {
  DASH_TEST_LOCAL_ONLY();
  int extent = 42;
  dash::CartesianIndexSpace<1> cartesian1d(extent);
  EXPECT_EQ(cartesian1d.rank(), 1);
  EXPECT_EQ(cartesian1d.size(), extent);
  EXPECT_EQ(cartesian1d.extent(0), extent);
  for (size_t i = 0; i < extent; ++i) {
    EXPECT_EQ(cartesian1d.at(i), i);
    EXPECT_EQ(cartesian1d.coords(i)[0], i);
  }
}

TEST_F(CartesianTest, Conversion2Dim) {
  DASH_TEST_LOCAL_ONLY();
  int extent_x = 3;
  int extent_y = 5;
  dash::CartesianIndexSpace<2, dash::ROW_MAJOR, size_t> cartesian2dR(
    extent_x, extent_y);
  dash::CartesianIndexSpace<2, dash::COL_MAJOR, size_t> cartesian2dC(
    extent_x, extent_y);
  EXPECT_EQ(cartesian2dR.rank(), 2);
  EXPECT_EQ(cartesian2dC.rank(), 2);
  EXPECT_EQ(cartesian2dR.size(), extent_x * extent_y);
  EXPECT_EQ(cartesian2dC.size(), extent_x * extent_y);
  EXPECT_EQ(cartesian2dR.extent(0), extent_x);
  EXPECT_EQ(cartesian2dR.extent(0), extent_x);
  EXPECT_EQ(cartesian2dC.extent(1), extent_y);
  EXPECT_EQ(cartesian2dC.extent(1), extent_y);
  for (size_t x = 0; x < extent_x; ++x) {
    for (size_t y = 0; y < extent_y; ++y) {
      size_t exp_index_col_major = (y * extent_x) + x;
      size_t exp_index_row_major = (x * extent_y) + y;
      EXPECT_EQ(exp_index_row_major, cartesian2dR.at(x, y));
      EXPECT_EQ(x, cartesian2dR.coords(exp_index_row_major)[0]);
      EXPECT_EQ(y, cartesian2dR.coords(exp_index_row_major)[1]);
      EXPECT_EQ(exp_index_col_major, cartesian2dC.at(x, y));
      EXPECT_EQ(x, cartesian2dC.coords(exp_index_col_major)[0]);
      EXPECT_EQ(y, cartesian2dC.coords(exp_index_col_major)[1]);
    }
  }
}

TEST_F(CartesianTest, Conversion3Dim) {
  DASH_TEST_LOCAL_ONLY();
  int extent_x = 5;
  int extent_y = 7;
  int extent_z = 11;
  size_t size  = extent_x * extent_y * extent_z;
  dash::CartesianIndexSpace<3, dash::ROW_MAJOR, size_t> cartesian3dR(
    extent_x, extent_y, extent_z);
  dash::CartesianIndexSpace<3, dash::COL_MAJOR, size_t> cartesian3dC(
    extent_x, extent_y, extent_z);
  ASSERT_EQ(cartesian3dR.rank(), 3);
  ASSERT_EQ(cartesian3dC.rank(), 3);
  ASSERT_EQ(cartesian3dR.size(), size);
  ASSERT_EQ(cartesian3dC.size(), size);
  ASSERT_EQ(cartesian3dR.extent(0), extent_x);
  ASSERT_EQ(cartesian3dR.extent(0), extent_x);
  ASSERT_EQ(cartesian3dC.extent(1), extent_y);
  ASSERT_EQ(cartesian3dC.extent(1), extent_y);
  ASSERT_EQ(cartesian3dC.extent(2), extent_z);
  ASSERT_EQ(cartesian3dC.extent(2), extent_z);
  for (size_t x = 0; x < extent_x; ++x) {
    for (size_t y = 0; y < extent_y; ++y) {
      for (size_t z = 0; z < extent_z; ++z) {
        size_t exp_index_col_major =
          (z * extent_x * extent_y) + (y * extent_x) + x;
        size_t exp_index_row_major =
          (x * extent_y * extent_z) + (y * extent_z) + z;
        ASSERT_EQ(exp_index_row_major, cartesian3dR.at(x, y, z));
        ASSERT_EQ(x, cartesian3dR.coords(exp_index_row_major)[0]);
        ASSERT_EQ(y, cartesian3dR.coords(exp_index_row_major)[1]);
        ASSERT_EQ(exp_index_col_major, cartesian3dC.at(x, y, z));
        ASSERT_EQ(x, cartesian3dC.coords(exp_index_col_major)[0]);
        ASSERT_EQ(y, cartesian3dC.coords(exp_index_col_major)[1]);
      }
    }
  }
}

TEST_F(CartesianTest, Conversion10Dim) {
  DASH_TEST_LOCAL_ONLY();
  const size_t Dimensions = 10;
  ::std::array<size_t, Dimensions> extents =
    { 3, 13, 17, 23, 2, 3, 1, 1, 2, 2 };
  size_t size = ::std::accumulate(extents.begin(), extents.end(), 1,
                                  ::std::multiplies<size_t>());
  dash::CartesianIndexSpace<Dimensions, dash::ROW_MAJOR, size_t>
    cartesianR(extents);
  dash::CartesianIndexSpace<Dimensions, dash::COL_MAJOR, size_t>
    cartesianC(extents);
  EXPECT_EQ(cartesianR.rank(), Dimensions);
  EXPECT_EQ(cartesianC.rank(), Dimensions);
  EXPECT_EQ(cartesianR.size(), size);
  EXPECT_EQ(cartesianC.size(), size);
  for (int d = 0; d < Dimensions; ++d) {
    EXPECT_EQ(cartesianR.extent(d), extents[d]);
    EXPECT_EQ(cartesianC.extent(d), extents[d]);
  }
}

