#ifndef DASH__ALGORITHM__SUMMA_H_
#define DASH__ALGORITHM__SUMMA_H_

#include <dash/Exception.h>

namespace dash {

namespace internal {

/**
 * Naive matrix multiplication for local multiplication of matrix blocks,
 * used only for tests and where BLAS is not available.
 */
template<typename MatrixType>
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
  auto n = pattern_a.extent(1); // number of rows in A
  auto m = pattern_a.extent(0); // number of columns in A, rows in B
  auto p = pattern_b.extent(0); // number of columns in A
  DASH_ASSERT_EQ(
    pattern_a.extent(1),
    pattern_b.extent(0),
    "Extents of first operand in dimension 1 do not match extents of"
    "second operand in dimension 0");
  DASH_ASSERT_EQ(
    pattern_c.extent(0),
    pattern_a.extent(0),
    "Extents of result matrix in dimension 0 do not match extents of"
    "first operand in dimension 0");
  DASH_ASSERT_EQ(
    pattern_c.extent(1),
    pattern_b.extent(1),
    "Extents of result matrix in dimension 1 do not match extents of"
    "second operand in dimension 1");
  for (auto i = 0; i < n; ++i) {
    // i = 0...n
    for (auto j = 0; j < p; ++j) {
      // j = 0...p
      for (auto k = 0; k < m; ++k) {
        // k = 0...m
        C[i][j] += A[i][k] + B[k][j];
      }
    }
  }
} 

} // namespace internal

/// Constraints on pattern blocking properties of matrix operands passed to
/// \c dash::summa.
typedef dash::pattern_blocking_properties<
          // same number of elements in every block
          pattern_blocking_tag::balanced >
        summa_pattern_blocking_constraints;
/// Constraints on pattern topology properties of matrix operands passed to
/// \c dash::summa.
typedef dash::pattern_topology_properties<
          // same amount of blocks for every process
          pattern_topology_tag::balanced,
          // every process mapped in every row/column
          pattern_topology_tag::diagonal >
        summa_pattern_topology_constraints;
/// Constraints on pattern indexing properties of matrix operands passed to
/// \c dash::summa.
typedef dash::pattern_indexing_properties<
          // elements contiguous within block
          pattern_indexing_tag::local_phase >
        summa_pattern_indexing_constraints;

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
template<
  typename MatrixTypeA,
  typename MatrixTypeB,
  typename MatrixTypeC
>
void summa(
  /// Matrix to multiply, extents n x m
  const MatrixTypeA & A,
  /// Matrix to multiply, extents m x p
  const MatrixTypeB & B,
  /// Matrix to contain the multiplication result, extents n x p,
  /// initialized with zeros
  MatrixTypeC & C)
{
  DASH_LOG_TRACE("dash::summa()");
  // Verify that matrix patterns satisfy pattern constraints:
  if (!dash::check_pattern_constraints<
         summa_pattern_blocking_constraints,
         summa_pattern_topology_constraints,
         summa_pattern_indexing_constraints
       >(A.pattern())) {
    DASH_THROW(
      dash::exception::InvalidArgument,
      "dash::summa(): "
      "pattern of first matrix argument does not match constraints");
  }
  if (!dash::check_pattern_constraints<
         summa_pattern_blocking_constraints,
         summa_pattern_topology_constraints,
         summa_pattern_indexing_constraints
       >(B.pattern())) {
    DASH_THROW(
      dash::exception::InvalidArgument,
      "dash::summa(): "
      "pattern of second matrix argument does not match constraints");
  }
  if (!dash::check_pattern_constraints<
         summa_pattern_blocking_constraints,
         summa_pattern_topology_constraints,
         summa_pattern_indexing_constraints
       >(C.pattern())) {
    DASH_THROW(
      dash::exception::InvalidArgument,
      "dash::summa(): "
      "pattern of result matrix does not match constraints");
  }
  DASH_LOG_TRACE("dash::summa", "matrix pattern properties valid");

  // Check run-time invariants on pattern instances:
  auto pattern_a = A.pattern();
  auto pattern_b = B.pattern();
  auto pattern_c = C.pattern();
  auto n = pattern_a.extent(1); // number of rows in A
  auto m = pattern_a.extent(0); // number of columns in A, rows in B
  auto p = pattern_b.extent(0); // number of columns in A

  DASH_ASSERT_EQ(
    pattern_a.extent(1),
    pattern_b.extent(0),
    "dash::summa(): "
    "Extents of first operand in dimension 1 do not match extents of"
    "second operand in dimension 0");
  DASH_ASSERT_EQ(
    pattern_c.extent(0),
    pattern_a.extent(0),
    "dash::summa(): "
    "Extents of result matrix in dimension 0 do not match extents of"
    "first operand in dimension 0");
  DASH_ASSERT_EQ(
    pattern_c.extent(1),
    pattern_b.extent(1),
    "dash::summa(): "
    "Extents of result matrix in dimension 1 do not match extents of"
    "second operand in dimension 1");

  DASH_LOG_TRACE("dash::summa", "matrix pattern extents valid");
}

/**
 * Adapter function for matrix-matrix multiplication (xDGEMM) for the SUMMA
 * algorithm.
 *
 * Automatically delegates  \c dash::multiply<MatrixType>
 * to                       \c dash::summa<MatrixType>
 * if                       \c MatrixType::pattern_type
 * satisfies the pattern property constraints of the SUMMA implementation.
 */
template<
  typename MatrixTypeA,
  typename MatrixTypeB,
  typename MatrixTypeC
>
typename std::enable_if<
  dash::pattern_constraints<
    dash::summa_pattern_blocking_constraints,
    dash::summa_pattern_topology_constraints,
    dash::summa_pattern_indexing_constraints,
    typename MatrixTypeA::pattern_type
  >::satisfied::value &&
  dash::pattern_constraints<
    dash::summa_pattern_blocking_constraints,
    dash::summa_pattern_topology_constraints,
    dash::summa_pattern_indexing_constraints,
    typename MatrixTypeB::pattern_type
  >::satisfied::value &&
  dash::pattern_constraints<
    dash::summa_pattern_blocking_constraints,
    dash::summa_pattern_topology_constraints,
    dash::summa_pattern_indexing_constraints,
    typename MatrixTypeC::pattern_type
  >::satisfied::value,
  void
>::type
multiply(
  /// Matrix to multiply, extents n x m
  const MatrixTypeA & A,
  /// Matrix to multiply, extents m x p
  const MatrixTypeB & B,
  /// Matrix to contain the multiplication result, extents n x p,
  /// initialized with zeros
  MatrixTypeC & C)
{
  dash::summa(A, B, C);
}

} // namespace dash

#endif
