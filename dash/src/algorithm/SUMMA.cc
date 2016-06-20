#include <dash/Exception.h>
#include <dash/Types.h>
#include <dash/Pattern.h>
#include <dash/algorithm/SUMMA.h>

// Prefer MKL if available:
#ifdef DASH_ENABLE_MKL
#include <mkl.h>
#include <mkl_types.h>
#include <mkl_cblas.h>
#include <mkl_blas.h>
#include <mkl_lapack.h>
// BLAS support:
#elif defined(DASH_ENABLE_BLAS)
extern "C"
{
  #include <cblas.h>
}
#endif

namespace dash {
namespace internal {

#if defined(DASH_ENABLE_MKL) || defined(DASH_ENABLE_BLAS)

/**
 * Matrix multiplication for local multiplication of matrix blocks via MKL.
 */
template<>
void mmult_local<float>(
  /// Matrix to multiply, m rows by k columns.
  const float * A,
  /// Matrix to multiply, k rows by n columns.
  const float * B,
  /// Matrix to contain the multiplication result, m rows by n columns.
  float       * C,
  long long     m,
  long long     n,
  long long     k,
  MemArrange    storage)
{
  typedef float value_t;
  /// Memory storage order of A, B, C:
  auto   strg  = (storage == dash::ROW_MAJOR)
                 ? CblasRowMajor
                 : CblasColMajor;
  /// Matrices A and B should not be transposed or conjugate transposed before
  /// multiplication.
  auto   tp_a  = CblasNoTrans;
  auto   tp_b  = CblasNoTrans;
  /// Leading dimension of A, or the number of elements between successive
  /// rows (for row major storage) in memory.
  auto   lda   = k;
  /// Leading dimension of B, or the number of elements between successive
  /// rows (for row major storage) in memory.
  auto   ldb   = n;
  /// Leading dimension of B, or the number of elements between successive
  /// rows (for row major storage) in memory.
  auto   ldc   = n;
  /// Real value used to scale the product of matrices A and B.
  value_t alpha = 1.0;
  /// Real value used to scale matrix C.
  /// Using 1.0 to preserve existing values in C such that C += A x B.
  value_t beta  = 1.0;
/*
  void cblas_?gemm(const CBLAS_LAYOUT    Layout,
                   const CBLAS_TRANSPOSE TransA,
                   const CBLAS_TRANSPOSE TransB,
                   const MKL_INT         M,
                   const MKL_INT         N,
                   const MKL_INT         K,
                   const ValueT          alpha,
                   const ValueT *        A,
                   const MKL_INT         lda,
                   const ValueT *        B,
                   const MKL_INT         ldb,
                   const ValueT          beta,
                   double *              C,
                   const MKL_INT         ldc);

*/
  cblas_sgemm(strg,
              tp_a, tp_b,
              m, n, k,
              alpha,
              A, lda,
              B, ldb,
              beta,
              C, ldc);
}
/**
 * Matrix multiplication for local multiplication of matrix blocks via MKL.
 */
template<>
void mmult_local<double>(
  /// Matrix to multiply, m rows by k columns.
  const double * A,
  /// Matrix to multiply, k rows by n columns.
  const double * B,
  /// Matrix to contain the multiplication result, m rows by n columns.
  double       * C,
  long long      m,
  long long      n,
  long long      k,
  MemArrange     storage)
{
  typedef double value_t;
  /// Memory storage order of A, B, C:
  auto   strg  = (storage == dash::ROW_MAJOR)
                 ? CblasRowMajor
                 : CblasColMajor;
  /// Matrices A and B should not be transposed or conjugate transposed before
  /// multiplication.
  auto   tp_a  = CblasNoTrans;
  auto   tp_b  = CblasNoTrans;
  /// Leading dimension of A, or the number of elements between successive
  /// rows (for row major storage) in memory.
  auto   lda   = k;
  /// Leading dimension of B, or the number of elements between successive
  /// rows (for row major storage) in memory.
  auto   ldb   = n;
  /// Leading dimension of B, or the number of elements between successive
  /// rows (for row major storage) in memory.
  auto   ldc   = n;
  /// Real value used to scale the product of matrices A and B.
  value_t alpha = 1.0;
  /// Real value used to scale matrix C.
  /// Using 1.0 to preserve existing values in C such that C += A x B.
  value_t beta  = 1.0;
/*
  void cblas_?gemm(const CBLAS_LAYOUT    Layout,
                   const CBLAS_TRANSPOSE TransA,
                   const CBLAS_TRANSPOSE TransB,
                   const MKL_INT         M,
                   const MKL_INT         N,
                   const MKL_INT         K,
                   const ValueT          alpha,
                   const ValueT *        A,
                   const MKL_INT         lda,
                   const ValueT *        B,
                   const MKL_INT         ldb,
                   const ValueT          beta,
                   double *              C,
                   const MKL_INT         ldc);

*/
  cblas_dgemm(strg,
              tp_a, tp_b,
              m, n, k,
              alpha,
              A, lda,
              B, ldb,
              beta,
              C, ldc);
}

#endif // defined(DASH_ENABLE_MKL) || defined(DASH_ENABLE_BLAS)

} // namespace internal
} // namespace dash
