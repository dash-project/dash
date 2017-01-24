#define DASH__MKL_MULTITHREADING

// Undefine for single-precision benchmark:
#define DASH__BENCH_10_SUMMA__DOUBLE_PREC

#ifdef DASH_ENABLE_IPM
#include <mpi.h>
#define DASH__ALGORITHM__COPY__USE_WAIT
#endif

#ifdef DASH_ENABLE_OPENMP
#include <omp.h>
#endif

#ifndef MPI_IMPL_ID
#define MPI_IMPL_ID unknown
#endif

#ifdef DASH_ENABLE_PLASMA
#include <plasma.h>
#endif

#include <libdash.h>
#include <dash/internal/Math.h>

#include <array>
#include <string>
#include <vector>
#include <deque>
#include <utility>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <unistd.h>
#include <type_traits>
#include <cmath>

// Prefer MKL if available:
#if defined(DASH_ENABLE_MKL)
#include <mkl.h>
#include <mkl_types.h>
#include <mkl_cblas.h>
#include <mkl_blas.h>
#include <mkl_lapack.h>
#    if defined(DASH_ENABLE_SCALAPACK)
// #include <mkl_scalapack.h>
#include <mkl_pblas.h>
#include <mkl_blacs.h>
#    endif
// BLAS support:
#elif defined(DASH_ENABLE_BLAS)
extern "C" {
#include <cblas.h>
}
#    if defined(DASH_ENABLE_LAPACK)
// #include <clapack.h>
#    endif
#endif

extern "C" {
  void descinit_(int *idescal,
                 int *m, int *n,
                 int *mb,int *nb,
                 int *dummy1 , int *dummy2,
                 int *icon,
                 int *mla,
                 int *info);
  int numroc_(int *n, int *nb, int *iproc, int *isrcprocs, int *nprocs);
}

using std::cout;
using std::endl;
using std::setw;
using std::string;

// Environment variables as array of strings, terminated by null pointer.
extern char ** environ;

typedef dash::util::Timer<
          dash::util::TimeMeasure::Clock
        > Timer;

#ifdef DASH__BENCH_10_SUMMA__DOUBLE_PREC
typedef double  value_t;
#else
typedef float   value_t;
#endif
typedef dash::default_index_t   index_t;
typedef dash::default_extent_t  extent_t;

typedef std::vector< std::pair< std::string, std::string > >
  env_flags;

typedef struct benchmark_params_t {
  std::string variant;
  extent_t    size_base;
  extent_t    tilesize_base;
  bool        tilesize_fixed;
  extent_t    exp_max;
  unsigned    rep_base;
  unsigned    rep_max;
  extent_t    units_max;
  extent_t    units_x;
  extent_t    units_y;
  extent_t    units_inc;
  extent_t    threads;
  float       cpu_gflops_peak;
  bool        mkl_dyn;
  bool        verify;
} benchmark_params;

template<typename MatrixType>
void init_values(
  MatrixType             & matrix_a,
  MatrixType             & matrix_b,
  MatrixType             & matrix_c,
  const benchmark_params & params);

template<class PatternType>
std::pair<double, double> test_dash(
  extent_t                 n,
  unsigned                 repeat,
  const benchmark_params & params,
  const PatternType      & pattern);

void init_values(
  value_t                * matrix_a,
  value_t                * matrix_b,
  value_t                * matrix_c,
  extent_t                 sb,
  const benchmark_params & params);

std::pair<double, double> test_blas(
  extent_t                 sb,
  unsigned                 repeat,
  const benchmark_params & params);

std::pair<double, double> test_plasma(
  extent_t                 sb,
  unsigned                 repeat,
  const benchmark_params & params,
  extent_t                 tilesize);

std::pair<double, double> test_pblas(
  extent_t                 sb,
  unsigned                 repeat,
  const benchmark_params & params);

void perform_test(
  const std::string      & variant,
  extent_t                 n,
  unsigned                 repeat,
  unsigned                 num_repeats,
  const benchmark_params & params);

benchmark_params parse_args(int argc, char * argv[]);

void print_params(
  const dash::util::BenchmarkParams & bench_cfg,
  const benchmark_params            & params);


