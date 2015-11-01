#include <libdash.h>
#include <gtest/gtest.h>
#include "TestBase.h"
#include "MakePatternTest.h"

#include <dash/MakePattern.h>

TEST_F(MakePatternTest, DefaultTraits)
{
  size_t extent_x   = 20 * _dash_size;
  size_t extent_y   = 30 * _dash_size;
  auto sizespec     = dash::SizeSpec<2>(extent_x, extent_y);
  auto teamspec     = dash::TeamSpec<2>(_dash_size, 1);

  auto tile_pattern   = dash::make_pattern<
                          // Blocking constraints:
                          dash::pattern_blocking_traits<
                            // same number of elements in every block
                            dash::pattern_blocking_tag::balanced
//                          // elements in block should fit into a cache line
//                          dash::pattern_blockint_tag::fit_cache 
                          >,
                          // Mapping constraints:
                          dash::pattern_mapping_traits<
                            // same amount of blocks for every process
                            dash::pattern_mapping_tag::balanced
//                          // every process mapped in every row/column
//                          dash::pattern_mapping_diagonal
                          >,
                          // Linearization constraints:
                          dash::pattern_linearization_traits<
                            // elements contiguous within block
                            dash::pattern_linearization_tag::in_block
                          >
                        >(sizespec, teamspec);
  // Test pattern type traits:
  ASSERT_FALSE_U(tile_pattern.is_strided());

  auto stride_pattern = dash::make_pattern<
                          // Blocking constraints:
                          dash::pattern_blocking_traits<
                            // same number of elements in every block
                            dash::pattern_blocking_tag::balanced
//                          // elements in block should fit into a cache line
//                          dash::pattern_blockint_tag::fit_cache 
                          >,
                          // Mapping constraints:
                          dash::pattern_mapping_traits<
                            // same amount of blocks for every process
                            dash::pattern_mapping_tag::balanced
//                          // every process mapped in every row/column
//                          dash::pattern_mapping_diagonal
                          >,
                          // Linearization constraints:
                          dash::pattern_linearization_traits<
                            // elements contiguous within block
                            dash::pattern_linearization_tag::strided
                          >
                        >(sizespec, teamspec);
  // Test pattern type traits:
  ASSERT_TRUE_U(stride_pattern.is_strided());
}
