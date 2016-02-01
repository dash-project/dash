/*
 * Sequential GUPS benchmark for various pattern types.
 *
 */
/* @DASH_HEADER@ */

//#define DASH__MKL_MULTITHREADING
#define DASH__BENCH_10_SUMMA__DOUBLE_PREC
//#define DASH_ENABLE_SCALAPACK

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
//#include <mkl_scalapack.h>
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
  bool        env_scalapack;
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

std::pair<double, double> test_pblas(
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
    } else if (variant == "mkl" || variant == "blas") {
      mem_local_mb = ( sizeof(value_t) * (
                         // matrices A, B, C:
                         (3 * n * n)
                       ) / 1024 ) / 1024;
    } else if (variant == "pblas") {
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
  if (variant == "mkl" || variant == "blas") {
    t_mmult = test_blas(n, num_repeats);
  } else if (variant == "pblas") {
    t_mmult = test_pblas(n, num_repeats);
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

  DASH_ASSERT_MSG(pattern.extent(0) % dash::size() == 0,
                  "Matrix columns not divisible by number of units");
  DASH_ASSERT_MSG(pattern.extent(1) % dash::size() == 0,
                  "Matrix rows not divisible by number of units");

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
  unsigned repeat)
{
#if defined(DASH_ENABLE_MKL) && defined(DASH_ENABLE_SCALAPACK)
  MKL_INT          i_one     =  1;
  MKL_INT          i_zero    =  0;
  MKL_INT          i_negone  = -1;
  const float      f_one     =  1.0E+0;
  const float      f_zero    =  0.0E+0;
  const float      f_negone  = -1.0E+0;
  const double     d_one     =  1.0E+0;
  const double     d_zero    =  0.0E+0;
  const double     d_negone  = -1.0E+0;
  int64_t          N         = sb;
  char             storage[] = "R";
  char             trans_a[] = "N";
  char             trans_b[] = "N";
  int            * desc_a;
  int            * desc_b;
  int            * desc_c;
  value_t        * matrix_a_dist;
  value_t        * matrix_b_dist;
  value_t        * matrix_c_dist;

  std::pair<double, double> time;

  int ictxt, myid, myrow, mycol;
  int ierr;
  int numproc  = dash::size();
  int nprow    = numproc / 4,
      npcol    = 4;
  // Block extent in block size (nb x nb):
  int nb       = N / nprow;

  // Number of rows in submatrix C used in the computation, and
  // if transa = 'N', the number of rows in submatrix A.
  int m   = N / nprow;
  // Number of columns in submatrix C used in the computation, and
  // if transb = 'N', the number of columns in submatrix B
  int n   = N / npcol;
  // If transa = 'N', the number of columns in submatrix A.
  // If transb = 'N', the number of rows in submatrix B.
  int k   = N / npcol;
  // Row index of the global matrix A, identifying the first row of the
  // submatrix A.
  int i_a = myrow * nprow;
  // Column index of the global matrix A, identifying the first column of the
  // submatrix A.
  int j_a = mycol * npcol;
  // Row index of the global matrix B, identifying the first row of the
  // submatrix B.
  int i_b = myrow * nprow;
  // Column index of the global matrix B, identifying the first column of the
  // submatrix B.
  int j_b = mycol * npcol;
  // Row index of the global matrix C, identifying the first row of the
  // submatrix C.
  int i_c = myrow * nprow;
  // Column index of the global matrix C, identifying the first column of the
  // submatrix C.
  int j_c = mycol * npcol;

  int lld_a, lld_b, lld_c;
  int mp;
  int kp;
  int kq;
  int nq;

  auto ts_init_start = Timer::Now();

  // Note:
  // Alternatives to blacs_foo functions: Cblacs_foo

  // Initialize process grid:
  blacs_get_(&i_negone, &i_zero, &ictxt);
  blacs_gridinit_(&ictxt, storage, &nprow, &npcol);
  blacs_gridinfo_(&ictxt,          &nprow, &npcol, &myrow, &mycol);
  // Initialize array descriptors for matrix A, B, C:
  mp = numroc_(&m, &nb, &myrow, &i_zero, &nprow);
  kp = numroc_(&k, &nb, &myrow, &i_zero, &nprow);
  kq = numroc_(&k, &nb, &mycol, &i_zero, &npcol);
  nq = numroc_(&n, &nb, &mycol, &i_zero, &npcol);
  lld_a = dash::math::max(mp, 1);
  lld_b = dash::math::max(kp, 1);
  lld_c = dash::math::max(mp, 1);
  /*
     descinit(&desc,
              row,         cols,
              block_rows,  block_cols,
              pgrid_row_0, pgrid_col_0,
              &context,
              local_leading_dim);
  */
  descinit_(desc_a, &m, &k, &nb, &nb, 0, 0, &ictxt, &lld_a, &ierr);
  DASH_ASSERT_EQ(0, ierr, "descinit(desc_a) failed");
  descinit_(desc_b, &k, &n, &nb, &nb, 0, 0, &ictxt, &lld_b, &ierr);
  DASH_ASSERT_EQ(0, ierr, "descinit(desc_b) failed");
  descinit_(desc_c, &m, &n, &nb, &nb, 0, 0, &ictxt, &lld_c, &ierr);
  DASH_ASSERT_EQ(0, ierr, "descinit(desc_c) failed");

  // Allocate and initialize matrices A, B, C:
  // Note:
  // Leading dimensions in effect denote the linear storage order such that:
  // A[i][j] := A[j * lda + i]

  matrix_a_dist = (value_t *)(mkl_malloc(mp * nq * sizeof(double), 64));
  matrix_b_dist = (value_t *)(mkl_malloc(mp * nq * sizeof(double), 64));
  matrix_c_dist = (value_t *)(mkl_malloc(mp * nq * sizeof(double), 64));

#if 0
  // Call pdgeadd_ to distribute matrix (i.e. copy A into A_distr).
  // Computes sum of two matrices A,A': A' := alpha * A + beta * A'
  // so using alpha = 1, beta = 0 copies A to A'. Here, A' is the distributed
  // matrix of A (matrix_a_dist).
  matrix_a = (value_t *)(mkl_malloc(sizeof(value_t) * N * N, 64));
  matrix_b = (value_t *)(mkl_malloc(sizeof(value_t) * N * N, 64));
  matrix_c = (value_t *)(mkl_malloc(sizeof(value_t) * N * N, 64));

  pdgeadd_("N", &m, &n,
           &d_one,  matrix_a,      &i_one, &i_one, descA,
           &d_zero, matrix_a_dist, &i_one, &i_one, descA_distr);
#endif

  time.first = Timer::ElapsedSince(ts_init_start);

  auto ts_multiply_start = Timer::Now();
#ifdef DASH__BENCH_10_SUMMA__DOUBLE_PREC
  pdgemm_(
      trans_a,
      trans_b,
      &m,
      &n,
      &k,
      &d_one,
      static_cast<const double *>(matrix_a_dist),
      &i_a,
      &j_a,
      desc_a,
      static_cast<const double *>(matrix_b_dist),
      &i_b,
      &j_b,
      desc_b,
      &d_zero,
      static_cast<double *>(matrix_c_dist),
      &i_c,
      &j_c,
      desc_c);
  time.second = Timer::ElapsedSince(ts_multiply_start);
#else
  DASH_THROW(dash::exception::RuntimeError,
             "PBLAS benchmark expects type double");
#endif

  // Exit process grid:
  blacs_gridexit_(&ictxt);
  blacs_exit_(&i_zero);

  mkl_free(matrix_a_dist);
  mkl_free(matrix_b_dist);
  mkl_free(matrix_c_dist);

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
  params.threads            = 1;
  params.exp_max            = 4;
  params.mkl_dyn            = false;
  params.env_mpi_shared_win = true;
  params.env_mkl            = false;
  params.env_scalapack      = false;
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
    cout << "--   ScaLAPACK:          ";
    if (params.env_scalapack) {
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