int main(int argc, char* argv[])
{
  dash::init(&argc, &argv);

  try {
#ifdef DASH_ENABLE_IPM
    MPI_Pcontrol(0, "off");
#endif

    Timer::Calibrate(0);

    dash::barrier();
    DASH_LOG_DEBUG_VAR("bench.10.summa", getpid());
    dash::barrier();

    auto        params      = parse_args(argc, argv);
    std::string variant     = params.variant;
    extent_t    exp_max     = params.exp_max;
    unsigned    repeats     = params.rep_max;
    unsigned    rep_base    = params.rep_base;

#ifdef DASH_ENABLE_OPENMP
    omp_set_num_threads(params.threads);
#endif

    if (variant == "mkl") {
#ifdef DASH_ENABLE_MKL
      // Require single unit for MKL variant:
      auto nunits = dash::size();
      if (nunits != 1) {
        DASH_THROW(
          dash::exception::RuntimeError,
          "MKL variant of bench.10.summa called with" <<
          "team size " << nunits << " " <<
          "but must be run on a single unit.");
      }
      // Do not set dynamic flag by default as performance is impaired if SMT
      // is not required.
      mkl_set_dynamic(false);
      mkl_set_num_threads(params.threads);
      if (params.mkl_dyn ||
          (mkl_get_max_threads() > 0 &&
           mkl_get_max_threads() < params.threads)) {
        // requested number of threads exceeds number of physical cores, set
        // MKL dynamic flag and retry:
        mkl_set_dynamic(true);
        mkl_set_num_threads(params.threads);
      }
      params.threads = mkl_get_max_threads();
      params.mkl_dyn = mkl_get_dynamic();
#else
      DASH_THROW(dash::exception::RuntimeError, "MKL not enabled");
#endif
    } else {
#ifdef DASH_ENABLE_MKL
      mkl_set_dynamic(false);
      mkl_set_num_threads(params.threads);
      if (params.mkl_dyn ||
          (mkl_get_max_threads() > 0 &&
           mkl_get_max_threads() < params.threads)) {
        // requested number of threads exceeds number of physical cores, set
        // MKL dynamic flag and retry:
        mkl_set_dynamic(true);
        mkl_set_num_threads(params.threads);
      }
      params.threads = mkl_get_max_threads();
      params.mkl_dyn = mkl_get_dynamic();
#endif
    }

    if (variant == "plasma") {
#ifdef DASH_ENABLE_PLASMA
      PLASMA_Init(params.threads);
#else
      DASH_THROW(dash::exception::RuntimeError, "PLASMA not enabled");
#endif
    }

    dash::util::BenchmarkParams bench_params("bench.10.summa");
    bench_params.set_output_width(72);
    bench_params.print_header();
    bench_params.print_pinning();

    print_params(bench_params, params);

    // Run tests, try to balance overall number of gflop in test runs:
    extent_t extent_base = 1;
    for (extent_t exp = 0; exp < exp_max; ++exp) {
      extent_t extent_run = extent_base * params.size_base;
      if (repeats == 0) {
        repeats = 1;
      }

      perform_test(variant, extent_run, exp, repeats, params);

      repeats /= rep_base;
      if      (exp < 1) extent_base += 1;
      else if (exp < 4) extent_base += 2;
      else              extent_base += 4;
    }

#ifdef DASH_ENABLE_PLASMA
    if (variant == "plasma") {
      PLASMA_Finalize();
    }
#endif

  } catch (std::exception & excep) {
    cout << "ERROR: " << excep.what()
         << endl;
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
  auto   myid       = dash::myid();
  auto   num_units  = dash::size();
  auto   variant_id = variant;
  double gflop      = static_cast<double>(n * n * n * 2) * 1.0e-9;

  dash::SizeSpec<2, extent_t> size_spec(n, n);
  auto team_spec = dash::make_team_spec<
                     dash::summa_pattern_partitioning_constraints,
                     dash::summa_pattern_mapping_constraints,
                     dash::summa_pattern_layout_constraints >(
                       size_spec);
  if (params.units_x > 0 && params.units_y > 0) {
    std::array<extent_t, 2> team_extents {{ params.units_y, params.units_x }};
    team_spec.resize(team_extents);
  }
#if 1
  auto pattern   = dash::make_pattern<
                     dash::summa_pattern_partitioning_constraints,
                     dash::summa_pattern_mapping_constraints,
                     dash::summa_pattern_layout_constraints >(
                       size_spec,
                       team_spec);
  typedef decltype(pattern) pattern_t;
  extent_t tilesize = pattern.blocksize(0);
#else
  typedef dash::SeqTilePattern<2, dash::ROW_MAJOR, index_t> pattern_t;
  extent_t tilesize = size_spec.extent(0) / team_spec.size();
  dash::DistributionSpec<2> dist_spec(dash::TILE(tilesize),
                                      dash::TILE(tilesize));
  pattern_t pattern(size_spec, dist_spec, team_spec);
#endif

  if (params.tilesize_base > 0) {
    if (params.tilesize_fixed) {
      tilesize = params.tilesize_base;
    } else{
      tilesize = (n / params.size_base) * params.tilesize_base;
    }
    pattern_t custom_pattern(
                size_spec,
                dash::DistributionSpec<2>(
                  dash::TILE(tilesize),
                  dash::TILE(tilesize)),
                team_spec);
    pattern = custom_pattern;
    // Write back for verification:
    tilesize = pattern.blocksize(0);
  }

  if (myid == 0) {
    if (iteration == 0) {
      cout << "-- Pattern: " << pattern << endl
           << "--" << endl;
      // Print data set column headers:
      cout << std::right
           << setw(7)  << "units"   << ", "
           << setw(7)  << "threads" << ", "
           << setw(6)  << "n"       << ", "
           << setw(12) << "size"    << ", "
           << setw(7)  << "team"    << ", "
           << setw(11) << "tile"    << ", "
           << setw(6)  << "mem.mb"  << ", "
           << setw(10) << "mpi"     << ", "
           << setw(10) << "impl"    << ", "
           << setw(10) << "gflop/r" << ", "
           << setw(7)  << "peak.gf" << ", "
           << setw(7)  << "repeats" << ", "
           << setw(10) << "gflop/s" << ", "
           << setw(11) << "init.s"  << ", "
           << setw(11) << "mmult.s"
           << endl;
    }
    int mem_total_mb = 0;
    if (variant.find("dash") == 0) {
      auto block_s = (n / num_units) * (n / num_units);
      mem_total_mb = ( sizeof(value_t) * (
                         // matrices A, B, C:
                         (3 * n * n) +
                         // four local temporary blocks per unit:
                         (num_units * 4 * block_s)
                       ) / 1024 ) / 1024;
    } else {
      mem_total_mb = ( sizeof(value_t) * (
                         // matrices A, B, C:
                         (3 * n * n)
                       ) / 1024 ) / 1024;
    }

    std::ostringstream team_ss;
    team_ss << team_spec.extent(0) << "x" << team_spec.extent(1);
    std::string team_extents = team_ss.str();
    std::string mpi_impl     = dash__toxstr(MPI_IMPL_ID);

    int gflops_peak = static_cast<int>(params.cpu_gflops_peak *
                                       num_units * params.threads);
    cout << std::right
         << setw(7)  << num_units      << ", "
         << setw(7)  << params.threads << ", "
         << setw(6)  << n              << ", "
         << setw(12) << (n*n)          << ", "
         << setw(7)  << team_extents   << ", "
         << setw(11) << tilesize       << ", "
         << setw(6)  << mem_total_mb   << ", "
         << setw(10) << mpi_impl       << ", "
         << setw(10) << variant_id     << ", "
         << setw(10) << std::fixed << std::setprecision(2)
                     << gflop          << ", "
         << setw(7)  << gflops_peak    << ", "
         << setw(7)  << num_repeats    << ", "
         << std::flush;
  }

  dash::util::TraceStore::on();
  dash::util::TraceStore::clear();

  dash::barrier();

  std::pair<double, double> t_mmult;
  if (variant == "mkl" || variant == "blas") {
    t_mmult = test_blas(n, num_repeats, params);
  } else if (variant == "plasma") {
    t_mmult = test_plasma(n, num_repeats, params, tilesize);
  } else if (variant == "pblas") {
    t_mmult = test_pblas(n, num_repeats, params);
  } else {
    t_mmult = test_dash(n, num_repeats, params, pattern);
  }
  double t_init = t_mmult.first;
  double t_mult = t_mmult.second;

  if (myid == 0) {
    double s_mult = 1.0e-6 * t_mult;
    double s_init = 1.0e-6 * t_init;
    double gflops = (gflop * num_repeats) / s_mult;
    cout << setw(10) << std::fixed << std::setprecision(4) << gflops << ", "
         << setw(11) << std::fixed << std::setprecision(4) << s_init << ", "
         << setw(11) << std::fixed << std::setprecision(4) << s_mult
         << endl;
  }

  dash::barrier();

  dash::util::TraceStore::write(std::cout);
  dash::util::TraceStore::clear();
  dash::util::TraceStore::off();
}

template<typename MatrixType>
void init_values(
  MatrixType             & matrix_a,
  MatrixType             & matrix_b,
  MatrixType             & matrix_c,
  const benchmark_params & params)
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
  for (auto l_block_idx = 0;
       l_block_idx < num_local_blocks;
       ++l_block_idx)
  {
    auto l_block_a = matrix_a.local.block(l_block_idx);
    auto l_block_b = matrix_b.local.block(l_block_idx);
    l_block_elem_a = l_block_a.begin().local();
    l_block_elem_b = l_block_b.begin().local();
    for (auto phase = 0; phase < block_cols * block_rows; ++phase) {
      value_t value = (100000 * (unit_id + 1)) +
                      (100 * l_block_idx) +
                      phase;
      l_block_elem_a[phase] = value;
      if (!params.verify) {
        l_block_elem_b[phase] = value;
      }
    }
  }
  if (params.verify && dash::myid() == 0) {
    // Initialize matrix B as identity matrix to verify A x B = A
    // after the test run:
    for (auto diag = 0; diag < pattern.extent(0); ++diag) {
      matrix_b[diag][diag] = 1;
    }
  }
  dash::barrier();
}

