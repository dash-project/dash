/*
 * Sequential GUPS benchmark for various pattern types.
 *
 */
/* @DASH_HEADER@ */

#include "../bench.h"
#include <libdash.h>

#include <array>
#include <vector>
#include <deque>
#include <utility>
#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <type_traits>

using std::cout;
using std::endl;

typedef dash::util::Timer<
          dash::util::TimeMeasure::Clock
        > Timer;

typedef double    value_t;
typedef int64_t   index_t;
typedef uint64_t  extent_t;

template<typename MatrixType>
void init_values(
  MatrixType & matrix_a,
  MatrixType & matrix_b,
  MatrixType & matrix_c);

template<typename MatrixType>
std::pair<double, double> test_summa(
  MatrixType & matrix_a,
  MatrixType & matrix_b,
  MatrixType & matrix_c,
  unsigned     repeat);

void perform_test(extent_t n, unsigned repeat);

int main(int argc, char* argv[]) {
#ifndef DASH_ENABLE_MKL
  cout << "WARNING: MKL not available, "
       << "falling back to naive local matrix multiplication"
       << endl;
#endif
  dash::init(&argc, &argv);

  Timer::Calibrate(0);

  std::deque<std::pair<extent_t, unsigned>> tests;

  tests.push_back({0,      0}); // this prints the header
#ifdef DASH_ENABLE_MKL
  tests.push_back({1024, 100});
  tests.push_back({2048,  50});
  tests.push_back({4096,   5});
  tests.push_back({8192,   1});
  tests.push_back({16384,  1});
  tests.push_back({32768,  1});
//tests.push_back({65536,  1});
#else
  tests.push_back({64,   100});
  tests.push_back({256,   50});
  tests.push_back({1024,  10});
  tests.push_back({2048,   1});
#endif

  for (auto test: tests) {
    perform_test(test.first, test.second);
  }

  dash::finalize();

  return 0;
}

void perform_test(
  extent_t n,
  unsigned repeat)
{
  auto num_units = dash::size();
  if (n == 0) {
    if (dash::myid() == 0) {
      cout << std::setw(10)
           << "units"
           << ", "
           << std::setw(10)
           << "n"
           << ", "
           << std::setw(10)
           << "size"
           << ", "
           << std::setw(10)
           << "gflop"
           << ", "
           << std::setw(10)
           << "gflop/s"
           << ", "
           << std::setw(10)
           << "repeats"
           << ", "
           << std::setw(11)
           << "time (s)"
           << ", "
           << std::setw(11)
           << "init time (s)"
           << endl;
    }
    return;
  }

  // Automatically deduce pattern type satisfying constraints defined by
  // SUMMA implementation:
  dash::SizeSpec<2, extent_t> size_spec(n, n);
  dash::TeamSpec<2, index_t>  team_spec;
  auto pattern = dash::make_pattern<
                 dash::summa_pattern_partitioning_constraints,
                 dash::summa_pattern_mapping_constraints,
                 dash::summa_pattern_layout_constraints >(
                   size_spec,
                   team_spec);
  static_assert(std::is_same<extent_t, decltype(size_spec)::size_type>::value,
                "size type of SizeSpec must be unsigned long long");
  static_assert(std::is_same<index_t, decltype(size_spec)::index_type>::value,
                "index type of SizeSpec must be long long");
  static_assert(std::is_same<index_t, decltype(pattern)::index_type>::value,
                "index type of deduced pattern must be long long");

  dash::Matrix<value_t, 2, index_t> matrix_a(pattern);
  dash::Matrix<value_t, 2, index_t> matrix_b(pattern);
  dash::Matrix<value_t, 2, index_t> matrix_c(pattern);

  std::pair<double, double> t_summa = test_summa(matrix_a,
                                                 matrix_b,
                                                 matrix_c,
                                                 repeat);
  double t_init     = t_summa.first;
  double t_multiply = t_summa.second;

  dash::barrier();

  if (dash::myid() == 0) {
    double gflop      = static_cast<double>(n * n * n * 2) * 1.0e-9 * repeat;
    double s_multiply = 1.0e-6 * t_multiply;
    double s_init     = 1.0e-6 * t_init;
    double gflops     = gflop / s_multiply;
    cout << std::setw(10)
         << num_units
         << ", "
         << std::setw(10)
         << n
         << ", "
         << std::setw(10)
         << (n*n)
         << ", "
         << std::setw(10) << std::fixed << std::setprecision(4)
         << gflop
         << ", "
         << std::setw(10) << std::fixed << std::setprecision(4)
         << gflops
         << ", "
         << std::setw(10)
         << repeat
         << ", "
         << std::setw(11) << std::fixed << std::setprecision(4)
         << s_init
         << ", "
         << std::setw(11) << std::fixed << std::setprecision(4)
         << s_multiply
         << endl;
  }
}

template<typename MatrixType>
void init_values(
  MatrixType & matrix_a,
  MatrixType & matrix_b,
  MatrixType & matrix_c)
{
  // Initialize local sections of matrix C:
  auto unit_id          = dash::myid();
  auto pattern          = matrix_c.pattern();
  auto block_cols       = pattern.blocksize(1);
  auto block_rows       = pattern.blocksize(1);
  auto num_blocks_cols  = pattern.extent(0) / block_cols;
  auto num_blocks_rows  = pattern.extent(1) / block_rows;
  auto num_blocks       = num_blocks_rows * num_blocks_cols;
  auto num_local_blocks = num_blocks / dash::Team::All().size();
  for (auto l_block_idx = 0; l_block_idx < num_local_blocks; ++l_block_idx) {
    auto l_block = matrix_a.local.block(l_block_idx);
    for (auto elem : l_block) {
      elem = 100000 * (unit_id + 1) + l_block_idx;
    }
  }
  // Matrix B is identity matrix:
  index_t diagonal_block_idx = 0;
  for (auto block_row = 0; block_row < num_blocks_rows; ++block_row) {
    auto diagonal_block = matrix_b.block(diagonal_block_idx);
    if (diagonal_block.begin().is_local()) {
      // Diagonal block is local, fill with identity submatrix:
      for (auto diag_idx = 0; diag_idx < pattern.blocksize(0); ++diag_idx) {
         diagonal_block[diag_idx][diag_idx] = 1;
      }
    }
    diagonal_block_idx += (num_blocks_cols + 1);
  }
}

/**
 * Returns pair of durations (init_secs, multiply_secs).
 *
 */
template<typename MatrixType>
std::pair<double, double> test_summa(
  MatrixType & matrix_a,
  MatrixType & matrix_b,
  MatrixType & matrix_c,
  unsigned     repeat)
{
  std::pair<double, double> time;

  auto ts_init_start = Timer::Now();
  init_values(matrix_a, matrix_b, matrix_c);
  time.first = Timer::ElapsedSince(ts_init_start);

  auto ts_multiply_start = Timer::Now();
  for (auto i = 0; i < repeat; ++i) {
    dash::summa(matrix_a, matrix_b, matrix_c);
  }
  time.second = Timer::ElapsedSince(ts_multiply_start);
  return time;
}


