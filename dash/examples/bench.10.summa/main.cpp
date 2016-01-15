/*
 * Sequential GUPS benchmark for various pattern types.
 *
 */
/* @DASH_HEADER@ */

#include "../bench.h"
#include <libdash.h>

#include <array>
#include <string>
#include <vector>
#include <deque>
#include <utility>
#include <iostream>
#include <iomanip>
#include <unistd.h>
#include <type_traits>
#include <cmath>

#ifdef DASH_ENABLE_MKL
#include <mkl.h>
#include <mkl_types.h>
#include <mkl_cblas.h>
#include <mkl_blas.h>
#include <mkl_lapack.h>
#endif

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
std::pair<double, double> test_dash(
  MatrixType & matrix_a,
  MatrixType & matrix_b,
  MatrixType & matrix_c,
  unsigned     repeat);

template<typename MatrixType>
std::pair<double, double> test_blas(
  MatrixType & matrix_a,
  MatrixType & matrix_b,
  MatrixType & matrix_c,
  unsigned     repeat);

void perform_test(
  const std::string & variant,
  extent_t            n,
  unsigned            repeat);

int main(int argc, char* argv[]) {
#ifndef DASH_ENABLE_MKL
  cout << "WARNING: MKL not available, "
       << "falling back to naive local matrix multiplication"
       << endl;
#endif
  dash::init(&argc, &argv);

  Timer::Calibrate(0);

  std::deque<std::pair<extent_t, unsigned>> tests;

  extent_t size_base  = 12;
  std::string variant = "dash";
  if (argc > 2 && std::string(argv[1]) == "-b") {
    // bench.10.summa -b <size base>
    size_base = static_cast<extent_t>(atoi(argv[2]));
    if (argc > 4 && std::string(argv[3]) == "-s") {
      // bench.10.summa -b <size base> -s <variant>
      variant = argv[4];
    }
  }
  if (argc > 2 && std::string(argv[1]) == "-s") {
    // bench.10.summa -s <variant>
    variant = argv[2];
    if (argc > 4 && std::string(argv[3]) == "-b") {
      // bench.10.summa -s <variant> -b <size base>
      size_base = static_cast<extent_t>(atoi(argv[4]));
    }
  }
  extent_t size_first = pow(size_base, 2);

  tests.push_back({0,      0}); // this prints the header
#ifdef DASH_ENABLE_MKL
  tests.push_back({size_first,      500});
  tests.push_back({size_first *= 2, 100});
  tests.push_back({size_first *= 2,  50});
  tests.push_back({size_first *= 2,  10});
  tests.push_back({size_first *= 2,   5});
  tests.push_back({size_first *= 2,   1});
  tests.push_back({size_first *= 2,   1});
#else
  tests.push_back({64,   100});
  tests.push_back({256,   50});
  tests.push_back({1024,  10});
  tests.push_back({2048,   1});
#endif

  for (auto test: tests) {
    perform_test(variant, test.first, test.second);
  }

  dash::finalize();

  return 0;
}