/**
 * Returns pair of durations (init_secs, multiply_secs).
 *
 */
template<class PatternType>
std::pair<double, double> test_dash(
  extent_t                  n,
  unsigned                  repeat,
  const benchmark_params  & params,
  const PatternType       & pattern)
{
  std::pair<double, double> time;

  typedef typename PatternType::size_type  pat_size_t;
  typedef typename PatternType::index_type pat_index_t;

  static_assert(std::is_same<extent_t, pat_size_t>::value,
                "size type of deduced pattern and size spec differ");
  static_assert(std::is_same<index_t,  pat_index_t>::value,
                "index type of deduced pattern and size spec differ");

  DASH_ASSERT_MSG(pattern.extent(0) % dash::size() == 0,
                  "Matrix columns not divisible by number of units");
  DASH_ASSERT_MSG(pattern.extent(1) % dash::size() == 0,
                  "Matrix rows not divisible by number of units");

  dash::Matrix<value_t, 2, index_t, PatternType> matrix_a(pattern);
  dash::Matrix<value_t, 2, index_t, PatternType> matrix_b(pattern);
  dash::Matrix<value_t, 2, index_t, PatternType> matrix_c(pattern);

  dash::barrier();

  auto ts_init_start = Timer::Now();
  init_values(matrix_a, matrix_b, matrix_c, params);
  time.first = Timer::ElapsedSince(ts_init_start);

  dash::barrier();

#ifdef DASH_ENABLE_IPM
  MPI_Pcontrol(0, "on");
  MPI_Pcontrol(0, "clear");
#endif
  auto ts_multiply_start = Timer::Now();
  dash::util::TraceStore::off();
  for (unsigned i = 0; i < repeat; ++i) {
    if (i == 0) {
      dash::util::TraceStore::on();
    }

    dash::summa(matrix_a, matrix_b, matrix_c);

    if (i == 0) {
      dash::util::TraceStore::off();
    }
  }
  time.second = Timer::ElapsedSince(ts_multiply_start);
#ifdef DASH_ENABLE_IPM
  MPI_Pcontrol(0, "off");
#endif

  dash::barrier();

  if (params.verify) {
    auto pattern          = matrix_c.pattern();
    auto block_cols       = pattern.blocksize(0);
    auto block_rows       = pattern.blocksize(1);
    auto num_blocks_cols  = pattern.extent(0) / block_cols;
    auto num_blocks_rows  = pattern.extent(1) / block_rows;
    auto num_blocks       = num_blocks_rows * num_blocks_cols;
    auto num_local_blocks = num_blocks / dash::Team::All().size();
    for (auto l_block_idx = 0;
         l_block_idx < num_local_blocks;
         ++l_block_idx)
    {
      auto l_block_a = matrix_a.local.block(l_block_idx);
      auto l_block_c = matrix_c.local.block(l_block_idx);
      value_t * l_block_a_values = l_block_a.begin().local();
      value_t * l_block_c_values = l_block_c.begin().local();
      for (size_t p = 0; p < l_block_a.size(); ++p) {
        if (l_block_a_values[p] != l_block_c_values[p]) {
          DASH_THROW(
            dash::exception::RuntimeError,
            "Validation failed at unit " << dash::myid() << " " <<
            "in local block " << l_block_idx << ", offset " << p);
        }
      }
    }
  }

  return time;
}

