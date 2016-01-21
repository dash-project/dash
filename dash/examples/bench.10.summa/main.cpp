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
  unsigned            repeat,
  unsigned            num_repeats);

typedef struct benchmark_params_t {
  std::string variant;
  extent_t    size_base;
  extent_t    exp_max;
  unsigned    rep_base;
  unsigned    rep_max;
  extent_t    units_max;
  extent_t    units_inc;
  extent_t    threads;
  bool        env_mkl;
  bool        env_mpi_shared_win;
} benchmark_params;

benchmark_params parse_args(int argc, char * argv[]);

void print_params(const benchmark_params & params);


int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);

  Timer::Calibrate(0);

  dash::barrier();
  DASH_LOG_DEBUG_VAR("bench.10.summa", getpid());
  dash::barrier();

  auto        params      = parse_args(argc, argv);
  std::string variant     = params.variant;
  extent_t    exp_max     = params.exp_max;
  extent_t    num_threads = params.threads;
  unsigned    repeats     = params.rep_max;
  unsigned    rep_base    = params.rep_base;

#ifdef DASH_ENABLE_MKL
  // Require single unit for MKL variant:
  if (variant == "mkl" && dash::size() != 1) {
    DASH_THROW(
      dash::exception::RuntimeError,
      "MKL variant of bench.10.summa called with" <<
      "team size " << dash::size() << " " <<
      "but must be run on a single unit.");
  }
  // Do not set dynamic flag by default as performance is impaired if SMT is
  // not required.
  mkl_set_dynamic(false);
  mkl_set_num_threads(num_threads);
  if (mkl_get_max_threads() < num_threads) {
    // requested number of threads exceeds number of physical cores, set MKL
    // dynamic flag and retry:
    mkl_set_dynamic(true);
    mkl_set_num_threads(num_threads);
  }
#endif

  // Run tests, try to balance overall number of gflop in test runs:
  for (auto exp = 0; exp < exp_max; ++exp) {
    extent_t size_run = pow(2, exp) * params.size_base;
    if (repeats == 0) {
      repeats = 1;
    }
    perform_test(variant, size_run, exp, repeats);
    repeats /= rep_base;
  }

  dash::finalize();

  return 0;
}

void perform_test(
  const std::string & variant,
  extent_t            n,
  unsigned            iteration,
  unsigned            num_repeats)
{
  auto num_units   = dash::size();
  auto num_threads = mkl_get_max_threads();

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
  static_assert(std::is_same<extent_t, decltype(pattern)::size_type>::value,
                "size type of deduced pattern and size spec differ");
  static_assert(std::is_same<index_t,  decltype(pattern)::index_type>::value,
                "index type of deduced pattern and size spec differ");

  dash::Matrix<value_t, 2, index_t> matrix_a(pattern);
  dash::Matrix<value_t, 2, index_t> matrix_b(pattern);
  dash::Matrix<value_t, 2, index_t> matrix_c(pattern);

  double gflop = static_cast<double>(n * n * n * 2) * 1.0e-9;
  if (dash::myid() == 0) {
    if (iteration == 0) {
      // Print data set column headers:
      cout << setw(7)  << "units"   << ", "
           << setw(7)  << "threads" << ", "
           << setw(6)  << "n"       << ", "
           << setw(10) << "size"    << ", "
           << setw(5)  << "impl"    << ", "
           << setw(10) << "gflop/r" << ", "
           << setw(7)  << "repeats" << ", "
           << setw(10) << "gflop/s" << ", "
           << setw(11) << "init.s"  << ", "
           << setw(11) << "mmult.s"
           << endl;
    }
    cout << setw(7)  << num_units   << ", "
         << setw(7)  << num_threads << ", "
         << setw(6)  << n           << ", "
         << setw(10) << (n*n)       << ", "
         << setw(5)  << variant     << ", "
         << setw(10) << std::fixed  << std::setprecision(4)
                     << gflop       << ", "
         << setw(7)  << num_repeats << ", "
         << std::flush;
  }

  std::pair<double, double> t_mmult;
  if (variant == "mkl") {
    t_mmult = test_blas(matrix_a,
                        matrix_b,
                        matrix_c,
                        num_repeats);
  } else {
    t_mmult = test_dash(matrix_a,
                        matrix_b,
                        matrix_c,
                        num_repeats);
  }
  double t_init = t_mmult.first;
  double t_mult = t_mmult.second;

  dash::barrier();

  if (dash::myid() == 0) {
    double s_mult = 1.0e-6 * t_mult;
    double s_init = 1.0e-6 * t_init;
    double gflops = (gflop * num_repeats) / s_mult;
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
  auto block_cols       = pattern.blocksize(0);
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
#ifdef DASH_ENABLE_MKL
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
#else
  DASH_THROW(dash::exception::RuntimeError, "MKL not enabled");
#endif
}

benchmark_params parse_args(int argc, char * argv[]) {
  benchmark_params params;
  params.size_base = 0;
  params.rep_base  = 2;
  params.rep_max   = 0;
  params.variant   = "dash";
  params.units_max = 0;
  params.units_inc = 0;
  params.threads   = 1;
#ifdef DASH_ENABLE_MKL
  params.env_mkl   = true;
  params.exp_max   = 7;
#else
  params.env_mkl   = false;
  params.exp_max   = 4;
#endif
#ifdef DART_MPI_DISABLE_SHARED_WINDOWS
  params.env_mpi_shared_win = false;
#else
  params.env_mpi_shared_win = true;
#endif
  extent_t size_base     = 0;
  extent_t num_units_inc = 0;
  extent_t max_units     = 0;
  extent_t remainder     = 0;
  for (auto i = 1; i < argc; i += 2) {
    std::string flag = argv[i];
    if (flag == "-sb") {
      size_base        = static_cast<extent_t>(atoi(argv[i+1]));
    } else if (flag == "-ninc") {
      num_units_inc    = static_cast<extent_t>(atoi(argv[i+1]));
      params.units_inc = num_units_inc;
    } else if (flag == "-nmax") {
      max_units       = static_cast<extent_t>(atoi(argv[i+1]));
      params.units_max = max_units;
    } else if (flag == "-nt") {
      params.threads  = static_cast<extent_t>(atoi(argv[i+1]));
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

void print_params(const benchmark_params & params)
{
  cout << "---------------------------------" << endl
       << "-- DASH benchmark bench.10.summa" << endl
       << "-- parameters:" << endl
       << "-- -s    variant:      " << setw(10) << params.variant   << endl
       << "-- -sb   size base:    " << setw(10) << params.size_base << endl
       << "-- -nmax units max:    " << setw(10) << params.units_max << endl
       << "-- -ninc units inc:    " << setw(10) << params.units_inc << endl
       << "-- -nt   threads/unit: " << setw(10) << params.threads   << endl
       << "-- -emax exp max:      " << setw(10) << params.exp_max   << endl
       << "-- -rmax rep. max:     " << setw(10) << params.rep_max   << endl
       << "-- -rb   rep. base:    " << setw(10) << params.rep_base  << endl
       << "-- environment:" << endl;
  if (params.env_mpi_shared_win) {
    cout << "--   MPI shared windows:  enabled" << endl;
  } else {
    cout << "--   MPI shared windows: disabled" << endl;
  }
  if (params.env_mkl) {
    cout << "--   Intel MKL:           enabled" << endl;
  } else {
    cout << "--   Intel MKL:          disabled" << endl
         << "-- ! MKL not available,"           << endl
         << "-- ! falling back to naive local"  << endl
         << "-- ! matrix multiplication"        << endl
         << endl;
  }
  cout << "---------------------------------"
       << endl;
}