void perform_test(
  const std::string & variant,
  extent_t            n,
  unsigned            repeat)
{
  auto num_units = dash::size();
  if (n == 0) {
    if (dash::myid() == 0) {
      cout << std::setw(10) << "units"   << ", "
           << std::setw(10) << "n"       << ", "
           << std::setw(10) << "size"    << ", "
           << std::setw(5)  << "impl"    << ", "
           << std::setw(10) << "gflop"   << ", "
           << std::setw(10) << "gflop/s" << ", "
           << std::setw(10) << "repeats" << ", "
           << std::setw(11) << "init.s"  << ", "
           << std::setw(11) << "mmult.s"
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

  std::pair<double, double> t_mmult;
  if (variant == "mkl" || variant == "blas") {
    t_mmult = test_blas(matrix_a,
                        matrix_b,
                        matrix_c,
                        repeat);
  } else {
    t_mmult = test_dash(matrix_a,
                        matrix_b,
                        matrix_c,
                        repeat);
  }
  double t_init     = t_mmult.first;
  double t_multiply = t_mmult.second;

  dash::barrier();

  if (dash::myid() == 0) {
    double gflop      = static_cast<double>(n * n * n * 2) * 1.0e-9 * repeat;
    double s_multiply = 1.0e-6 * t_multiply;
    double s_init     = 1.0e-6 * t_init;
    double gflops     = gflop / s_multiply;
    cout << std::setw(10) << num_units << ", "
         << std::setw(10) << n         << ", "
         << std::setw(10) << (n*n)     << ", "
         << std::setw(5)  << variant   << ", "
         << std::setw(10) << std::fixed << std::setprecision(4)
                          << gflop     << ", "
         << std::setw(10) << std::fixed << std::setprecision(4)
                          << gflops    << ", "
         << std::setw(10) << repeat    << ", "
         << std::setw(11) << std::fixed << std::setprecision(4)
                          << s_init    << ", "
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
  double * l_block_elem_a;
  double * l_block_elem_b;
  for (auto l_block_idx = 0; l_block_idx < num_local_blocks; ++l_block_idx) {
    auto l_block_a = matrix_a.local.block(l_block_idx);
    auto l_block_b = matrix_b.local.block(l_block_idx);
    l_block_elem_a = l_block_a.begin().local();
    l_block_elem_b = l_block_a.begin().local();
    for (auto phase = 0; phase < block_cols * block_rows; ++phase) {
      double value = (100000 * (unit_id + 1)) +
                     (100 * l_block_idx) +
                     phase;
      l_block_elem_a[phase] = value;
      l_block_elem_b[phase] = value;
    }
  }

#if 0
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
#endif
  dash::barrier();
}

/**
 * Returns pair of durations (init_secs, multiply_secs).
 *
 */
template<typename MatrixType>
std::pair<double, double> test_dash(
  MatrixType & matrix_a,
  MatrixType & matrix_b,
  MatrixType & matrix_c,
  unsigned     repeat)
{
  std::pair<double, double> time;

  dash::barrier();

  auto ts_init_start = Timer::Now();
  init_values(matrix_a, matrix_b, matrix_c);
  time.first = Timer::ElapsedSince(ts_init_start);

  dash::barrier();

  auto ts_multiply_start = Timer::Now();
  for (auto i = 0; i < repeat; ++i) {
    dash::summa(matrix_a, matrix_b, matrix_c);
  }
  time.second = Timer::ElapsedSince(ts_multiply_start);

  dash::barrier();

  return time;
}

/**
 * Returns pair of durations (init_secs, multiply_secs).
 *
 */
template<typename MatrixType>
std::pair<double, double> test_blas(
  MatrixType & matrix_a,
  MatrixType & matrix_b,
  MatrixType & matrix_c,
  unsigned     repeat)
{
  std::pair<double, double> time;
  double * l_matrix_a;
  double * l_matrix_b;
  double * l_matrix_c;

  dash::barrier();

  auto ts_init_start = Timer::Now();
  init_values(matrix_a, matrix_b, matrix_c);
  time.first = Timer::ElapsedSince(ts_init_start);

  if (dash::myid() == 0) {
    // Create local copy of matrices:
    l_matrix_a = new double[matrix_a.size()];
    l_matrix_b = new double[matrix_b.size()];
    l_matrix_c = new double[matrix_c.size()];
    dash::copy(matrix_a.begin(), matrix_a.end(), l_matrix_a);
    dash::copy(matrix_b.begin(), matrix_b.end(), l_matrix_b);
    dash::copy(matrix_c.begin(), matrix_c.end(), l_matrix_c);
  }

  dash::barrier();

  auto ts_multiply_start = Timer::Now();
  if (dash::myid() == 0) {
    auto m = matrix_a.extent(0);
    auto n = matrix_a.extent(1);
    auto p = matrix_b.extent(0);
    mkl_set_num_threads(dash::Team::All().size());

    ts_multiply_start = Timer::Now();
    for (auto i = 0; i < repeat; ++i) {
      cblas_dgemm(CblasRowMajor,
                  CblasNoTrans,
                  CblasNoTrans,
                  m,
                  n,
                  p,
                  1.0,
                  static_cast<const double *>(l_matrix_a),
                  p,
                  static_cast<const double *>(l_matrix_b),
                  n,
                  0.0,
                  static_cast<double *>(l_matrix_c),
                  n);
    }
  }
  time.second = Timer::ElapsedSince(ts_multiply_start);

  dash::barrier();

  if (dash::myid() == 0) {
    delete[] l_matrix_a;
    delete[] l_matrix_b;
    delete[] l_matrix_c;
  }

  return time;
}
