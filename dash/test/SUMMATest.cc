#include <libdash.h>
#include <gtest/gtest.h>
#include "TestBase.h"
#include "SUMMATest.h"

TEST_F(SUMMATest, Deduction)
{
  dart_unit_t myid   = dash::myid();
  size_t num_units   = dash::Team::All().size();
  // Use square matrices for operands and result:
  size_t tilesize_x  = 3;
  size_t tilesize_y  = 3;
  size_t extent_cols = tilesize_x * num_units * 2;
  size_t extent_rows = tilesize_y * num_units * 2;

  dash::SizeSpec<2> size_spec(extent_cols, extent_rows);
  dash::TeamSpec<2> team_spec(num_units, 1);

  // Automatically deduce pattern type satisfying constraints defined by
  // SUMMA implementation:
  LOG_MESSAGE("Initialize matrix pattern ...");
  auto pattern = dash::make_pattern <
                 dash::summa_pattern_blocking_constraints,
                 dash::summa_pattern_topology_constraints,
                 dash::summa_pattern_indexing_constraints > (
                   size_spec,
                   team_spec);
  // Plausibility check of single pattern traits:
  ASSERT_FALSE_U(
    dash::pattern_indexing_traits <
      decltype(pattern)
    >::type::local_strided);
  ASSERT_TRUE_U(
    dash::pattern_indexing_traits <
      decltype(pattern)
    >::type::local_phase);
  ASSERT_TRUE_U(
    dash::pattern_blocking_traits <
      decltype(pattern)
    >::type::balanced);
  ASSERT_TRUE_U(
    dash::pattern_topology_traits <
      decltype(pattern)
    >::type::diagonal);
  ASSERT_TRUE_U(
    dash::pattern_topology_traits <
      decltype(pattern)
    >::type::balanced);

  // Test pattern constraints verification. Pattern has been deduced from
  // a set of constraints, so it is expected to satisfy these constraints:
  const bool constraints_matched =
    dash::check_pattern_constraints <
        dash::summa_pattern_blocking_constraints,
        dash::summa_pattern_topology_constraints,
        dash::summa_pattern_indexing_constraints >(
      pattern);

  // Create operands and result matrices with identical distribution pattern:
  LOG_MESSAGE("Initialize matrix instances ...");
  dash::Matrix<int, 2> matrix_a(pattern);
  dash::Matrix<int, 2> matrix_b(pattern);
  dash::Matrix<int, 2> matrix_c(pattern);

  // Expected to be resolved to SUMMA version of dash::multiply:
  LOG_MESSAGE("Calling dash::multiply ...");
  dash::multiply(matrix_a,
                 matrix_b,
                 matrix_c);
}