void init_values(
  value_t  * matrix_a,
  value_t  * matrix_b,
  value_t  * matrix_c,
  extent_t   sb,
  const benchmark_params & params)
{
  // Initialize local sections of matrices:
  for (extent_t i = 0; i < sb; ++i) {
    for (extent_t j = 0; j < sb; ++j) {
      value_t value = (100000 * (i % 12)) + (j * 1000) + i;
      matrix_a[i * sb + j] = value;
      matrix_c[i * sb + j] = 0;
      matrix_b[i * sb + j] = value;
    }
  }
}

/**
 * Returns pair of durations (init_secs, multiply_secs).
 *
 */
std::pair<double, double> test_blas(
  extent_t sb,
  unsigned repeat,
  const benchmark_params & params)
{
#if defined(DASH_ENABLE_MKL) || defined(DASH_ENABLE_BLAS)
  std::pair<double, double> time;
  value_t * l_matrix_a;
  value_t * l_matrix_b;
  value_t * l_matrix_c;

  if (dash::size() != 1) {
    time.first  = 0;
    time.second = 0;
    return time;
  }

  // Create local copy of matrices:
#ifdef DASH_ENABLE_MKL
  l_matrix_a = (value_t *)(mkl_malloc(sizeof(value_t) * sb * sb, 64));
  l_matrix_b = (value_t *)(mkl_malloc(sizeof(value_t) * sb * sb, 64));
  l_matrix_c = (value_t *)(mkl_malloc(sizeof(value_t) * sb * sb, 64));
#else
  l_matrix_a = (value_t *)(malloc(sizeof(value_t) * sb * sb));
  l_matrix_b = (value_t *)(malloc(sizeof(value_t) * sb * sb));
  l_matrix_c = (value_t *)(malloc(sizeof(value_t) * sb * sb));
#endif

  auto ts_init_start = Timer::Now();
  init_values(l_matrix_a, l_matrix_b, l_matrix_c, sb, params);
  time.first = Timer::ElapsedSince(ts_init_start);

  auto ts_multiply_start = Timer::Now();
  auto m = sb;
  auto n = sb;
  auto p = sb;
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

#ifdef DASH_ENABLE_MKL
  mkl_free(l_matrix_a);
  mkl_free(l_matrix_b);
  mkl_free(l_matrix_c);
#else
  free(l_matrix_a);
  free(l_matrix_b);
  free(l_matrix_c);
#endif

  return time;
#else
  DASH_THROW(dash::exception::RuntimeError,
             "MKL or BLAS required for BLAS scenario");
#endif
}

