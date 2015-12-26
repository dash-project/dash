#ifndef DASH__ALGORITHM__SUMMA_H_
#define DASH__ALGORITHM__SUMMA_H_

#include <dash/Exception.h>
#include <dash/bindings/LAPACK.h>

namespace dash {

namespace internal {

/**
 * Naive matrix multiplication for local multiplication of matrix blocks,
 * used only for tests and where BLAS is not available.
 */
template<typename MatrixType>
void multiply_naive(
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

  auto num_blocks_l = 0;
  auto num_blocks_m = 0;
  auto num_blocks_n = 0;

  auto block_size_x = pattern_a.blocksizespec().extent(0);
  auto block_size_y = pattern_a.blocksizespec().extent(1);
  auto block_size   = block_size_x * block_size_y;

  Element * local_block_a = new Element[block_size];
  Element * local_block_b = new Element[block_size];

  for (int k = 0; k < num_blocks_l; k++) {
    for (int i = 0; i < num_blocks_m; i += pgrid_nrow) {
      // Local copy of block A[i + myrow][k]:
      auto block_a = A.block({ i + myrow, k });
      dash::copy(block_a.begin(),
                 block_a.end(),
                 local_block_a);

      for (int j = 0; j < num_blocks_n; j += pgrid_ncol) {
        // Local copy of block B[k][j + mycol]:
        auto block_b = B.block({ k, j + myrow });
        dash::copy(block_b.begin(),
                   block_b.end(),
                   local_block_b);
        // Result block, in local memory:
        auto block_c = C.block({ i + myrow, j + mycol });
        DASH_ASSERT(block_c.is_local());
        // Local matrix multiplication:
        dash::multiply_naive(
            local_block_a,
            local_block_b,
            block_c.begin());
      } // close of the j loop
    } // close of the i loop
  } // close of the k loop 
}

/**
 * Registration of \c dash::summa as an implementation of matrix-matrix
 * multiplication (xDGEMM).
 *
 * Delegates  \c dash::multiply<MatrixType>
 * to         \c dash::summa<MatrixType>
 * if         \c MatrixType::pattern_type
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
