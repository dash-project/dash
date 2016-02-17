/*
 * Sequential GUPS benchmark for various pattern types.
 *
 */
/* @DASH_HEADER@ */

#define DASH__MKL_MULTITHREADING
#define DASH__BENCH_10_SUMMA__DOUBLE_PREC

// #define DASH_ALGORITHM_SUMMA_DIAGONAL_MAPPING

#include "../bench.h"
#include <libdash.h>
#include <dash/internal/Math.h>

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
#ifdef DASH_ENABLE_SCALAPACK
// #include <mkl_scalapack.h>
#include <mkl_pblas.h>
#include <mkl_blacs.h>
#endif
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

typedef struct {
  int  rank;
  char host[100];
  int  cpu;
  int  numa_node;
} unit_pin_info;

std::ostream & operator<<(
  std::ostream        & os,
  const unit_pin_info & upi)
{
  std::ostringstream ss;
  ss << "unit_pin_info("
     << "rank:" << upi.rank << " "
     << "host:" << upi.host << " "
     << "cpu:"  << upi.cpu  << " "
     << "numa:" << upi.numa_node << ")";
  return operator<<(os, ss.str());
}

#ifdef DASH__BENCH_10_SUMMA__DOUBLE_PREC
typedef double    value_t;
#else
typedef float     value_t;
#endif
typedef int64_t   index_t;
typedef uint64_t  extent_t;

typedef std::vector< std::pair< std::string, std::string > >
  env_flags;

typedef struct benchmark_params_t {
  env_flags   env_config;
  std::string variant;
  extent_t    size_base;
  extent_t    exp_max;
  unsigned    rep_base;
  unsigned    rep_max;
  extent_t    units_max;
  extent_t    units_x;
  extent_t    units_y;
  extent_t    units_inc;
  extent_t    threads;
  bool        env_mkl;
  bool        env_scalapack;
  bool        env_mpi_shared_win;
  bool        mkl_dyn;
  float       cpu_gflops_peak;
} benchmark_params;

template<typename MatrixType>
void init_values(
  MatrixType & matrix_a,
  MatrixType & matrix_b,
  MatrixType & matrix_c);

std::pair<double, double> test_dash(
  extent_t sb,
  unsigned repeat,
  const benchmark_params & params);

void init_values(
  value_t  * matrix_a,
  value_t  * matrix_b,
  value_t  * matrix_c,
  extent_t   sb);

std::pair<double, double> test_blas(
  extent_t sb,
  unsigned repeat,
  const benchmark_params & params);

std::pair<double, double> test_pblas(
  extent_t sb,
  unsigned repeat,
  const benchmark_params & params);

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

  int myid   = dash::myid();

  Timer::Calibrate(0);

  dash::barrier();
  DASH_LOG_DEBUG_VAR("bench.10.summa", getpid());
  dash::barrier();

  // Collect process pinning information:
  //
  dash::Array<unit_pin_info> unit_pinning(dash::size());
  unit_pin_info my_pin_info;
  my_pin_info.rank      = dash::myid();
  my_pin_info.cpu       = dash::util::Locality::UnitCPU();
  my_pin_info.numa_node = dash::util::Locality::UnitNUMANode();
  gethostname(my_pin_info.host, 100);

  unit_pinning[dash::myid()] = my_pin_info;

  dash::barrier();

  auto        params      = parse_args(argc, argv);
  std::string variant     = params.variant;
  extent_t    exp_max     = params.exp_max;
  unsigned    repeats     = params.rep_max;
  unsigned    rep_base    = params.rep_base;