/**
 * Returns pair of durations (init_secs, multiply_secs).
 *
 */
std::pair<double, double> test_plasma(
  extent_t                 sb,
  unsigned                 repeat,
  const benchmark_params & params,
  extent_t                 tilesize)
{
#ifdef DASH_ENABLE_PLASMA
  std::pair<double, double> time;
  value_t * l_matrix_a;
  value_t * l_matrix_b;
  value_t * l_matrix_c;

  if (dash::size() != 1) {
    time.first  = 0;
    time.second = 0;
    return time;
  }

  // See PLASMA Users' Guide:
  // http://icl.cs.utk.edu/projectsfiles/plasma/pdf/users_guide20090825.pdf

  // Create local copy of matrices:
  l_matrix_a = (value_t *)(malloc(sizeof(value_t) * sb * sb));
  l_matrix_b = (value_t *)(malloc(sizeof(value_t) * sb * sb));
  l_matrix_c = (value_t *)(malloc(sizeof(value_t) * sb * sb));

  auto ts_init_start = Timer::Now();
  init_values(l_matrix_a, l_matrix_b, l_matrix_c, sb, params);

  // Fill the matrix column major
  for (extent_t i = 0; i < sb; ++i) {
    for (extent_t j = 0; j < sb; ++j) {
      value_t value = (100000 * (i % 12)) + (j * 1000) + i;
      l_matrix_a[i + sb * j] = value;
      l_matrix_b[i + sb * j] = value;
      l_matrix_c[i + sb * j] = 0;
    }
  }
  if (tilesize > 0) {
    PLASMA_Disable(PLASMA_AUTOTUNING);
    PLASMA_Set(PLASMA_TILE_SIZE, tilesize);
  }
  time.first = Timer::ElapsedSince(ts_init_start);

  auto ts_multiply_start = Timer::Now();
  auto m = sb;
  auto n = sb;
  auto p = sb;
  for (auto i = 0; i < repeat; ++i) {
#ifdef DASH__BENCH_10_SUMMA__DOUBLE_PREC
    PLASMA_dgemm(
        PlasmaNoTrans,
        PlasmaNoTrans,
        m,
        n,
        p,
        1.0,
        static_cast<double *>(l_matrix_a),
        p,
        static_cast<double *>(l_matrix_b),
        n,
        0.0,
        static_cast<double *>(l_matrix_c),
        n);
#else
    PLASMA_sgemm(
        PlasmaNoTrans,
        PlasmaNoTrans,
        m,
        n,
        p,
        1.0,
        static_cast<float *>(l_matrix_a),
        p,
        static_cast<float *>(l_matrix_b),
        n,
        0.0,
        static_cast<float *>(l_matrix_c),
        n);
#endif
  }
  time.second = Timer::ElapsedSince(ts_multiply_start);

  free(l_matrix_a);
  free(l_matrix_b);
  free(l_matrix_c);

  return time;
#else
  DASH_THROW(dash::exception::RuntimeError, "PLASMA not enabled");
#endif
}

