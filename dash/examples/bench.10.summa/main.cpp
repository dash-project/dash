/*
 * Sequential GUPS benchmark for various pattern types.
 *
 */
/* @DASH_HEADER@ */

//#define DASH__MKL_MULTITHREADING
#define DASH__BENCH_10_SUMMA__DOUBLE_PREC
#define DASH__ALGORITHM__COPY__USE_WAIT

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

#ifdef DASH__BENCH_10_SUMMA__DOUBLE_PREC
typedef double    value_t;
#else
typedef float     value_t;
#endif
typedef int64_t   index_t;
typedef uint64_t  extent_t;

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
  bool        mkl_dyn;
} benchmark_params;


template<typename MatrixType>
void init_values(
  MatrixType & matrix_a,
  MatrixType & matrix_b,
  MatrixType & matrix_c);

std::pair<double, double> test_dash(
  extent_t sb,
  unsigned repeat);

void init_values(
  value_t  * matrix_a,
  value_t  * matrix_b,
  value_t  * matrix_c,
  extent_t   sb);

std::pair<double, double> test_blas(
  extent_t sb,
  unsigned repeat);

void perform_test(
  const std::string      & variant,
  extent_t                 n,
  unsigned                 repeat,
  unsigned                 num_repeats,
  const benchmark_params & params);

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
  unsigned    repeats     = params.rep_max;
  unsigned    rep_base    = params.rep_base;

#ifdef DASH_ENABLE_MKL
  if (variant == "mkl") {
    // Require single unit for MKL variant:
    if (dash::size() != 1) {
      DASH_THROW(
        dash::exception::RuntimeError,
        "MKL variant of bench.10.summa called with" <<
        "team size " << dash::size() << " " <<
        "but must be run on a single unit.");
    }
  }
  // Do not set dynamic flag by default as performance is impaired if SMT is
  // not required.
  mkl_set_dynamic(false);
  mkl_set_num_threads(params.threads);
  if (params.mkl_dyn || mkl_get_max_threads() < params.threads) {
    // requested number of threads exceeds number of physical cores, set MKL
    // dynamic flag and retry:
    mkl_set_dynamic(true);
    mkl_set_num_threads(params.threads);
  }
  params.threads = mkl_get_max_threads();
  params.mkl_dyn = mkl_get_dynamic();
#else
  if (variant == "mkl") {
    DASH_THROW(dash::exception::RuntimeError, "MKL not enabled");
  }
#endif

  if (dash::myid() == 0) {
    print_params(params);
  }

  // Run tests, try to balance overall number of gflop in test runs:
  for (auto exp = 0; exp < exp_max; ++exp) {
    extent_t size_run = pow(2, exp) * params.size_base;
    if (repeats == 0) {
      repeats = 1;
    }
    perform_test(variant, size_run, exp, repeats, params);
    repeats /= rep_base;
  }

  dash::finalize();

  return 0;
}

