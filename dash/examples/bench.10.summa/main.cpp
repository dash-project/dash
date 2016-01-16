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
using std::setw;

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

typedef struct benchmark_params_t {
  std::string variant;
  extent_t    size_base;
  extent_t    exp_max;
  unsigned    rep_base;
  unsigned    rep_max;
  extent_t    units_max;
  extent_t    units_inc;
} benchmark_params;

benchmark_params parse_args(int argc, char * argv[]);


int main(int argc, char* argv[]) {
#ifdef DASH_ENABLE_MKL
  const bool mkl_available = true;
#else
  const bool mkl_available = false;
#endif

  dash::init(&argc, &argv);

  Timer::Calibrate(0);

  auto params = parse_args(argc, argv);
  if (dash::myid() == 0) {
    cout << "--------------------------------" << endl
         << "-- DASH benchmark bench.10.summa" << endl
         << "-- parameters:" << endl
         << "--   -s    variant:   " << setw(5) << params.variant   << endl
         << "--   -sb   size base: " << setw(5) << params.size_base << endl
         << "--   -nmax units max: " << setw(5) << params.units_max << endl
         << "--   -ninc units inc: " << setw(5) << params.units_inc << endl
         << "--   -emax exp max:   " << setw(5) << params.exp_max   << endl
         << "--   -rmax rep. max:  " << setw(5) << params.rep_max   << endl
         << "--   -rb   rep. base: " << setw(5) << params.rep_base  << endl
         << "-- environment:" << endl;
    if (mkl_available) {
      cout << "--   BLAS: found" << endl;
    } else {
      cout << "--   BLAS: not found" << endl
           << "-- ! WARNING:"                      << endl
           << "-- !   MKL not available,"          << endl
           << "-- !   falling back to naive local" << endl
           << "-- !   matrix multiplication"       << endl
           << endl;
    }
    cout << "--------------------------------"
         << endl;
  }
  std::string variant = params.variant;
  extent_t size_first = params.size_base * 4;
  extent_t exp_max    = params.exp_max;
  unsigned repeats    = params.rep_max;
  unsigned rep_base   = params.rep_base;

  std::deque<std::pair<extent_t, unsigned>> tests;
  // Prints the header
  tests.push_back({0, 0});

  // Balance overall number of gflop in test runs:
  for (auto exp = 0; exp < exp_max; ++exp) {
    extent_t size_run = pow(2, exp) * size_first;
    if (repeats == 0) {
      repeats = 1;
    }
    tests.push_back({ size_run, repeats });
    repeats /= rep_base;
  }

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
      cout << setw(7)  << "units"   << ", "
           << setw(10) << "n"       << ", "
           << setw(10) << "size"    << ", "
           << setw(5)  << "impl"    << ", "
           << setw(10) << "gflop/r" << ", "
           << setw(10) << "repeats" << ", "
           << setw(10) << "gflop/s" << ", "
           << setw(11) << "init.s"  << ", "
           << setw(11) << "mmult.s"
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

  double gflop = static_cast<double>(n * n * n * 2) * 1.0e-9;
  if (dash::myid() == 0) {
    cout << setw(7)  << num_units << ", "
         << setw(10) << n         << ", "
         << setw(10) << (n*n)     << ", "
         << setw(5)  << variant   << ", "
         << setw(10) << std::fixed << std::setprecision(4)
                     << gflop     << ", "
         << setw(10) << repeat    << ", "
         << std::flush;
  }

  std::pair<double, double> t_mmult;
  if (variant == "mkl" || variant == "blas") {
#ifdef DASH_ENABLE_MKL
    t_mmult = test_blas(matrix_a,
                        matrix_b,
                        matrix_c,
                        repeat);
#endif
  } else {
    t_mmult = test_dash(matrix_a,
                        matrix_b,
                        matrix_c,
                        repeat);
  }
  double t_init = t_mmult.first;
  double t_mult = t_mmult.second;

  dash::barrier();

  if (dash::myid() == 0) {
    double s_mult = 1.0e-6 * t_mult;
    double s_init = 1.0e-6 * t_init;
    double gflops = (gflop * repeat) / s_mult;
    cout << setw(10) << std::fixed << std::setprecision(4) << gflops << ", "
         << setw(11) << std::fixed << std::setprecision(4) << s_init << ", "
         << setw(11) << std::fixed << std::setprecision(4) << s_mult
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

#ifdef DASH_ENABLE_MKL
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
#endif

benchmark_params parse_args(int argc, char * argv[]) {
  benchmark_params params;
  params.size_base = 0;
  params.rep_base  = 2;
  params.rep_max   = 0;
  params.variant   = "dash";
  params.units_max = 0;
  params.units_inc = 0;
#ifdef DASH_ENABLE_MKL
  params.exp_max   = 7;
#else
  params.exp_max   = 4;
#endif
  extent_t size_base     = 0;
  extent_t num_units_inc = 0;
  extent_t max_units     = 0;
  extent_t remainder     = 0;
  for (auto i = 1; i < argc; i += 2) {
    std::string flag = argv[i];
    if (flag == "-sb") {
      size_base       = static_cast<extent_t>(atoi(argv[i+1]));
    } else if (flag == "-ninc") {
      num_units_inc    = static_cast<extent_t>(atoi(argv[i+1]));
      params.units_inc = num_units_inc;
    } else if (flag == "-nmax") {
      max_units       = static_cast<extent_t>(atoi(argv[i+1]));
      params.units_max = max_units;
    } else if (flag == "-s") {
      params.variant  = argv[i+1];
    } else if (flag == "-emax") {
      params.exp_max  = static_cast<extent_t>(atoi(argv[i+1]));
    } else if (flag == "-rb") {
      params.rep_base = static_cast<unsigned>(atoi(argv[i+1]));
    } else if (flag == "-rmax") {
      params.rep_max  = static_cast<unsigned>(atoi(argv[i+1]));
    }
  }
  if (size_base == 0 && max_units > 0 && num_units_inc > 0) {
    size_base = num_units_inc;
    // Simple integer factorization by trial division:
    for (remainder = max_units;
         remainder > num_units_inc;
         remainder -= num_units_inc) {
      extent_t r       = remainder;
      extent_t z       = 2;
      extent_t z_last  = 1;
      while (z * z <= r) {
        if (r % z == 0) {
          if (z != z_last && size_base % z != 0) {
            size_base *= z;
          }
          r      /= z;
          z_last  = z;
        } else {
          z++;
        }
      }
      if (r > 1 && size_base % r != 0) {
        size_base *= r;
      }
    }
    if (params.rep_max == 0) {
      params.rep_max = pow(params.rep_base, params.exp_max - 1);
    }
  }
  params.size_base = size_base;
  return params;
}