/**
 * Returns pair of durations (init_secs, multiply_secs).
 *
 * See:
 * http://www-01.ibm.com/support/knowledgecenter/SSNR5K_5.2.0/com.ibm.cluster.pessl.v5r2.pssl100.doc/am6gr_lgemm.htm
 * http://www.trentu.ca/academic/physics/batkinson/scalapack_docs/node11.html
 *
 */
std::pair<double, double> test_pblas(
  extent_t sb,
  unsigned repeat,
  const benchmark_params & params)
{
#if defined(DASH_ENABLE_MKL) && defined(DASH_ENABLE_SCALAPACK)
  typedef MKL_INT int_t;

  int_t            N         = sb;
  int_t            i_one     =  1;
  int_t            i_zero    =  0;
  int_t            i_negone  = -1;
  value_t          d_one     =  1.0E+0;
  value_t          d_zero    =  0.0E+0;
  value_t          d_negone  = -1.0E+0;
  char             storage[] = "R";
  char             trans_a[] = "N";
  char             trans_b[] = "N";
  int              desc_a[12];
  int              desc_b[12];
  int              desc_c[12];
  int              desc_a_distr[12];
  int              desc_b_distr[12];
  int              desc_c_distr[12];
  value_t        * matrix_a_distr;
  value_t        * matrix_b_distr;
  value_t        * matrix_c_distr;

  std::pair<double, double> time;

  int_t ictxt;
  int_t myrow, mycol;
  int_t ierr;
  int_t numproc  = dash::size();
  int_t myid     = dash::myid();

  int_t npcol    = params.units_inc;
  int_t nprow    = numproc / npcol;
  // Block extents such that block size = (nb x mb):
  int_t sbrow    = N / nprow;
  int_t sbcol    = N / npcol;
  // Number of blocks:
  int_t nbrow    = nprow;
  int_t nbcol    = npcol;

  // Number of rows in submatrix C used in the computation, and
  // if transa = 'N', the number of rows in submatrix A.
  int_t m   = N;
  // Number of columns in submatrix C used in the computation, and
  // if transb = 'N', the number of columns in submatrix B
  int_t n   = N;
  // If transa = 'N', the number of columns in submatrix A.
  // If transb = 'N', the number of rows in submatrix B.
  int_t k   = N;
  // Row index of the global matrix A, identifying the first row of the
  // submatrix A. Global scope.
  int_t i_a = 1;
  // Column index of the global matrix A, identifying the first column of
  // the submatrix A. Global scope.
  int_t j_a = 1;
  // Row index of the global matrix B, identifying the first row of the
  // submatrix B. Global scope.
  int_t i_b = 1;
  // Column index of the global matrix B, identifying the first column of
  // the submatrix B. Global scope.
  int_t j_b = 1;
  // Row index of the global matrix C, identifying the first row of the
  // submatrix C. Global scope.
  int_t i_c = 1;
  // Column index of the global matrix C, identifying the first column of
  // the submatrix C. Global scope.
  int_t j_c = 1;
  // Local leading dimensions of global matrices:
  int_t lld_a,       lld_b,       lld_c;
  // Local leading dimensions of distributed submatrices:
  int_t lld_a_distr, lld_b_distr, lld_c_distr;
  // Blocking factors:
  int_t mp;
  int_t kp;
  int_t kq;
  int_t nq;

  auto ts_init_start = Timer::Now();

  // Note:
  // Alternatives to blacs_foo functions: Cblacs_foo

  // Initialize process grid:
  blacs_get_(&i_negone, &i_zero, &ictxt);
  blacs_gridinit_(&ictxt, storage, &nprow, &npcol);
  blacs_gridinfo_(&ictxt,          &nprow, &npcol, &myrow, &mycol);

  // Initialize array descriptors for matrix A, B, C:

  /*
     NUMROC computes the NUMber of Rows Or Columns of a distributed matrix
     owned by the process indicated by IPROC.

     numroc(
       n,        // - Number of rows or columns in the distributed matrix.
       nb,       // - Block size in row or column dimension.
       iproc,    // - Coordinate of the process whose local row or column
                 //   is to be determined.
       isrcproc, // - Coordinate of the process that possesses the first
                 //   row or column in the distributed matrix.
       nprocs    // - Total number of processes over which the matrix is
                 //   distributed.
     )
   */
  mp = numroc_(&m, &sbrow, &myrow, &i_zero, &nprow);
  kp = numroc_(&k, &sbrow, &myrow, &i_zero, &nprow);
  kq = numroc_(&k, &sbcol, &mycol, &i_zero, &npcol);
  nq = numroc_(&n, &sbcol, &mycol, &i_zero, &npcol);
  // Leading dimensions in effect denote the linear storage order such that:
  // A[i][j] := A[j * lda + i]
  // i.e. LD is used to define the distance in memory between elements of
  // two consecutive columns which have the same row index.
  // Local leaading dimensions (lld) refer to the size of a local block
  // instead of the global matrix.
  lld_a_distr = dash::math::max(mp, 1);
  lld_b_distr = dash::math::max(kp, 1);
  lld_c_distr = dash::math::max(mp, 1);
  // Global matrices are considered as distributed with blocking factors
  // (n,m) i.e. there is only one block (the whole matrix) located on
  // process (0,0).
  lld_a = dash::math::max(numroc_(&n, &m, &myrow, &i_zero, &nprow), 1);
  lld_b = dash::math::max(numroc_(&n, &m, &myrow, &i_zero, &nprow), 1);
  lld_c = dash::math::max(numroc_(&n, &m, &myrow, &i_zero, &nprow), 1);

  DASH_LOG_DEBUG(
      "bench.10.summa", "test_pblas",
      "P:",     myid,
      "npcol:", npcol,
      "nprow:", nprow,
      "mycol:", mycol,
      "myrow:", myrow,
      "sbrow:", sbrow,
      "sbcol:", sbcol,
      "lda_d:", lld_a_distr,
      "ldb_d:", lld_b_distr,
      "ldc_d:", lld_c_distr,
      "mp:",    mp,
      "kp:",    kp,
      "kq:",    kq,
      "nq:",    nq);

  // Allocate and initialize local submatrices of A, B, C:
#ifdef DASH_ENABLE_MKL
  matrix_a_distr = (value_t *)(mkl_malloc(mp * nq * sizeof(value_t), 64));
  matrix_b_distr = (value_t *)(mkl_malloc(mp * nq * sizeof(value_t), 64));
  matrix_c_distr = (value_t *)(mkl_malloc(mp * nq * sizeof(value_t), 64));
#else
  matrix_a_distr = (value_t *)(malloc(mp * nq * sizeof(value_t)));
  matrix_b_distr = (value_t *)(malloc(mp * nq * sizeof(value_t)));
  matrix_c_distr = (value_t *)(malloc(mp * nq * sizeof(value_t)));
#endif

  init_values(matrix_a_distr, matrix_b_distr, matrix_c_distr, sbrow, params);

  /*
     descinit(
       &desc,         // - Descriptor to initialize.
       row,           // - Number of rows in the distributed matrix.
       cols,          // - Number of columns in the distributed matrix.
       block_rows,    // - Block size in row dimension.
       block_cols,    // - Block size in column dimension.
       pgrid_row_src, // - Process row over which the first row of the
                      //   matrix is distributed.
       pgrid_col_src, // - Process column over which the first column of
                      //   the matrix is distributed.
       &context,      // - BLACS context handle, indicating the global
                      //   context of the operation on the matrix. The
                      //   context itself is global.
       lld            // - The leading dimension of the local array
                      //   storing the local blocks of the distributed
                      //   matrix.
     );
  */

  // Create descriptors for distributed matrices:
  descinit_(desc_a_distr, &m, &k, &sbrow, &sbcol, &i_zero, &i_zero,
            &ictxt, &lld_a_distr, &ierr);
  descinit_(desc_b_distr, &k, &n, &sbrow, &sbcol, &i_zero, &i_zero,
            &ictxt, &lld_b_distr, &ierr);
  descinit_(desc_c_distr, &m, &n, &sbrow, &sbcol, &i_zero, &i_zero,
            &ictxt, &lld_c_distr, &ierr);

  time.first = Timer::ElapsedSince(ts_init_start);

  auto ts_multiply_start = Timer::Now();
  for (auto i = 0; i < repeat; ++i) {
#ifdef DASH__BENCH_10_SUMMA__DOUBLE_PREC
    pdgemm_(
        trans_a,
        trans_b,
        &m,
        &n,
        &k,
        &d_one,
        static_cast<double *>(matrix_a_distr),
        &i_a,
        &j_a,
        desc_a_distr,
        static_cast<double *>(matrix_b_distr),
        &i_b,
        &j_b,
        desc_b_distr,
        &d_zero,
        static_cast<double *>(matrix_c_distr),
        &i_c,
        &j_c,
        desc_c_distr);
#else
    psgemm_(
        trans_a,
        trans_b,
        &m,
        &n,
        &k,
        &d_one,
        static_cast<float *>(matrix_a_distr),
        &i_a,
        &j_a,
        desc_a_distr,
        static_cast<float *>(matrix_b_distr),
        &i_b,
        &j_b,
        desc_b_distr,
        &d_zero,
        static_cast<float *>(matrix_c_distr),
        &i_c,
        &j_c,
        desc_c_distr);
#endif
  }
  time.second = Timer::ElapsedSince(ts_multiply_start);

  // Exit process grid:
  blacs_gridexit_(&ictxt);

#ifdef DASH_ENABLE_MKL
  mkl_free(matrix_a_distr);
  mkl_free(matrix_b_distr);
  mkl_free(matrix_c_distr);
#else
  free(matrix_a_distr);
  free(matrix_b_distr);
  free(matrix_c_distr);
#endif

  return time;
#else
  DASH_THROW(dash::exception::RuntimeError, "MKL or ScaLAPACK not enabled");
#endif
}