void perform_test(
  const std::string      & variant,
  extent_t                 n,
  unsigned                 iteration,
  unsigned                 num_repeats,
  const benchmark_params & params)
{
  auto num_units   = dash::size();
  auto num_threads = params.threads;

  double gflop = static_cast<double>(n * n * n * 2) * 1.0e-9;
  if (dash::myid() == 0) {
    if (iteration == 0) {
      // Print data set column headers:
      cout << setw(7)  << "units"   << ", "
           << setw(7)  << "threads" << ", "
           << setw(6)  << "n"       << ", "
           << setw(10) << "size"    << ", "
           << setw(6)  << "mem.mb"  << ", "
           << setw(5)  << "impl"    << ", "
           << setw(12) << "gflop/r" << ", "
           << setw(7)  << "repeats" << ", "
           << setw(10) << "gflop/s" << ", "
           << setw(11) << "init.s"  << ", "
           << setw(11) << "mmult.s"
           << endl;
    }
    int  mem_local_mb;
    if (variant == "dash") {
      auto block_s = (n / num_units) * (n / num_units);
      mem_local_mb = ( sizeof(value_t) * (
                         // matrices A, B, C:
                         (3 * n * n / num_units) +
                         // four local temporary blocks per unit:
                         (num_units * 4 * block_s)
                       ) / 1024 ) / 1024;
    } else if (variant == "mkl") {
      mem_local_mb = ( sizeof(value_t) * (
                         // matrices A, B, C:
                         (3 * n * n)
                       ) / 1024 ) / 1024;
    }
    cout << setw(7)  << num_units      << ", "
         << setw(7)  << params.threads << ", "
         << setw(6)  << n              << ", "
         << setw(10) << (n*n)          << ", "
         << setw(6)  << mem_local_mb   << ", "
         << setw(5)  << variant        << ", "
         << setw(12) << std::fixed     << std::setprecision(4)
                     << gflop          << ", "
         << setw(7)  << num_repeats    << ", "
         << std::flush;
  }

  std::pair<double, double> t_mmult;
  if (variant == "mkl") {
    t_mmult = test_blas(n, num_repeats);
  } else {
    t_mmult = test_dash(n, num_repeats);
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
  value_t * l_block_elem_a;
  value_t * l_block_elem_b;
  for (auto l_block_idx = 0; l_block_idx < num_local_blocks; ++l_block_idx) {
    auto l_block_a = matrix_a.local.block(l_block_idx);
    auto l_block_b = matrix_b.local.block(l_block_idx);
    l_block_elem_a = l_block_a.begin().local();
    l_block_elem_b = l_block_a.begin().local();
    for (auto phase = 0; phase < block_cols * block_rows; ++phase) {
      value_t value = (100000 * (unit_id + 1)) +
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
std::pair<double, double> test_dash(
  extent_t n,
  unsigned repeat)
{
  std::pair<double, double> time;

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

void init_values(
  value_t  * matrix_a,
  value_t  * matrix_b,
  value_t  * matrix_c,
  extent_t   sb)
{
  // Initialize local sections of matrix C:
  for (int i = 0; i < sb; ++i) {
    for (int j = 0; j < sb; ++j) {
      value_t value = (100000 * (i % 12)) + (j * 1000) + i;
      matrix_a[i * sb + j] = value;
      matrix_b[i * sb + j] = value;
      matrix_c[i * sb + j] = 0;
    }
  }
}

/**
 * Returns pair of durations (init_secs, multiply_secs).
 *
 */
std::pair<double, double> test_blas(
  extent_t sb,
  unsigned repeat)
{
#ifdef DASH_ENABLE_MKL
  std::pair<double, double> time;
  value_t * l_matrix_a;
  value_t * l_matrix_b;
  value_t * l_matrix_c;

  if (dash::size() != 1) {
    time.first  = 0;
    time.second = 0;
    return time;
  }
  long long N = sb;

  // Create local copy of matrices:
  l_matrix_a = (value_t *)(mkl_malloc(sizeof(value_t) * N * N, 64));
  l_matrix_b = (value_t *)(mkl_malloc(sizeof(value_t) * N * N, 64));
  l_matrix_c = (value_t *)(mkl_malloc(sizeof(value_t) * N * N, 64));

  auto ts_init_start = Timer::Now();
  init_values(l_matrix_a, l_matrix_b, l_matrix_c, sb);
  time.first = Timer::ElapsedSince(ts_init_start);

  auto ts_multiply_start = Timer::Now();
  auto m = sb;
  auto n = sb;
  auto p = sb;

  ts_multiply_start = Timer::Now();
  for (auto i = 0; i < repeat; ++i) {
#ifdef DASH__BENCH_10_SUMMA__DOUBLE_PREC
    cblas_dgemm(
        CblasRowMajor,
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
#else
    cblas_sgemm(
        CblasRowMajor,
        CblasNoTrans,
        CblasNoTrans,
        m,
        n,
        p,
        1.0,
        static_cast<const float *>(l_matrix_a),
        p,
        static_cast<const float *>(l_matrix_b),
        n,
        0.0,
        static_cast<float *>(l_matrix_c),
        n);
#endif
  }
  time.second = Timer::ElapsedSince(ts_multiply_start);

  mkl_free(l_matrix_a);
  mkl_free(l_matrix_b);
  mkl_free(l_matrix_c);

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
  params.mkl_dyn   = false;
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
    } else if (flag == "-mkldyn") {
      params.mkl_dyn  = true;
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
#ifdef DASH__BENCH_10_SUMMA__DOUBLE_PREC
       << "-- data type:            " << setw(8) << "double" << endl
#else
       << "-- data type:            " << setw(8) << "float"  << endl
#endif
       << "-- parameters:" << endl
       << "--   -s    variant:      " << setw(8) << params.variant   << endl
       << "--   -sb   size base:    " << setw(8) << params.size_base << endl
       << "--   -nmax units max:    " << setw(8) << params.units_max << endl
       << "--   -ninc units inc:    " << setw(8) << params.units_inc << endl
       << "--   -nt   threads/unit: " << setw(8) << params.threads   << endl
       << "--   -emax exp max:      " << setw(8) << params.exp_max   << endl
       << "--   -rmax rep. max:     " << setw(8) << params.rep_max   << endl
       << "--   -rb   rep. base:    " << setw(8) << params.rep_base  << endl
       << "-- environment:" << endl;
#ifdef MPI_IMPL_ID
  cout << "--   MPI implementation: "
       << setw(8) << dash__toxstr(MPI_IMPL_ID) << endl;
#endif
  cout << "--   MPI shared windows: ";
  if (params.env_mpi_shared_win) {
    cout << " enabled" << endl;
  } else {
    cout << "disabled" << endl;
  }
  cout << "--   Intel MKL:          ";
  if (params.env_mkl) {
    cout << " enabled" << endl;
    cout << "--   MKL dynamic:        ";
    if (params.mkl_dyn) {
      cout << " enabled" << endl;
    } else {
      cout << "disabled" << endl;
    }
  } else {
    cout << "disabled" << endl
         << "-- ! MKL not available,"           << endl
         << "-- ! falling back to naive local"  << endl
         << "-- ! matrix multiplication"        << endl
         << endl;
  }
  cout << "---------------------------------"
       << endl;
}
