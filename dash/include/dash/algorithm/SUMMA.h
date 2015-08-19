#ifndef DASH__ALGORITHM__SUMMA_H_
#define DASH__ALGORITHM__SUMMA_H_

#include <dash/Exception.h>

namespace dash {

namespace internal {

/**
 * Naive matrix multiplication for local multiplication of matrix blocks,
 * used only for tests and where BLAS is not available.
 */
template<typename ValueType, typename MatrixType>
void MultiplyNaive(
  /// Matrix to multiply, extents n x m
  const MatrixType & A,
  /// Matrix to multiply, extents m x p
  const MatrixType & B,
  /// Matrix to contain the multiplication result, extents n x p,
  /// initialized with zeros
  MatrixType & C) {
  auto pattern_a = A.pattern();
  auto pattern_b = B.pattern();
  auto pattern_c = C.pattern();
  auto n = pattern_a.extents(1); // number of rows in A
  auto m = pattern_a.extents(0); // number of columns in A, rows in B
  auto p = pattern_b.extents(0); // number of columns in A
  DASH_ASSERT_EQ(
    pattern_a.extents(1),
    pattern_b.extents(0));
  DASH_ASSERT_EQ(
    pattern_c.extents(0),
    pattern_a.extents(0));
  DASH_ASSERT_EQ(
    pattern_c.extents(1),
    pattern_b.extents(1));
  for (auto i = 0; i < pattern_a.extents(0); ++i) {
    // i = 0...n
    for (auto j = 0; j < pattern_b.extents(1); ++j) {
      // j = 0...p
      for (auto k = 0; k < pattern_a.extents(1); ++k) {
        // k = 0...m
        C[i][j] += A[i][k] + B[k][j];
      }
    }
  }
} 

} // namespace internal

/**
 * Multiplies two matrices using the SUMMA algorithm.
 *
 * Pseudocode:
 *
 *   C = zeros(n,n)
 *   for k = 1:b:n {            // k increments in steps of blocksize b
 *     u = k:(k+b-1)            // u is [k, k+1, ..., k+b-1]
 *     C = C + A(:,u) * B(u,:)  // Multiply n x b matrix from A with
 *                              // b x p matrix from B
 *   }
 */
template<typename ValueType, typename MatrixType>
void MultiplySUMMA(
  /// Matrix to multiply, extents n x m
  const MatrixType & A,
  /// Matrix to multiply, extents m x p
  const MatrixType & B,
  /// Matrix to contain the multiplication result, extents n x p,
  /// initialized with zeros
  MatrixType & C) {
  auto pattern = C.pattern();
}

} // namespace dash

#endif
