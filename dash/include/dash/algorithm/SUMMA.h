#ifndef DASH__ALGORITHM__SUMMA_H_
#define DASH__ALGORITHM__SUMMA_H_

#include <dash/Exception.h>

#include <utility>

#ifdef DASH_ENABLE_MKL
#include <mkl.h>
#include <mkl_types.h>
#include <mkl_cblas.h>
#include <mkl_blas.h>
#include <mkl_lapack.h>
#endif

#define SUMMA_EXPERIMENTAL_

namespace dash {

namespace internal {

#ifdef DASH_ENABLE_MKL
/**
 * Matrix multiplication for local multiplication of matrix blocks via MKL.
 */
template<
  typename   ValueType,
  MemArrange storage    = dash::ROW_MAJOR>
void multiply_local(
  /// Matrix to multiply, m rows by k columns.
  const ValueType * A,
  /// Matrix to multiply, k rows by n columns.
  const ValueType * B,
  /// Matrix to contain the multiplication result, m rows by n columns.
  ValueType       * C,
  size_t            m,
  size_t            n,
  size_t            k)
{
  mkl_set_num_threads(1);
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
  double alpha = 1.0;
  /// Real value used to scale matrix C.
  /// Using 1.0 to preserve existing values in C such that C += A x B.
  double beta  = 1.0;
/*
  void cblas_dgemm(const CBLAS_LAYOUT    Layout,
                   const CBLAS_TRANSPOSE TransA,
                   const CBLAS_TRANSPOSE TransB,
                   const MKL_INT         M,
                   const MKL_INT         N,
                   const MKL_INT         K,
                   const double          alpha,
                   const double *        A,
                   const MKL_INT         lda,
                   const double *        B,
                   const MKL_INT         ldb,
                   const double          beta,
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

#else
/**
 * Naive matrix multiplication for local multiplication of matrix blocks,
 * used only for tests and where MKL is not available.
 */
template<
  typename   ValueType,
  MemArrange storage    = dash::ROW_MAJOR>
void multiply_local(
  /// Matrix to multiply, extents n x m
  const ValueType * A,
  /// Matrix to multiply, extents m x p
  const ValueType * B,
  /// Matrix to contain the multiplication result, extents n x p
  ValueType       * C,
  size_t            m,
  size_t            n,
  size_t            p)
{
  ValueType c_sum = 0;
  for (auto i = 0; i < n; ++i) {
    // row i = 0...n
    for (auto j = 0; j < p; ++j) {
      // column j = 0...p
      c_sum = C[i * p + j]; // = C[j][i]
      for (auto k = 0; k < m; ++k) {
        // k = 0...m
        auto ik     = i * m + k;
        auto kj     = k * m + j;
        auto value  = A[ik] * B[kj];
        c_sum      += value;
      }
      C[i * p + j] = c_sum; // C[j][i] = c_sum
    }
  }
}

#endif // ifdef DASH_ENABLE_MKL

} // namespace internal

/// Constraints on pattern partitioning properties of matrix operands passed to
/// \c dash::summa.
typedef dash::pattern_partitioning_properties<
            // Block extents are constant for every dimension.
            dash::pattern_partitioning_tag::rectangular,
            // Identical number of elements in every block.
            dash::pattern_partitioning_tag::balanced
        > summa_pattern_partitioning_constraints;
/// Constraints on pattern mapping properties of matrix operands passed to
/// \c dash::summa.
typedef dash::pattern_mapping_properties<
            // Number of blocks assigned to a unit may differ.
            dash::pattern_mapping_tag::unbalanced,
            // Every unit mapped in any single slice in every dimension.
            dash::pattern_mapping_tag::diagonal
        > summa_pattern_mapping_constraints;
/// Constraints on pattern layout properties of matrix operands passed to
/// \c dash::summa.
typedef dash::pattern_layout_properties<
            // Elements are contiguous in local memory within single block.
            dash::pattern_layout_tag::blocked,
            // Local element order corresponds to a logical linearization
            // within single blocks.
            // Required for cache-optimized block matrix multiplication.
            dash::pattern_layout_tag::linear
        > summa_pattern_layout_constraints;

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
  MatrixTypeA & A,
  /// Matrix to multiply, extents m x p
  MatrixTypeB & B,
  /// Matrix to contain the multiplication result, extents n x p,
  /// initialized with zeros
  MatrixTypeC & C)
{
  typedef typename MatrixTypeA::value_type   value_type;
  typedef typename MatrixTypeA::index_type   index_t;
  typedef typename MatrixTypeA::pattern_type pattern_a_type;
  typedef typename MatrixTypeB::pattern_type pattern_b_type;
  typedef typename MatrixTypeC::pattern_type pattern_c_type;
  typedef std::array<index_t, 2>             coords_t;

  static_assert(
      std::is_same<value_type, double>::value,
      "dash::summa expects matrix element type double");

  DASH_LOG_DEBUG("dash::summa()");
  // Verify that matrix patterns satisfy pattern constraints:
  if (!dash::check_pattern_constraints<
         summa_pattern_partitioning_constraints,
         summa_pattern_mapping_constraints,
         summa_pattern_layout_constraints
       >(A.pattern())) {
    DASH_THROW(
      dash::exception::InvalidArgument,
      "dash::summa(): "
      "pattern of first matrix argument does not match constraints");
  }
  if (!dash::check_pattern_constraints<
         summa_pattern_partitioning_constraints,
         summa_pattern_mapping_constraints,
         summa_pattern_layout_constraints
       >(B.pattern())) {
    DASH_THROW(
      dash::exception::InvalidArgument,
      "dash::summa(): "
      "pattern of second matrix argument does not match constraints");
  }
  if (!dash::check_pattern_constraints<
         summa_pattern_partitioning_constraints,
         summa_pattern_mapping_constraints,
         summa_pattern_layout_constraints
       >(C.pattern())) {
    DASH_THROW(
      dash::exception::InvalidArgument,
      "dash::summa(): "
      "pattern of result matrix does not match constraints");
  }
  DASH_LOG_TRACE("dash::summa", "matrix pattern properties valid");

  //    A         B         C
  //  _____     _____     _____
  // |     |   |     |   |     |
  // n     | x m     | = n     |
  // |_ m _|   |_ p _|   |_ p _|
  //
  dash::Team & team = C.team();
  auto unit_id      = team.myid();
  // Check run-time invariants on pattern instances:
  auto pattern_a    = A.pattern();
  auto pattern_b    = B.pattern();
  auto pattern_c    = C.pattern();
  auto m = pattern_a.extent(0); // number of columns in A, rows in B
  auto n = pattern_a.extent(1); // number of rows in A and C
  auto p = pattern_b.extent(0); // number of columns in B and C

  const dash::MemArrange memory_order = pattern_a.memory_order();

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

  // Patterns are balanced, all blocks have identical size:
  auto block_size_m  = pattern_a.block(0).extent(0);
  auto block_size_n  = pattern_b.block(0).extent(1);
  auto block_size_p  = pattern_b.block(0).extent(0);
  auto num_blocks_l  = n / block_size_n;
  auto num_blocks_m  = m / block_size_m;
  auto num_blocks_n  = n / block_size_n;
  auto num_blocks_p  = p / block_size_p;
  // Size of temporary local blocks
  auto block_a_size  = block_size_n * block_size_m;
  auto block_b_size  = block_size_m * block_size_p;
  // Number of units in rows and columns:
  auto teamspec      = C.pattern().teamspec();

  DASH_LOG_TRACE("dash::summa", "blocks:",
                 "m:", num_blocks_m, "*", block_size_m,
                 "n:", num_blocks_n, "*", block_size_n,
                 "p:", num_blocks_p, "*", block_size_p);
  DASH_LOG_TRACE("dash::summa", "number of units:",
                 "cols:", teamspec.extent(0),
                 "rows:", teamspec.extent(1));
  DASH_LOG_TRACE("dash::summa", "allocating local temporary blocks, sizes:",
                 "A:", block_a_size,
                 "B:", block_b_size);
  value_type * buf_block_a_get    = new value_type[block_a_size];
  value_type * buf_block_b_get    = new value_type[block_b_size];
  value_type * buf_block_a_comp   = new value_type[block_a_size];
  value_type * buf_block_b_comp   = new value_type[block_b_size];
  // Copy of buffer pointers for swapping, delete[] on swapped pointers tends
  // to crash:
  value_type * local_block_a_get  = buf_block_a_get;
  value_type * local_block_b_get  = buf_block_b_get;
  value_type * local_block_a_comp = buf_block_a_comp;
  value_type * local_block_b_comp = buf_block_b_comp;
  // -------------------------------------------------------------------------
  // Prefetch blocks from A and B for first local multiplication:
  // -------------------------------------------------------------------------
  // Block coordinates of submatrices of A and B to be prefetched:
  coords_t block_a_get_coords;
  coords_t block_b_get_coords;
  // Local block index of local submatrix of C for multiplication result of
  // blocks to be prefetched:
  auto     l_block_c_get       = C.local.block(0);
  auto     l_block_c_get_view  = l_block_c_get.begin().viewspec();
  index_t  l_block_c_get_row   = l_block_c_get_view.offset(1) / block_size_n;
  index_t  l_block_c_get_col   = l_block_c_get_view.offset(0) / block_size_p;
  // Block coordinates of blocks in A and B to prefetch:
  block_a_get_coords = coords_t { static_cast<index_t>(unit_id),
                                  l_block_c_get_row };
  block_b_get_coords = coords_t { l_block_c_get_col,
                                  static_cast<index_t>(unit_id) };
  // Local block index of local submatrix of C for multiplication result of
  // currently prefetched blocks:
  auto     l_block_c_comp      = l_block_c_get;
  auto     l_block_c_comp_view = l_block_c_comp.begin().viewspec();
  index_t  l_block_c_comp_row  = l_block_c_comp_view.offset(1) / block_size_n;
  index_t  l_block_c_comp_col  = l_block_c_comp_view.offset(0) / block_size_p;
  // Prefetch blocks from A and B for computation in next iteration:
  auto block_a = A.block(block_a_get_coords);
  auto block_b = B.block(block_b_get_coords);
  DASH_LOG_TRACE("dash::summa", "summa.prefetch.block.a",
                 "block:", block_a_get_coords,
                 "unit:",  block_a.begin().lpos().unit,
                 "view:",  block_a.begin().viewspec());
  auto get_a = dash::copy_async(block_a.begin(), block_a.end(),
                                local_block_a_comp);
  DASH_LOG_TRACE("dash::summa", "summa.prefetch.block.b",
                 "block:", block_b_get_coords,
                 "unit:",  block_b.begin().lpos().unit,
                 "view:",  block_b.begin().viewspec());
  auto get_b = dash::copy_async(block_b.begin(), block_b.end(),
                                local_block_b_comp);
  DASH_LOG_TRACE("dash::summa", "summa.block",
                 "waiting for prefetching of blocks");
  get_a.wait();
  get_b.wait();
  DASH_LOG_TRACE("dash::summa", "summa.block",
                 "prefetching of blocks completed");
  // -------------------------------------------------------------------------
  // Iterate local blocks in matrix C:
  // -------------------------------------------------------------------------
  auto num_local_blocks_c = (num_blocks_n * num_blocks_p) / teamspec.size();
  DASH_LOG_TRACE("dash::summa", "summa.block.C",
                 "C.num.local.blocks:",  num_local_blocks_c,
                 "C.num.column.blocks:", num_blocks_m);
  for (auto lb = 0; lb < num_local_blocks_c; ++lb) {
    // Block coordinates for current block multiplication result:
    l_block_c_comp      = C.local.block(lb);
    l_block_c_comp_view = l_block_c_comp.begin().viewspec();
    l_block_c_comp_row  = l_block_c_comp_view.offset(1) / block_size_n;
    l_block_c_comp_col  = l_block_c_comp_view.offset(0) / block_size_p;
    // Block coordinates for next block multiplication result:
    l_block_c_get       = l_block_c_comp;
    l_block_c_get_view  = l_block_c_comp_view;
    l_block_c_get_row   = l_block_c_get_row;
    l_block_c_get_col   = l_block_c_get_col;
    DASH_LOG_TRACE("dash::summa", "summa.block.comp", "C.local.block",
                   "l_block_idx:", lb,
                   "row:",         l_block_c_comp_row,
                   "col:",         l_block_c_comp_col,
                   "view:",        l_block_c_comp_view);
    // -----------------------------------------------------------------------
    // Iterate blocks in columns of A / rows of B:
    // -----------------------------------------------------------------------
    for (index_t block_k = 0; block_k < num_blocks_m; ++block_k) {
      DASH_LOG_TRACE("dash::summa", "summa.block.k", block_k);
      // ---------------------------------------------------------------------
      // Prefetch local copy of blocks from A and B for multiplication in
      // next iteration.
      // ---------------------------------------------------------------------
      bool last = (lb == num_local_blocks_c - 1) &&
                  (block_k == num_blocks_m - 1);
      // Do not prefetch blocks in last iteration:
      if (!last) {
        auto block_get_k = block_k + 1;
        block_get_k = (block_get_k + unit_id) % num_blocks_m;
        // Block coordinate of local block in matrix C to prefetch:
        if (block_k == num_blocks_m - 1) {
          // Prefetch for next local block in matrix C:
          block_get_k        = unit_id;
          l_block_c_get      = C.local.block(lb + 1);
          l_block_c_get_view = l_block_c_get.begin().viewspec();
          l_block_c_get_row  = l_block_c_get_view.offset(1) / block_size_n;
          l_block_c_get_col  = l_block_c_get_view.offset(0) / block_size_p;
        }
        // Block coordinates of blocks in A and B to prefetch:
        block_a_get_coords = coords_t { block_get_k,       l_block_c_get_row };
        block_b_get_coords = coords_t { l_block_c_get_col, block_get_k       };

        block_a = A.block(block_a_get_coords);
        DASH_LOG_TRACE("dash::summa", "summa.prefetch.block.a",
                       "block:", block_a_get_coords,
                       "unit:",  block_a.begin().lpos().unit,
                       "view:",  block_a.begin().viewspec());
        get_a = dash::copy_async(block_a.begin(),
                                 block_a.end(),
                                 local_block_a_get);
        block_b = B.block(block_b_get_coords);
        DASH_LOG_TRACE("dash::summa", "summa.prefetch.block.b",
                       "block:", block_b_get_coords,
                       "unit:",  block_b.begin().lpos().unit,
                       "view:",  block_b.begin().viewspec());
        get_b = dash::copy_async(block_b.begin(),
                                 block_b.end(),
                                 local_block_b_get);
      } else {
        DASH_LOG_TRACE("dash::summa", " ->",
                       "last block multiplication",
                       "lb:", lb, "bk:", block_k);
      }
      // ---------------------------------------------------------------------
      // Computation of matrix product of local block matrices:
      // ---------------------------------------------------------------------
      DASH_LOG_TRACE("dash::summa", "summa.block.comp.multiply",
                     "multiplying local block matrices",
                     "C.local.block.comp:", lb,
                     "view:", l_block_c_comp.begin().viewspec());
      dash::internal::multiply_local<value_type, memory_order>(
          local_block_a_comp,
          local_block_b_comp,
          l_block_c_comp.begin().local(),
          block_size_m,
          block_size_n,
          block_size_p);
#ifdef DASH_ENABLE_LOGGING
      auto A = local_block_a_comp;
      auto B = local_block_b_comp;
      auto C = l_block_c_comp.begin().local();
      std::vector< std::vector<value_type> > a_rows;
      std::vector< std::vector<value_type> > b_rows;
      std::vector< std::vector<value_type> > c_rows;
      for (auto row = 0; row < block_size_n; ++row) {
        std::vector<value_type> a_row;
        std::vector<value_type> b_row;
        std::vector<value_type> c_row;
        for (auto col = 0; col < block_size_m; ++col) {
          auto offset = row * block_size_n + col;
          a_row.push_back(A[offset]);
          b_row.push_back(B[offset]);
          c_row.push_back(C[offset]);
        }
        a_rows.push_back(a_row);
        b_rows.push_back(b_row);
        c_rows.push_back(c_row);
      }
      for (auto row : a_rows) {
        DASH_LOG_TRACE("dash::internal::multiply_local", "summa.multiply",
                       "  submatrix A:", row);
      }
      for (auto row : b_rows) {
        DASH_LOG_TRACE("dash::internal::multiply_local", "summa.multiply",
                       "x submatrix B:", row);
      }
      for (auto row : c_rows) {
        DASH_LOG_TRACE("dash::internal::multiply_local", "summa.multiply",
                       "= submatrix C:", row);
      }
#endif
      if (!last) {
        // -------------------------------------------------------------------
        // Wait for local copies:
        // -------------------------------------------------------------------
        DASH_LOG_TRACE("dash::summa", "summa.prefetch.wait",
                       "waiting for local copies of next blocks");
        get_a.wait();
        get_b.wait();
        DASH_LOG_TRACE("dash::summa", "summa.prefetch.completed",
                       "local copies of next blocks received");
        // -------------------------------------------------------------------
        // Swap communication and computation buffers:
        // -------------------------------------------------------------------
        std::swap(local_block_a_get, local_block_a_comp);
        std::swap(local_block_b_get, local_block_b_comp);
      }
    }
  } // for lb

  DASH_LOG_TRACE("dash::summa", "locally completed");
  delete[] buf_block_a_get;
  delete[] buf_block_b_get;
  delete[] buf_block_a_comp;
  delete[] buf_block_b_comp;

  DASH_LOG_TRACE("dash::summa", "waiting for other units");
  C.barrier();

  DASH_LOG_TRACE("dash::summa >", "finished");
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
    dash::summa_pattern_partitioning_constraints,
    dash::summa_pattern_mapping_constraints,
    dash::summa_pattern_layout_constraints,
    typename MatrixTypeA::pattern_type
  >::satisfied::value &&
  dash::pattern_constraints<
    dash::summa_pattern_partitioning_constraints,
    dash::summa_pattern_mapping_constraints,
    dash::summa_pattern_layout_constraints,
    typename MatrixTypeB::pattern_type
  >::satisfied::value &&
  dash::pattern_constraints<
    dash::summa_pattern_partitioning_constraints,
    dash::summa_pattern_mapping_constraints,
    dash::summa_pattern_layout_constraints,
    typename MatrixTypeC::pattern_type
  >::satisfied::value,
  void
>::type
multiply(
  /// Matrix to multiply, extents n x m
  MatrixTypeA & A,
  /// Matrix to multiply, extents m x p
  MatrixTypeB & B,
  /// Matrix to contain the multiplication result, extents n x p,
  /// initialized with zeros
  MatrixTypeC & C)
{
  dash::summa(A, B, C);
}

} // namespace dash

#endif
