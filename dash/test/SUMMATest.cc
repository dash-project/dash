#include <libdash.h>
#include <gtest/gtest.h>
#include "TestBase.h"
#include "SUMMATest.h"

TEST_F(SUMMATest, Deduction)
{
  typedef int                  value_t;
  typedef dash::TilePattern<2> pattern_t;

  dart_unit_t myid   = dash::myid();
  size_t num_units   = dash::Team::All().size();
  // Use square matrices for operands and result:
  size_t tilesize_x  = 3;
  size_t tilesize_y  = 3;
  size_t num_local_blocks_x = 1;
  size_t num_local_blocks_y = 1;
  size_t extent_cols = tilesize_x * num_units * num_local_blocks_x;
  size_t extent_rows = tilesize_y * num_units * num_local_blocks_y;

  if (num_units % 2 > 0) {
    LOG_MESSAGE("Team size must be multiple of 2 for SUMMATest.Deduction");
    return;
  }

  dash::SizeSpec<2> size_spec(extent_cols, extent_rows);
  dash::TeamSpec<2> team_spec(num_units, 1);

  pattern_t pattern(
    dash::SizeSpec<2>(
      extent_cols,
      extent_rows),
    dash::DistributionSpec<2>(
      dash::TILE(tilesize_x),
      dash::TILE(tilesize_y))
  );

#if DEFUNCT
  // Automatically deduce pattern type satisfying constraints defined by
  // SUMMA implementation:
  LOG_MESSAGE("Initialize matrix pattern ...");
  auto pattern = dash::make_pattern <
                 dash::summa_pattern_blocking_constraints,
                 dash::summa_pattern_topology_constraints,
                 dash::summa_pattern_indexing_constraints > (
                   size_spec,
                   team_spec);
#endif
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
  dash::Matrix<value_t, 2> matrix_a(pattern);
  dash::Matrix<value_t, 2> matrix_b(pattern);
  dash::Matrix<value_t, 2> matrix_c(pattern);

  // Initialize operands:
  if (_dash_id == 0) {
    for (auto col = 0; col < pattern.extent(0); ++col) {
      for (auto row = 0; row < pattern.extent(1); ++row) {
        matrix_a[col][row] = ((1 + col) * 1000) + (row + 1);
      }
    }
    // Matrix B is identity matrix:
    for (auto diag_idx = 0; diag_idx < pattern.extent(0); ++diag_idx) {
      matrix_b[diag_idx][diag_idx] = 1;
    }
  }

  LOG_MESSAGE("Waiting for barrier ...");
  dash::barrier();

  // Expected to be resolved to SUMMA version of dash::multiply:
  LOG_MESSAGE("Calling dash::multiply ...");
  dash::multiply(matrix_a,
                 matrix_b,
                 matrix_c);

  if (_dash_id == 0) {
    print_matrix("matrix A", matrix_a);
    print_matrix("matrix B", matrix_b);
    print_matrix("matrix C", matrix_c);
  }

  dash::barrier();

  // Verify multiplication result (id x id = id):
  if (_dash_id == 0) {
    // Multiplication of matrix A with identity matrix B should be identical
    // to matrix A:
    for (auto col = 0; col < pattern.extent(0); ++col) {
      for (auto row = 0; row < pattern.extent(1); ++row) {
        value_t expected = ((1 + col) * 1000) + (row + 1);
        value_t actual   = matrix_c[col][row];
        ASSERT_EQ_U(expected, actual);
      }
    }
  }
}
