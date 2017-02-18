
#include "SUMMATest.h"

#include <dash/algorithm/SUMMA.h>
#include <dash/Matrix.h>

#include <sstream>
#include <iomanip>


#define SKIP_TEST_IF_NO_SUMMA()           \
  auto conf = dash::util::DashConfig;     \
  if (!conf.avail_algo_summa) {           \
    SKIP_TEST_MSG("SUMMA not available"); \
  }                                       \
  do { } while(0)


TEST_F(SUMMATest, Deduction)
{
  SKIP_TEST_IF_NO_SUMMA();

  size_t num_units   = dash::Team::All().size();
  size_t team_size_x = num_units;
  size_t team_size_y = 1;

  size_t extent_cols = num_units;
  size_t extent_rows = num_units;

#if 0
  // For explicit definition of data distribution:
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
  team_spec.balance_extents();

  LOG_MESSAGE("Initialize matrix pattern ...");
  auto pattern = dash::make_pattern<
                   dash::summa_pattern_partitioning_constraints,
                   dash::summa_pattern_mapping_constraints,
                   dash::summa_pattern_layout_constraints
                 >(size_spec,
                   team_spec);

  LOG_MESSAGE("SizeSpec(%lu,%lu) TeamSpec(%lu,%lu)",
              size_spec.extent(0), size_spec.extent(1),
              team_spec.extent(0), team_spec.extent(1));

  typedef double                         value_t;
  typedef decltype(pattern)              pattern_t;
  typedef typename pattern_t::index_type index_t;

  if (dash::myid().id == 0) {
    dash::test::print_pattern_mapping(
      "pattern.unit_at", pattern, 3,
      [](const pattern_t & _pattern, int _x, int _y) -> dart_unit_t {
          return _pattern.unit_at(std::array<index_t, 2> {_x, _y});
      });
  }

  LOG_MESSAGE("Deduced pattern: "
              "size(%lu,%lu) tilesize(%lu,%lu) teamsize(%lu,%lu) disttype(%d,%d)",
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
//ASSERT_TRUE_U(
//  dash::pattern_partitioning_traits<decltype(pattern)>::type::minimal);
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

  LOG_MESSAGE("Starting initialization of matrix values");
  dash::barrier();

  // Initialize operands:
  if (dash::myid().id == 0) {
    // Matrix B is identity matrix:
    for (index_t d = 0; d < static_cast<index_t>(extent_rows); ++d) {
      DASH_LOG_TRACE("SUMMATest.Deduction",
                     "setting matrix B value (",d,",",d,")");
      matrix_b[d][d] = 1;
    }
    for (index_t row = 0; row < static_cast<index_t>(extent_rows); ++row) {
      for (index_t col = 0; col < static_cast<index_t>(extent_cols); ++col) {
        DASH_LOG_TRACE("SUMMATest.Deduction",
                       "initialize matrix A value (",col,",",row,")");
        auto unit  = matrix_a.pattern()
                             .unit_at(std::array<index_t, 2> { col, row });
        value_t value = ((1 + col) * 10000) + ((row + 1) * 100) + unit;
        DASH_LOG_TRACE("SUMMATest.Deduction",
                       "setting matrix A value (",col,",",row,")");
        matrix_a[col][row] = value;
      }
    }
  }

  LOG_MESSAGE("Waiting for initialization of matrices ...");
  dash::barrier();

  // Expected to be resolved to SUMMA version of dash::mmult:
  LOG_MESSAGE("Calling dash::mmult ...");
  dash::mmult(matrix_a,
                 matrix_b,
                 matrix_c);

  if (dash::myid().id == 0) {
    dash::test::print_matrix("summa.matrix A", matrix_a, 3);
    dash::test::print_matrix("summa.matrix B", matrix_b, 3);
    dash::test::print_matrix("summa.matrix C", matrix_c, 3);
  }

  dash::barrier();

  // Verify multiplication result (A x id = A):
  if (false && dash::myid().id == 0) {
    // Multiplication of matrix A with identity matrix B should be identical
    // to matrix A:
    for (index_t row = 0; row < static_cast<index_t>(extent_rows); ++row) {
      for (index_t col = 0; col < static_cast<index_t>(extent_cols); ++col) {
        auto unit = matrix_a.pattern()
                            .unit_at(std::array<index_t, 2> { col, row });
        value_t expect = ((1 + col) * 10000) + ((row + 1) * 100) + unit;
        value_t actual = matrix_c[col][row];
        ASSERT_EQ_U(expect, actual);
      }
    }
  }

  dash::barrier();
}

TEST_F(SUMMATest, SeqTilePatternMatrix)
{
  SKIP_TEST_IF_NO_SUMMA();

  typedef dash::SeqTilePattern<2>        pattern_t;
  typedef double                         value_t;
  typedef typename pattern_t::index_type index_t;
  typedef typename pattern_t::size_type  extent_t;

  extent_t tile_size   = 7;
  extent_t base_size   = tile_size * 3;
  extent_t extent_rows = dash::size() * base_size;
  extent_t extent_cols = dash::size() * base_size;
  dash::SizeSpec<2> size_spec(extent_rows, extent_cols);

  auto team_spec = dash::make_team_spec<
                     dash::summa_pattern_partitioning_constraints,
                     dash::summa_pattern_mapping_constraints,
                     dash::summa_pattern_layout_constraints >(
                       size_spec);

  dash::DistributionSpec<2> dist_spec(dash::TILE(tile_size),
                                      dash::TILE(tile_size));
  pattern_t pattern(size_spec, dist_spec, team_spec);

  // Create operands and result matrices with identical distribution pattern:
  LOG_MESSAGE("Initialize matrix instances ...");
  dash::Matrix<value_t, 2, index_t, pattern_t> matrix_a(pattern);
  dash::Matrix<value_t, 2, index_t, pattern_t> matrix_b(pattern);
  dash::Matrix<value_t, 2, index_t, pattern_t> matrix_c(pattern);

  LOG_MESSAGE("Starting initialization of matrix values");
  dash::barrier();

  // Initialize operands:
  if (dash::myid().id == 0) {
    // Matrix B is identity matrix:
    for (index_t d = 0; d < static_cast<index_t>(extent_rows); ++d) {
      DASH_LOG_TRACE("SUMMATest.Deduction",
                     "setting matrix B value (",d,",",d,")");
      matrix_b[d][d] = 1;
    }
    for (index_t row = 0; row < static_cast<index_t>(extent_rows); ++row) {
      for (index_t col = 0; col < static_cast<index_t>(extent_cols); ++col) {
        DASH_LOG_TRACE("SUMMATest.Deduction",
                       "initialize matrix A value (",col,",",row,")");
        auto unit  = matrix_a.pattern()
                             .unit_at(std::array<index_t, 2> { col, row });
        value_t value = ((1 + col) * 10000) + ((row + 1) * 100) + unit;
        DASH_LOG_TRACE("SUMMATest.Deduction",
                       "setting matrix A value (",col,",",row,")");
        matrix_a[col][row] = value;
      }
    }
  }

  LOG_MESSAGE("Waiting for initialization of matrices ...");
  dash::barrier();

  // Expected to be resolved to SUMMA version of dash::mmult:
  LOG_MESSAGE("Calling dash::mmult ...");

  dash::util::TraceStore::on();
  dash::util::TraceStore::clear();
  dash::util::Trace trace("SUMMATest.SeqTilePatternMatrix");
  dash::mmult(matrix_a,
                 matrix_b,
                 matrix_c);

  dash::barrier();
  dash::util::TraceStore::off();
  dash::util::TraceStore::write(std::cout);

  if (dash::myid().id == 0) {
    dash::test::print_matrix("summa.matrix A", matrix_a, 3);
    dash::test::print_matrix("summa.matrix B", matrix_b, 3);
    dash::test::print_matrix("summa.matrix C", matrix_c, 3);
  }

  dash::barrier();

  // Verify multiplication result (A x id = A):
  if (false && dash::myid().id == 0) {
    // Multiplication of matrix A with identity matrix B should be identical
    // to matrix A:
    for (index_t row = 0; row < static_cast<index_t>(extent_rows); ++row) {
      for (index_t col = 0; col < static_cast<index_t>(extent_cols); ++col) {
        auto unit = matrix_a.pattern()
                            .unit_at(std::array<index_t, 2> { col, row });
        value_t expect = ((1 + col) * 10000) + ((row + 1) * 100) + unit;
        value_t actual = matrix_c[col][row];
        ASSERT_EQ_U(expect, actual);
      }
    }
  }

  dash::barrier();
}