#ifdef DASH_ENABLE_MKL
  if (variant == "mkl") {
    // Require single unit for MKL variant:
    auto nunits = dash::size();
    if (nunits != 1) {
      DASH_THROW(
        dash::exception::RuntimeError,
        "MKL variant of bench.10.summa called with" <<
        "team size " << nunits << " " <<
        "but must be run on a single unit.");
    }
  }
  // Do not set dynamic flag by default as performance is impaired if SMT
  // is not required.
  mkl_set_dynamic(false);
  mkl_set_num_threads(params.threads);
  if (params.mkl_dyn || mkl_get_max_threads() < params.threads) {
    // requested number of threads exceeds number of physical cores, set
    // MKL dynamic flag and retry:
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

  if (params.units_x == 0 && params.units_y == 0) {
    params.units_x = dash::size();
    params.units_y = 1;
#ifndef DASH_ALGORITHM_SUMMA_DIAGONAL_MAPPING
    // Minimize surface-to-volume in team spec:
    while (params.units_inc > 1 &&
           params.units_x % params.units_inc == 0 &&
           params.units_x > 2 * params.units_inc)
    {
      params.units_y *= params.units_inc;
      params.units_x /= params.units_inc;
    }
    while (params.units_x % 2 == 0 &&
           params.units_x > 2 * params.units_y)
    {
      params.units_y *= 2;
      params.units_x /= 2;
    }
#endif
  }

  dash::barrier();

  if (myid == 0) {
    print_params(params);

    cout << std::left     << "-- "
         << std::setw(5)  << "unit"
         << std::setw(32) << "host"
         << std::setw(10) << "numa node"
         << std::setw(5)  << "cpu"
         << endl;
    for (size_t unit = 0; unit < dash::size(); ++unit) {
      unit_pin_info pin_info = unit_pinning[unit];
      cout << std::left     << "-- "
           << std::setw(5)  << pin_info.rank
           << std::setw(32) << pin_info.host
           << std::setw(10) << pin_info.numa_node
           << std::setw(5)  << pin_info.cpu
           << endl;
    }
  }

  // Run tests, try to balance overall number of gflop in test runs:
  extent_t extent_base = 1;
  for (extent_t exp = 0; exp < exp_max; ++exp) {
    extent_t extent_run  = extent_base * params.size_base;
    if (repeats == 0) {
      repeats = 1;
    }

    perform_test(variant, extent_run, exp, repeats, params);

    repeats /= rep_base;
    if      (exp < 1) extent_base += 1;
    else if (exp < 4) extent_base += 2;
    else              extent_base += 4;
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

  if (myid == 0) {
    if (iteration == 0) {
      // Print data set column headers:
      cout << std::right
           << setw(7)  << "units"   << ", "
           << setw(7)  << "threads" << ", "
           << setw(6)  << "n"       << ", "
           << setw(12) << "size"    << ", "
           << setw(7)  << "team"    << ", "
           << setw(6)  << "mem.mb"  << ", "
           << setw(10) << "mpi"     << ", "
           << setw(10) << "impl"    << ", "
           << setw(12) << "gflop/r" << ", "
           << setw(7)  << "peak.gf" << ", "
           << setw(7)  << "repeats" << ", "
           << setw(10) << "gflop/s" << ", "
           << setw(11) << "init.s"  << ", "
           << setw(11) << "mmult.s"
           << endl;
    }
    int mem_total_mb = 0;
    if (variant.find("dash") == 0) {
#ifdef DASH_ALGORITHM_SUMMA_DIAGONAL_MAPPING
      variant_id.append(".dm");
#else
      variant_id.append(".mp");
#endif
      auto block_s = (n / num_units) * (n / num_units);
      mem_total_mb = ( sizeof(value_t) * (
                         // matrices A, B, C:
                         (3 * n * n) +
                         // four local temporary blocks per unit:
                         (num_units * 4 * block_s)
                       ) / 1024 ) / 1024;
    } else if (variant.find("mkl") == 0 || variant.find("blas") == 0) {
      mem_total_mb = ( sizeof(value_t) * (
                         // matrices A, B, C:
                         (3 * n * n)
                       ) / 1024 ) / 1024;
    } else if (variant.find("pblas") == 0) {
      mem_total_mb = ( sizeof(value_t) * (
                         // matrices A, B, C:
                         (3 * n * n)
                       ) / 1024 ) / 1024;
    }

    std::ostringstream team_ss;
    team_ss << params.units_x << "x" << params.units_y;
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
         << setw(6)  << mem_total_mb   << ", "
         << setw(10) << mpi_impl       << ", "
         << setw(10) << variant_id     << ", "
         << setw(12) << std::fixed << std::setprecision(4)
                     << gflop          << ", "
         << setw(7)  << gflops_peak    << ", "
         << setw(7)  << num_repeats    << ", "
         << std::flush;
  }

  std::pair<double, double> t_mmult;
  if (variant == "mkl" || variant == "blas") {
    t_mmult = test_blas(n, num_repeats, params);
  } else if (variant == "pblas") {
    t_mmult = test_pblas(n, num_repeats, params);
  } else {
    t_mmult = test_dash(n, num_repeats, params);
  }
  double t_init = t_mmult.first;
  double t_mult = t_mmult.second;

  dash::barrier();

  if (myid == 0) {
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
  unsigned repeat,
  const benchmark_params & params)
{
  std::pair<double, double> time;

  // Automatically deduce pattern type satisfying constraints defined by
  // SUMMA implementation:
  dash::SizeSpec<2, extent_t> size_spec(n, n);
  dash::TeamSpec<2, index_t>  team_spec(params.units_x, params.units_y);

  auto pattern = dash::make_pattern<
                   dash::summa_pattern_partitioning_constraints,
                   dash::summa_pattern_mapping_constraints,
                   dash::summa_pattern_layout_constraints >(
                     size_spec,
                     team_spec);

  static_assert(std::is_same<extent_t,
                             decltype(pattern)::size_type>::value,
                "size type of deduced pattern and size spec differ");
  static_assert(std::is_same<index_t,
                             decltype(pattern)::index_type>::value,
                "index type of deduced pattern and size spec differ");

  DASH_ASSERT_MSG(pattern.extent(0) % dash::size() == 0,
                  "Matrix columns not divisible by number of units");
  DASH_ASSERT_MSG(pattern.extent(1) % dash::size() == 0,
                  "Matrix rows not divisible by number of units");

  dash::Matrix<value_t, 2, index_t, decltype(pattern)> matrix_a(pattern);
  dash::Matrix<value_t, 2, index_t, decltype(pattern)> matrix_b(pattern);
  dash::Matrix<value_t, 2, index_t, decltype(pattern)> matrix_c(pattern);

  dash::barrier();

  auto ts_init_start = Timer::Now();
  init_values(matrix_a, matrix_b, matrix_c);
  time.first = Timer::ElapsedSince(ts_init_start);

  dash::barrier();

  auto ts_multiply_start = Timer::Now();
  for (unsigned i = 0; i < repeat; ++i) {
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
  // Initialize local sections of matrices:
  for (extent_t i = 0; i < sb; ++i) {
    for (extent_t j = 0; j < sb; ++j) {
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
  unsigned repeat,
  const benchmark_params & params)
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
  const value_t    d_one     =  1.0E+0;
  const value_t    d_zero    =  0.0E+0;
  const value_t    d_negone  = -1.0E+0;
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
  matrix_a_distr = (value_t *)(mkl_malloc(mp * nq * sizeof(value_t), 64));
  matrix_b_distr = (value_t *)(mkl_malloc(mp * nq * sizeof(value_t), 64));
  matrix_c_distr = (value_t *)(mkl_malloc(mp * nq * sizeof(value_t), 64));

  init_values(matrix_a_distr, matrix_b_distr, matrix_c_distr, sbrow);

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
        static_cast<const double *>(matrix_a_distr),
        &i_a,
        &j_a,
        desc_a_distr,
        static_cast<const double *>(matrix_b_distr),
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
        static_cast<const float *>(matrix_a_distr),
        &i_a,
        &j_a,
        desc_a_distr,
        static_cast<const float *>(matrix_b_distr),
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

  mkl_free(matrix_a_distr);
  mkl_free(matrix_b_distr);
  mkl_free(matrix_c_distr);

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
  params.variant            = "dash";
  params.units_max          = 0;
  params.units_inc          = 0;
  params.units_x            = 0;
  params.units_y            = 0;
  params.threads            = 1;
  params.exp_max            = 4;
  params.mkl_dyn            = false;
  params.env_mpi_shared_win = true;
  params.env_mkl            = false;
  params.env_scalapack      = false;
  params.cpu_gflops_peak    = 41.4;
#ifdef DASH_ENABLE_MKL
  params.env_mkl            = true;
  params.exp_max            = 7;
#endif
#ifdef DASH_ENABLE_SCALAPACK
  params.env_scalapack      = true;
#endif
#ifdef DART_MPI_DISABLE_SHARED_WINDOWS
  params.env_mpi_shared_win = false;
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
    } else if (flag == "-mkldyn") {
      params.mkl_dyn  = true;
    } else if (flag == "-cpupeak") {
      params.cpu_gflops_peak = static_cast<float>(atof(argv[i+1]));
    } else if (flag == "-envcfg") {
      std::string flags_str = argv[i+1];
      // Split string into vector of key-value pairs
      const char delim    = ':';
      string::size_type i = 0;
      string::size_type j = flags_str.find(delim);
      while (j != string::npos) {
        string flag_str = flags_str.substr(i, j-i);
        // Split into key and value:
        string::size_type fi = flag_str.find('=');
        string::size_type fj = flag_str.find('=', fi);
        if (fj == string::npos) {
          fj = flag_str.length();
        }
        string flag_name    = flag_str.substr(0,    fi);
        string flag_value   = flag_str.substr(fi+1, fj);
        params.env_config.push_back(std::make_pair(flag_name, flag_value));
        i = ++j;
        j = flags_str.find(delim, j);
      }
      if (j == string::npos) {
        // Split into key and value:
        string::size_type k = flags_str.find('=', i+1);
        string flag_name    = flags_str.substr(i, k-i);
        string flag_value   = flags_str.substr(k+1, flags_str.length());
        params.env_config.push_back(std::make_pair(flag_name, flag_value));
      }
    }
  }
  // Add environment variables starting with 'I_MPI_' or 'MP_' to
  // params.env_config:
  int    i          = 1;
  char * env_var_kv = *environ;
  for (; env_var_kv != 0; ++i) {
    string flag_str(env_var_kv);
    if (flag_str.substr(0, 6) == "I_MPI_" ||
        flag_str.substr(0, 4) == "MV2_"   ||
        flag_str.substr(0, 3) == "MP_")
    {
      // Split into key and value:
      string::size_type fi = flag_str.find('=');
      string::size_type fj = flag_str.find('=', fi);
      if (fj == string::npos) {
        fj = flag_str.length();
      }
      string flag_name    = flag_str.substr(0,    fi);
      string flag_value   = flag_str.substr(fi+1, fj);
      params.env_config.push_back(std::make_pair(flag_name, flag_value));
    }
    env_var_kv = *(environ + i);
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
  size_t box_width = 53;
  std::string separator(box_width, '-');
  size_t numa_nodes = dash::util::Locality::NumNumaNodes();
  size_t local_cpus = dash::util::Locality::NumCPUs();
  cout << separator           << endl
       << "-- bench.10.summa" << endl
       << "-- environment:"   << endl
       << std::right
       << "--   NUMA nodes:"  << std::setw(box_width-16) << numa_nodes << endl
       << "--   Local CPUs:"  << std::setw(box_width-16) << local_cpus << endl
       << "--   Flags:"
       << endl;
  for (auto flag : params.env_config) {
    cout << "--     " << std::setw(box_width-22) << std::left  << flag.first
                      << std::setw(15)           << std::right << flag.second
         << endl;
  }
  size_t w = box_width-25;
  cout << std::right
#ifdef DASH__BENCH_10_SUMMA__DOUBLE_PREC
       << "-- data type:            " << setw(w) << "double" << endl
#else
       << "-- data type:            " << setw(w) << "float"  << endl
#endif
       << "-- parameters:" << endl
       << "--   -s    variant:      " << setw(w) << params.variant   << endl
       << "--   -sb   size base:    " << setw(w) << params.size_base << endl
       << "--   -nmax units max:    " << setw(w) << params.units_max << endl
       << "--   -nx   team size x:  " << setw(w) << params.units_x   << endl
       << "--   -ny   team size y:  " << setw(w) << params.units_y   << endl
       << "--   -ninc units inc:    " << setw(w) << params.units_inc << endl
       << "--   -nt   threads/unit: " << setw(w) << params.threads   << endl
       << "--   -emax exp max:      " << setw(w) << params.exp_max   << endl
       << "--   -rmax rep. max:     " << setw(w) << params.rep_max   << endl
       << "--   -rb   rep. base:    " << setw(w) << params.rep_base  << endl
       << "-- environment:" << endl;
#ifdef MPI_IMPL_ID
  cout << "--   MPI implementation:"
       << setw(box_width-24) << dash__toxstr(MPI_IMPL_ID) << endl;
#endif
  cout << "--   MPI shared windows:";
  if (params.env_mpi_shared_win) {
    cout << setw(box_width-24) << "enabled"  << endl;
  } else {
    cout << setw(box_width-24) << "disabled" << endl;
  }
  cout << "--   Intel MKL:";
  if (params.env_mkl) {
    cout << setw(box_width-15) << " enabled" << endl;
    cout << "--   MKL dynamic:";
    if (params.mkl_dyn) {
      cout << setw(box_width-17) << "enabled"  << endl;
    } else {
      cout << setw(box_width-17) << "disabled" << endl;
    }
    cout << "--   ScaLAPACK:";
    if (params.env_scalapack) {
      cout << setw(box_width-15) << "enabled" << endl;
    } else {
      cout << setw(box_width-15) << "disabled" << endl;
    }
  } else {
    cout << setw(box_width-15) << "disabled" << endl
         << "-- ! MKL not available,"           << endl
         << "-- ! falling back to naive local"  << endl
         << "-- ! matrix multiplication"        << endl
         << endl;
  }
  cout << separator
       << endl;
}
