#ifndef DASH__ALGORITHM__SUMMA_H_
#define DASH__ALGORITHM__SUMMA_H_

#include <dash/Exception.h>
#include <dash/Types.h>
#include <dash/Pattern.h>
#include <dash/Future.h>
#include <dash/algorithm/Copy.h>
#include <dash/util/Trace.h>

#include <utility>

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

#define DASH_ALGORITHM_SUMMA_ASYNC_INIT_PREFETCH

namespace dash {

namespace internal {

#if defined(DASH_ENABLE_MKL) || defined(DASH_ENABLE_BLAS)
/**
 * Matrix multiplication for local multiplication of matrix blocks via MKL.
 */
template<typename  ValueType>
void mmult_local(
  /// Matrix to multiply, m rows by k columns.
  const ValueType * A,
  /// Matrix to multiply, k rows by n columns.
  const ValueType * B,
  /// Matrix to contain the multiplication result, m rows by n columns.
  ValueType       * C,
  long long         m,
  long long         n,
  long long         k,
  MemArrange        storage);
#else
/**
 * Naive matrix multiplication for local multiplication of matrix blocks,
 * used only for tests and where MKL is not available.
 */
template<typename ValueType>
void mmult_local(
  /// Matrix to multiply, extents n x m
  const ValueType * A,
  /// Matrix to multiply, extents m x p
  const ValueType * B,
  /// Matrix to contain the multiplication result, extents n x p
  ValueType       * C,
  long long         m,
  long long         n,
  long long         p,
  MemArrange        storage)
{
#ifndef DEBUG
  DASH_THROW(
    dash::exception::RuntimeError,
    "Called fallback implementation of DGEMM (only enabled in Debug)");
#endif
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
#endif // defined(DASH_ENABLE_MKL) || defined(DASH_ENABLE_BLAS)

} // namespace internal

/// Constraints on pattern partitioning properties of matrix operands passed
/// to \c dash::summa.
typedef dash::pattern_partitioning_properties<
            // Block extents are constant for every dimension.
            dash::pattern_partitioning_tag::rectangular,
            // Identical number of elements in every block.
            dash::pattern_partitioning_tag::balanced,
            // Matrices must be partitioned in more than one dimension.
            dash::pattern_partitioning_tag::ndimensional
        > summa_pattern_partitioning_constraints;
/// Constraints on pattern mapping properties of matrix operands passed to
/// \c dash::summa.
typedef dash::pattern_mapping_properties<
            // Every unit mapped to more than one block, required for
            // block prefetching to take effect.
            dash::pattern_mapping_tag::multiple,
            // Number of blocks assigned to a unit may differ.
            dash::pattern_mapping_tag::unbalanced
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

template<typename MatrixType>
using summa_pattern_constraints =
    typename dash::pattern_constraints<
        dash::summa_pattern_partitioning_constraints,
        dash::summa_pattern_mapping_constraints,
        dash::summa_pattern_layout_constraints,
        typename MatrixType::pattern_type>;

/**
 * Multiplies two matrices using the SUMMA algorithm.
 * Performs \c (2 * (nunits-1) * nunits^2) async copy operations of
 * submatrices in \c A and \c B.
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
  typedef typename MatrixTypeA::size_type    extent_t;
//typedef typename MatrixTypeA::pattern_type pattern_a_type;
//typedef typename MatrixTypeB::pattern_type pattern_b_type;
//typedef typename MatrixTypeC::pattern_type pattern_c_type;
  typedef std::array<index_t, 2>             coords_t;

  const bool shifted_tiling = dash::pattern_constraints<
                                dash::pattern_partitioning_properties<>,
                                dash::pattern_mapping_properties<
                                  dash::pattern_mapping_tag::diagonal
                                >,
                                dash::pattern_layout_properties<>,
                                typename MatrixTypeC::pattern_type
                              >::satisfied::value;
  const bool minimal_tiling = dash::pattern_constraints<
                                dash::pattern_partitioning_properties<
                                  dash::pattern_partitioning_tag::minimal
                                >,
                                dash::pattern_mapping_properties<>,
                                dash::pattern_layout_properties<>,
                                typename MatrixTypeC::pattern_type
                              >::satisfied::value;

  static_assert(
      std::is_floating_point<value_type>::value,
      "dash::summa expects matrix element type double or float");

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

  if (shifted_tiling) {
    DASH_LOG_TRACE("dash::summa",
                   "using communication scheme for diagonal-shift mapping");
  }
  if (minimal_tiling) {
    DASH_LOG_TRACE("dash::summa",
                   "using communication scheme for minimal partitioning");
  }
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
#if DASH_ENABLE_TRACE_LOGGING
  auto n = pattern_a.extent(1); // number of rows in A and C
  auto p = pattern_b.extent(0); // number of columns in B and C
#endif
  const dash::MemArrange memory_order = pattern_a.memory_order();

  DASH_ASSERT_EQ(
    pattern_a.extent(1),
    pattern_b.extent(0),
    "dash::summa(): "
    "Extents of first operand in dimension 1 do not match extents of "
    "second operand in dimension 0");
  DASH_ASSERT_EQ(
    pattern_c.extent(0),
    pattern_a.extent(0),
    "dash::summa(): "
    "Extents of result matrix in dimension 0 do not match extents of "
    "first operand in dimension 0");
  DASH_ASSERT_EQ(
    pattern_c.extent(1),
    pattern_b.extent(1),
    "dash::summa(): "
    "Extents of result matrix in dimension 1 do not match extents of "
    "second operand in dimension 1");

  DASH_LOG_TRACE("dash::summa", "matrix pattern extents valid");

  // Patterns are balanced, all blocks have identical size:
  auto block_size_m   = pattern_a.block(0).extent(0);
  auto block_size_n   = pattern_b.block(0).extent(1);
  auto block_size_p   = pattern_b.block(0).extent(0);
  auto num_blocks_m   = m / block_size_m;
#if DASH_ENABLE_TRACE_LOGGING
  auto num_blocks_n   = n / block_size_n;
  auto num_blocks_p   = p / block_size_p;
#endif
  // Size of temporary local blocks
  auto block_a_size   = block_size_n * block_size_m;
  auto block_b_size   = block_size_m * block_size_p;
  // Number of units in rows and columns:
  auto teamspec       = C.pattern().teamspec();
  auto unit_ts_coords = teamspec.coords(unit_id);

  DASH_LOG_TRACE("dash::summa", "blocks:",
                 "m:", num_blocks_m, "*", block_size_m,
                 "n:", num_blocks_n, "*", block_size_n,
                 "p:", num_blocks_p, "*", block_size_p);
  DASH_LOG_TRACE("dash::summa",
                 "number of units:",
                 "cols:", teamspec.extent(0),
                 "rows:", teamspec.extent(1),
                 "unit team coords:", unit_ts_coords);
  DASH_LOG_TRACE("dash::summa", "allocating local temporary blocks, sizes:",
                 "A:", block_a_size,
                 "B:", block_b_size);

#ifdef DASH_ENABLE_MKL
  value_type * buf_block_a_get    = (value_type *)(mkl_malloc(
                                      sizeof(value_type) * block_a_size, 64));
  value_type * buf_block_b_get    = (value_type *)(mkl_malloc(
                                      sizeof(value_type) * block_b_size, 64));
  value_type * buf_block_a_comp   = (value_type *)(mkl_malloc(
                                      sizeof(value_type) * block_a_size, 64));
  value_type * buf_block_b_comp   = (value_type *)(mkl_malloc(
                                      sizeof(value_type) * block_b_size, 64));
#else
  value_type * buf_block_a_get    = new value_type[block_a_size];
  value_type * buf_block_b_get    = new value_type[block_b_size];
  value_type * buf_block_a_comp   = new value_type[block_a_size];
  value_type * buf_block_b_comp   = new value_type[block_b_size];
#endif
  // Copy of buffer pointers for swapping, delete[] on swapped pointers tends
  // to crash:
  value_type * local_block_a_get      = buf_block_a_get;
  value_type * local_block_b_get      = buf_block_b_get;
  value_type * local_block_a_comp     = buf_block_a_comp;
  value_type * local_block_b_comp     = buf_block_b_comp;
  value_type * local_block_a_get_bac  = nullptr;
  value_type * local_block_b_get_bac  = nullptr;
  value_type * local_block_a_comp_bac = nullptr;
  value_type * local_block_b_comp_bac = nullptr;
  // -------------------------------------------------------------------------
  // Prefetch blocks from A and B for first local multiplication:
  // -------------------------------------------------------------------------
  // Block coordinates of submatrices of A and B to be prefetched:
  // Local block index of local submatrix of C for multiplication result of
  // blocks to be prefetched:
  auto     l_block_c_get       = C.local.block(0);
  auto     l_block_c_get_view  = l_block_c_get.begin().viewspec();
  index_t  l_block_c_get_row   = l_block_c_get_view.offset(1) / block_size_n;
  index_t  l_block_c_get_col   = l_block_c_get_view.offset(0) / block_size_p;
  // Block coordinates of blocks in A and B to prefetch:
  coords_t block_a_get_coords = coords_t {{ static_cast<index_t>(unit_ts_coords[0]),
				   l_block_c_get_row }};
  coords_t block_b_get_coords = coords_t {{ l_block_c_get_col,
				   static_cast<index_t>(unit_ts_coords[0]) }};
  // Local block index of local submatrix of C for multiplication result of
  // currently prefetched blocks:
  auto     l_block_c_comp      = l_block_c_get;
  auto     l_block_c_comp_view = l_block_c_comp.begin().viewspec();
  index_t  l_block_c_comp_row  = l_block_c_comp_view.offset(1) / block_size_n;
  index_t  l_block_c_comp_col  = l_block_c_comp_view.offset(0) / block_size_p;
  // Prefetch blocks from A and B for computation in next iteration:
  dash::Future<value_type *> get_a;
  dash::Future<value_type *> get_b;
  auto block_a      = A.block(block_a_get_coords);
  auto block_a_lptr = block_a.begin().local();
  auto block_b      = B.block(block_b_get_coords);
  auto block_b_lptr = block_b.begin().local();
  DASH_LOG_TRACE("dash::summa", "summa.prefetch.block.a",
                 "block:", block_a_get_coords,
                 "local:", block_a_lptr != nullptr,
                 "unit:",  block_a.begin().lpos().unit,
                 "view:",  block_a.begin().viewspec());

  dash::util::Trace trace("SUMMA");

  trace.enter_state("prefetch");
  if (block_a_lptr == nullptr) {
#ifdef DASH_ALGORITHM_SUMMA_ASYNC_INIT_PREFETCH
    get_a = dash::copy_async(block_a.begin(), block_a.end(),
                             local_block_a_comp);
#else
    dash::copy(block_a.begin(), block_a.end(),
               local_block_a_comp);
    get_a = dash::Future<value_type *>(
              [=]() { return local_block_a_comp + block_a.size(); });
#endif
  } else {
    local_block_a_comp_bac = local_block_a_comp;
    local_block_a_comp     = block_a_lptr;
  }

  DASH_LOG_TRACE("dash::summa", "summa.prefetch.block.b",
                 "block:", block_b_get_coords,
                 "local:", block_b_lptr != nullptr,
                 "unit:",  block_b.begin().lpos().unit,
                 "view:",  block_b.begin().viewspec());
  if (block_b_lptr == nullptr) {
#ifdef DASH_ALGORITHM_SUMMA_ASYNC_INIT_PREFETCH
    get_b = dash::copy_async(block_b.begin(), block_b.end(),
                             local_block_b_comp);
#else
    dash::copy(block_b.begin(), block_b.end(),
               local_block_b_comp);
    get_b = dash::Future<value_type *>(
              [=]() { return local_block_b_comp + block_b.size(); });
#endif
  } else {
    local_block_b_comp_bac = local_block_b_comp;
    local_block_b_comp     = block_b_lptr;
  }
#ifdef DASH_ALGORITHM_SUMMA_ASYNC_INIT_PREFETCH
  if (block_a_lptr == nullptr) {
    DASH_LOG_TRACE("dash::summa", "summa.prefetch.block.a.wait",
                   "waiting for prefetching of block A from unit",
                   block_a.begin().lpos().unit);
    get_a.wait();
  }
  if (block_b_lptr == nullptr) {
    DASH_LOG_TRACE("dash::summa", "summa.prefetch.block.b.wait",
                   "waiting for prefetching of block B from unit",
                   block_b.begin().lpos().unit);
    get_b.wait();
  }
#endif
  trace.exit_state("prefetch");

  DASH_LOG_TRACE("dash::summa", "summa.block",
                 "prefetching of blocks completed");
  // -------------------------------------------------------------------------
  // Iterate local blocks in matrix C:
  // -------------------------------------------------------------------------
  extent_t num_local_blocks_c = pattern_c.local_blockspec().size();

  DASH_LOG_TRACE("dash::summa", "summa.block.C",
                 "C.num.local.blocks:",  num_local_blocks_c,
                 "C.num.column.blocks:", num_blocks_m);

  for (extent_t lb = 0; lb < num_local_blocks_c; ++lb) {
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
    for (extent_t block_k = 0; block_k < num_blocks_m; ++block_k) {
      DASH_LOG_TRACE("dash::summa", "summa.block.k", block_k,
                     "active local block in C:", lb);

      // ---------------------------------------------------------------------
      // Prefetch local copy of blocks from A and B for multiplication in
      // next iteration.
      // ---------------------------------------------------------------------
      bool last = (lb == num_local_blocks_c - 1) &&
                  (block_k == num_blocks_m - 1);
      // Do not prefetch blocks in last iteration:
      if (!last) {
        index_t block_get_k = static_cast<index_t>(block_k + 1);
        block_get_k = (block_get_k + unit_ts_coords[0]) % num_blocks_m;
        // Block coordinate of local block in matrix C to prefetch:
        if (block_k == num_blocks_m - 1) {
          // Prefetch for next local block in matrix C:
          block_get_k        = unit_ts_coords[0];
          l_block_c_get      = C.local.block(lb + 1);
          l_block_c_get_view = l_block_c_get.begin().viewspec();
          l_block_c_get_row  = l_block_c_get_view.offset(1) / block_size_n;
          l_block_c_get_col  = l_block_c_get_view.offset(0) / block_size_p;
        }
        // Block coordinates of blocks in A and B to prefetch:
        block_a_get_coords = coords_t {{ block_get_k, l_block_c_get_row }};
        block_b_get_coords = coords_t {{ l_block_c_get_col, block_get_k }};

        block_a      = A.block(block_a_get_coords);
        block_a_lptr = block_a.begin().local();
        block_b      = B.block(block_b_get_coords);
        block_b_lptr = block_b.begin().local();
        DASH_LOG_TRACE("dash::summa", "summa.prefetch.block.a",
                       "block:", block_a_get_coords,
                       "local:", block_a_lptr != nullptr,
                       "unit:",  block_a.begin().lpos().unit,
                       "view:",  block_a.begin().viewspec());
        if (block_a_lptr == nullptr) {
          get_a = dash::copy_async(block_a.begin(),
                                   block_a.end(),
                                   local_block_a_get);
          local_block_a_get_bac = nullptr;
        } else {
          local_block_a_get_bac = local_block_a_get;
          local_block_a_get     = block_a_lptr;
        }
        DASH_LOG_TRACE("dash::summa", "summa.prefetch.block.b",
                       "block:", block_b_get_coords,
                       "local:", block_b_lptr != nullptr,
                       "unit:",  block_b.begin().lpos().unit,
                       "view:",  block_b.begin().viewspec());
        if (block_b_lptr == nullptr) {
          get_b = dash::copy_async(block_b.begin(),
                                   block_b.end(),
                                   local_block_b_get);
          local_block_b_get_bac = nullptr;
        } else {
          local_block_b_get_bac = local_block_b_get;
          local_block_b_get     = block_b_lptr;
        }
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

      trace.enter_state("multiply");
      dash::internal::mmult_local<value_type>(
          local_block_a_comp,
          local_block_b_comp,
          l_block_c_comp.begin().local(),
          block_size_m,
          block_size_n,
          block_size_p,
          memory_order);
      trace.exit_state("multiply");

      if (local_block_a_comp_bac != nullptr) {
        local_block_a_comp     = local_block_a_comp_bac;
        local_block_a_comp_bac = nullptr;
      }
      if (local_block_b_comp_bac != nullptr) {
        local_block_b_comp     = local_block_b_comp_bac;
        local_block_b_comp_bac = nullptr;
      }
      if (!last) {
        // -------------------------------------------------------------------
        // Wait for local copies:
        // -------------------------------------------------------------------
        trace.enter_state("prefetch");
        if (block_a_lptr == nullptr) {
          DASH_LOG_TRACE("dash::summa", "summa.prefetch.block.a.wait",
                         "waiting for prefetching of block A from unit",
                         block_a.begin().lpos().unit);
          get_a.wait();
        }
        if (block_b_lptr == nullptr) {
          DASH_LOG_TRACE("dash::summa", "summa.prefetch.block.b.wait",
                         "waiting for prefetching of block B from unit",
                         block_b.begin().lpos().unit);
          get_b.wait();
        }
        DASH_LOG_TRACE("dash::summa", "summa.prefetch.completed",
                       "local copies of next blocks received");
        trace.exit_state("prefetch");

        // -----------------------------------------------------------------
        // Swap communication and computation buffers:
        // -----------------------------------------------------------------
        std::swap(local_block_a_get, local_block_a_comp);
        std::swap(local_block_b_get, local_block_b_comp);
        if (local_block_a_get_bac != nullptr) {
          local_block_a_comp_bac = local_block_a_get_bac;
          local_block_a_get_bac  = nullptr;
        }
        if (local_block_b_get_bac != nullptr) {
          local_block_b_comp_bac = local_block_b_get_bac;
          local_block_b_get_bac  = nullptr;
        }
      }
    }
  } // for lb

  DASH_LOG_TRACE("dash::summa", "locally completed");
#ifdef DASH_ENABLE_MKL
  mkl_free(buf_block_a_get);
  mkl_free(buf_block_b_get);
  mkl_free(buf_block_a_comp);
  mkl_free(buf_block_b_comp);
#else
  delete[] buf_block_a_get;
  delete[] buf_block_b_get;
  delete[] buf_block_a_comp;
  delete[] buf_block_b_comp;
#endif

  DASH_LOG_TRACE("dash::summa", "waiting for other units");
  trace.enter_state("barrier");
  C.barrier();
  trace.exit_state("barrier");

  DASH_LOG_TRACE("dash::summa >", "finished");
}

#ifdef DOXYGEN
/**
 * Function adapter to an implementation of matrix-matrix multiplication
 * (xDGEMM) depending on the matrix distribution patterns.
 *
 * Delegates  \c dash::mmult<MatrixType>
 * to         \c dash::summa<MatrixType>
 * if         \c MatrixType::pattern_type
 * satisfies the pattern property constraints of the SUMMA implementation.
 */
template <
  typename MatrixTypeA,
  typename MatrixTypeB,
  typename MatrixTypeC >
void mmult(
  /// Matrix to multiply, extents n x m
  MatrixTypeA & A,
  /// Matrix to multiply, extents m x p
  MatrixTypeB & B,
  /// Matrix to contain the multiplication result, extents n x p,
  /// initialized with zeros
  MatrixTypeC & C);

#else // DOXYGEN

template<
  typename MatrixTypeA,
  typename MatrixTypeB,
  typename MatrixTypeC
>
typename
std::enable_if<
 summa_pattern_constraints<MatrixTypeA>::satisfied::value &&
 summa_pattern_constraints<MatrixTypeB>::satisfied::value &&
 summa_pattern_constraints<MatrixTypeC>::satisfied::value,
 void>
mmult(
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

#endif // DOXYGEN

} // namespace dash

#endif // DASH__ALGORITHM__SUMMA_H_