benchmark_params parse_args(int argc, char * argv[])
{
  benchmark_params params;
  params.size_base          = 0;
  params.rep_base           = 2;
  params.rep_max            = 0;
  params.tilesize_base      = 0;
  params.tilesize_fixed     = false;
  params.variant            = "dash";
  params.units_max          = 0;
  params.units_inc          = 0;
  params.units_x            = 0;
  params.units_y            = 0;
  params.threads            = 1;
  params.exp_max            = 4;
  params.cpu_gflops_peak    = 41.4;
  params.mkl_dyn            = false;
  params.verify             = false;

  extent_t size_base        = 0;
  extent_t num_units_inc    = 0;
  extent_t max_units        = 0;
  extent_t remainder        = 0;
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
    } else if (flag == "-nx") {
      params.units_x   = static_cast<extent_t>(atoi(argv[i+1]));
    } else if (flag == "-ny") {
      params.units_y   = static_cast<extent_t>(atoi(argv[i+1]));
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
    } else if (flag == "-cpupeak") {
      params.cpu_gflops_peak = static_cast<float>(atof(argv[i+1]));
    } else if (flag == "-mkldyn") {
      params.mkl_dyn  = atoi(argv[i+1]) == 1;
    } else if (flag == "-verify") {
      params.verify   = atoi(argv[i+1]) == 1;
    } else if (flag == "-tb") {
      params.tilesize_base = static_cast<extent_t>(atoi(argv[i+1]));
    } else if (flag == "-tf") {
      params.tilesize_fixed = !!(atoi(argv[i+1]));
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

  if (params.size_base == 0) {
    DASH_THROW(
      dash::exception::InvalidArgument,
      "Unspecified argument: -sb <size base>");
  }
  if (params.verify) {
    // Only run one repetition for verification runs:
    params.rep_max = 1;
  }

  return params;
}

