#include <libdash.h>
#include <gtest/gtest.h>
#include "TestBase.h"
#include "MakePatternTest.h"

#include <dash/MakePattern.h>

using namespace dash;

TEST_F(MakePatternTest, DefaultTraits)
{
  size_t extent_x   = 20 * _dash_size;
  size_t extent_y   = 30 * _dash_size;
  auto sizespec     = dash::SizeSpec<2>(extent_x, extent_y);
  auto teamspec     = dash::TeamSpec<2>(_dash_size, 1);

  auto dflt_pattern = dash::make_pattern(
                        sizespec,
                        teamspec);
  // Test pattern type traits:
  ASSERT_TRUE_U(dflt_pattern.is_strided());
}

TEST_F(MakePatternTest, VarArgTags)
{
  size_t extent_x   = 20 * _dash_size;
  size_t extent_y   = 30 * _dash_size;
  auto sizespec     = dash::SizeSpec<2>(extent_x, extent_y);
  auto teamspec     = dash::TeamSpec<2>(_dash_size, 1);

  // Tiled pattern with one tag in blocking property category and two tags
  // in mapping property category:
  auto tile_pattern   = dash::make_pattern<
                          // Blocking constraints:
                          pattern_blocking_traits<
                            // same number of elements in every block
                            pattern_blocking_tag::balanced
                          >,
                          // Mapping constraints:
                          pattern_mapping_traits<
                            // same amount of blocks for every process
                            pattern_mapping_tag::balanced,
                            // every process mapped in every row/column
                            pattern_mapping_tag::diagonal
                          >,
                          // Linearization constraints:
                          pattern_linearization_traits<
                            // elements contiguous within block
                            pattern_linearization_tag::in_block
                          >
                        >(sizespec, teamspec);
  // Test pattern type traits:
  ASSERT_FALSE_U(tile_pattern.is_strided());

  // Strided pattern with two tags in blocking property category and one tag
  // in mapping property category:
  auto stride_pattern = dash::make_pattern<
                          // Blocking constraints:
                          pattern_blocking_traits<
                            // same number of elements in every block
                            pattern_blocking_tag::balanced,
                            // elements in block should fit into a cache line
                            pattern_blocking_tag::cache_sized
                          >,
                          // Mapping constraints:
                          pattern_mapping_traits<
                            // same amount of blocks for every process
                            pattern_mapping_tag::balanced,
                            // Unit mapped to block differs from neighbors
                            pattern_mapping_tag::remote_neighbors
                          >,
                          // Linearization constraints:
                          pattern_linearization_traits<
                            // elements contiguous within block
                            pattern_linearization_tag::strided
                          >
                        >(sizespec, teamspec);
  // Test pattern type traits:
  ASSERT_TRUE_U(stride_pattern.is_strided());
}
