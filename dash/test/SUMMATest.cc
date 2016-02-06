#include <libdash.h>
#include <gtest/gtest.h>

#include "TestBase.h"
#include "TestLogHelpers.h"
#include "SUMMATest.h"

TEST_F(SUMMATest, Deduction)
{
  size_t num_units   = dash::Team::All().size();
  size_t tilesize_x  = 2;
  size_t tilesize_y  = 2;

#ifdef DASH_ALGORITHM_SUMMA_MINIMAL_PARTITIONING
  size_t team_size_x = std::max<size_t>(
                         1, static_cast<size_t>(
                              std::ceil(sqrt(num_units))));
  size_t team_size_y = num_units / team_size_x;
  size_t extent_cols = team_size_x * tilesize_x;
  size_t extent_rows = team_size_y * tilesize_y;
  LOG_MESSAGE("team size: %lu x %lu", team_size_x, team_size_y);
#else
  size_t num_local_blocks_x = 1;
  size_t num_local_blocks_y = 1;
  size_t extent_cols = tilesize_x * num_units * num_local_blocks_x;
  size_t extent_rows = tilesize_y * num_units * num_local_blocks_y;
  size_t team_size_x = num_units;
  size_t team_size_y = 1;
#endif

#if 0
  // For explicit specification of data distribution:
  //
  pattern_t pattern(
    dash::SizeSpec<2>(
      extent_cols,
      extent_rows),
    dash::DistributionSpec<2>(
      dash::TILE(tilesize_x),
      dash::TILE(tilesize_y))
  );
#endif

  // Automatically deduce pattern type satisfying constraints defined by
  // SUMMA implementation:
  dash::SizeSpec<2> size_spec(extent_cols, extent_rows);
  dash::TeamSpec<2> team_spec(team_size_x, team_size_y);
  LOG_MESSAGE("Initialize matrix pattern ...");
  auto pattern = dash::make_pattern <
                 dash::summa_pattern_partitioning_constraints,
                 dash::summa_pattern_mapping_constraints,
                 dash::summa_pattern_layout_constraints >(
                   size_spec,
                   team_spec);

  LOG_MESSAGE("SizeSpec(%lu,%lu) TeamSpec(%lu,%lu)",
              size_spec.extent(0), size_spec.extent(1),
              team_spec.extent(0), team_spec.extent(1));

  typedef double                value_t;
  typedef decltype(pattern)     pattern_t;
  typedef pattern_t::index_type index_t;
  typedef pattern_t::size_type  extent_t;

  LOG_MESSAGE("Deduced pattern: "
              "size(%d,%d) tilesize(%d,%d) teamsize(%d,%d) disttype(%d,%d)",
              pattern.extent(0),
              pattern.extent(1),
              pattern.block(0).extent(0),
              pattern.block(0).extent(1),
              pattern.teamspec().extent(0),
              pattern.teamspec().extent(1),
              pattern.distspec()[0].type,
              pattern.distspec()[1].type);

  // Plausibility check of single pattern traits:
  ASSERT_TRUE_U(
    dash::pattern_partitioning_traits<decltype(pattern)>::type::balanced);
#ifdef DASH_ALGORITHM_SUMMA_MINIMAL_PARTITIONING
  ASSERT_TRUE_U(
    dash::pattern_partitioning_traits<decltype(pattern)>::type::minimal);
#else
  ASSERT_TRUE_U(
    dash::pattern_mapping_traits<decltype(pattern)>::type::diagonal);
#endif
  ASSERT_TRUE_U(
    dash::pattern_mapping_traits<decltype(pattern)>::type::unbalanced);
  ASSERT_TRUE_U(
    dash::pattern_layout_traits<decltype(pattern)>::type::blocked);
  ASSERT_TRUE_U(
    dash::pattern_layout_traits<decltype(pattern)>::type::linear);
  ASSERT_FALSE_U(
    dash::pattern_layout_traits<decltype(pattern)>::type::canonical);

  // Test pattern constraints verification. Pattern has been deduced from
  // a set of constraints, so it is expected to satisfy these constraints:
  const bool constraints_matched =
    dash::check_pattern_constraints <
        dash::summa_pattern_partitioning_constraints,
        dash::summa_pattern_mapping_constraints,
        dash::summa_pattern_layout_constraints >(
      pattern);
  ASSERT_TRUE_U(constraints_matched);

  // Create operands and result matrices with identical distribution pattern:
  LOG_MESSAGE("Initialize matrix instances ...");
  dash::Matrix<value_t, 2, index_t, decltype(pattern)> matrix_a(pattern);
  dash::Matrix<value_t, 2, index_t, decltype(pattern)> matrix_b(pattern);
  dash::Matrix<value_t, 2, index_t, decltype(pattern)> matrix_c(pattern);

  // Initialize operands:
  if (_dash_id == 0) {
    // Matrix B is identity matrix:
    for (extent_t diag_idx = 0; diag_idx < pattern.extent(0); ++diag_idx) {
      matrix_b[diag_idx][diag_idx] = 1;
    }
    for (index_t col = 0; col < pattern.extent(0); ++col) {
      for (index_t row = 0; row < pattern.extent(1); ++row) {
        auto unit  = matrix_a.pattern()
                             .unit_at(std::array<index_t, 2> { col, row });
        value_t value = ((1 + col) * 10000) + ((row + 1) * 100) + unit;
//      auto block_x  = col / tilesize_x;
//      auto block_y  = row / tilesize_x;
//      value_t value = static_cast<value_t>(block_x) +
//                      static_cast<value_t>(block_y) / 10.0;
        matrix_a[col][row] = value;
      }
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
    dash::test::print_matrix("summa.matrix A", matrix_a, 3);
    dash::test::print_matrix("summa.matrix B", matrix_b, 3);
    dash::test::print_matrix("summa.matrix C", matrix_c, 3);
  }

  dash::barrier();

  // Verify multiplication result (A x id = A):
  if (false && _dash_id == 0) {
    // Multiplication of matrix A with identity matrix B should be identical
    // to matrix A:
    for (index_t col = 0; col < extent_cols; ++col) {
      for (index_t row = 0; row < extent_rows; ++row) {
        auto unit = matrix_a.pattern()
                            .unit_at(std::array<index_t, 2> { col, row });
        value_t expect = ((1 + col) * 10000) + ((row + 1) * 100) + unit;
//      value_t expect = ((1 + col) * 10) + (row + 1);
        value_t actual = matrix_c[col][row];
        ASSERT_EQ_U(expect, actual);
      }
    }
  }

  dash::barrier();
}
