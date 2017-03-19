
#include "MakePatternTest.h"

#include <dash/pattern/MakePattern.h>
#include <dash/pattern/PatternProperties.h>
#include <dash/Dimensional.h>
#include <dash/TeamSpec.h>


using namespace dash;

TEST_F(MakePatternTest, DefaultTraits)
{
  size_t extent_x   = 20 * dash::size();
  size_t extent_y   = 30 * dash::size();
  auto sizespec     = dash::SizeSpec<2>(extent_x, extent_y);
  auto teamspec     = dash::TeamSpec<2>(dash::size(), 1);

  auto dflt_pattern = dash::make_pattern(
                        sizespec,
                        teamspec);
  // Test pattern type traits and default properties:
  ASSERT_TRUE_U(
    dash::pattern_layout_traits<
      decltype(dflt_pattern)
    >::type::linear);
  ASSERT_TRUE_U(
    dash::pattern_layout_traits<
      decltype(dflt_pattern)
    >::type::canonical);
  ASSERT_FALSE_U(
    dash::pattern_layout_traits<
      decltype(dflt_pattern)
    >::type::blocked);
}

TEST_F(MakePatternTest, VarArgTags)
{
  size_t extent_x   = 20 * dash::size();
  size_t extent_y   = 30 * dash::size();
  auto sizespec     = dash::SizeSpec<2>(extent_x, extent_y);
  auto teamspec     = dash::TeamSpec<2>(dash::size(), 1);

  // Tiled pattern with one tag in partitioning property category and two tags
  // in mapping property category:
  auto tile_pattern   = dash::make_pattern<
                          // Blocking constraints:
                          pattern_partitioning_properties<
                            // same number of elements in every block
                            pattern_partitioning_tag::balanced,
                            // rectangular blocks
                            pattern_partitioning_tag::rectangular
                          >,
                          // Topology constraints:
                          pattern_mapping_properties<
                            // same amount of blocks for every process
                            pattern_mapping_tag::balanced,
                            // every process mapped in every row/column
                            pattern_mapping_tag::diagonal
                          >,
                          // Linearization constraints:
                          pattern_layout_properties<
                            // elements contiguous within blocks
                            pattern_layout_tag::blocked,
                            // elements in linear order within blocks
                            pattern_layout_tag::linear
                          >
                        >(sizespec, teamspec);
  // Test pattern type traits:
  ASSERT_FALSE_U(
    dash::pattern_layout_traits <
      decltype(tile_pattern)
    >::type::canonical);
  ASSERT_TRUE_U(
    dash::pattern_layout_traits<
      decltype(tile_pattern)
    >::type::linear);
  ASSERT_TRUE_U(
    dash::pattern_partitioning_traits <
      decltype(tile_pattern)
    >::type::balanced);
  ASSERT_TRUE_U(
    dash::pattern_mapping_traits <
      decltype(tile_pattern)
    >::type::diagonal);
  ASSERT_TRUE_U(
    dash::pattern_mapping_traits <
      decltype(tile_pattern)
    >::type::balanced);

  // Strided pattern with two tags in partitioning property category and one
  // tag in mapping property category:
  auto stride_pattern = dash::make_pattern<
                          // Blocking constraints:
                          pattern_partitioning_properties<
                            // same number of elements in every block
                            pattern_partitioning_tag::balanced,
                            // rectangular blocks
                            pattern_partitioning_tag::rectangular
                          >,
                          // Topology constraints:
                          pattern_mapping_properties<
                            // same amount of blocks for every process
                            pattern_mapping_tag::balanced,
                            // Unit mapped to block differs from neighbors
                            pattern_mapping_tag::neighbor
                          >,
                          // Linearization constraints:
                          pattern_layout_properties<
                            // local element order corresponds to linearized
                            // canonical order
                            pattern_layout_tag::linear,
                            // all local elements in single logical index
                            // domain
                            pattern_layout_tag::canonical
                          >
                        >(sizespec, teamspec);
  // Test pattern type traits:
  ASSERT_TRUE_U(
    dash::pattern_layout_traits<
      decltype(stride_pattern)
    >::type::canonical);
  ASSERT_TRUE_U(
    dash::pattern_layout_traits<
      decltype(stride_pattern)
    >::type::linear);
  ASSERT_FALSE_U(
    dash::pattern_layout_traits<
      decltype(stride_pattern)
    >::type::blocked);
}