void print_params(
  const dash::util::BenchmarkParams & conf,
  const benchmark_params            & params)
{
  if (dash::myid() != 0) {
    return;
  }

  conf.print_section_start("Benchmark Configuration");
#ifdef DASH__BENCH_10_SUMMA__DOUBLE_PREC
  conf.print_param("data type",                     "double");
#else
  conf.print_param("data type",                     "float");
#endif
  conf.print_section_end();

  conf.print_section_start("Runtime arguments");
  conf.print_param("-s",      "variant",            params.variant);
  conf.print_param("-sb",     "size base",          params.size_base);
  conf.print_param("-tb",     "tilesize base",      params.tilesize_base);
  conf.print_param("-tf",     "fixed tilesize",     params.tilesize_fixed);
  conf.print_param("-nx",     "team columns",       params.units_x);
  conf.print_param("-ny",     "team rows",          params.units_y);
  conf.print_param("-emax",   "max. iterations",    params.exp_max);
  conf.print_param("-rmax",   "rep. max",           params.rep_max);
  conf.print_param("-rbase",  "rep. base",          params.rep_base);
  conf.print_param("-nt",     "threads/proc",       params.threads);
  conf.print_param("-mkldyn", "MKL dynamic",        params.mkl_dyn);
  conf.print_param("-verify", "run test iteration", params.verify);
  conf.print_param("-ninc",   "units inc.",         params.units_inc);
  conf.print_param("-nmax",   "max. units",         params.units_max);
  conf.print_section_end();
}

