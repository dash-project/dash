#ifndef DASH__MATRIX__LOCAL_MATRIX_REF_H__
#define DASH__MATRIX__LOCAL_MATRIX_REF_H__

#include <dash/dart/if/dart.h>

#include <dash/Team.h>
#include <dash/Pattern.h>
#include <dash/GlobIter.h>
#include <dash/GlobRef.h>
#include <dash/HView.h>
#include <dash/Container.h>

#include <dash/Matrix.h>


namespace dash {

/// Forward-declaration
template <
  typename T,
  dim_t NumDimensions,
  typename IndexT,
  class PatternT >
class Matrix;
/// Forward-declaration
template <
  typename T,
  dim_t NumDimensions,
  dim_t CUR,
  class PatternT >
class MatrixRef;
/// Forward-declaration
template <
  typename T,
  dim_t NumDimensions,
  dim_t CUR,
  class PatternT >
class LocalMatrixRef;

/**
 * Local part of a Matrix, provides local operations.
 *
 * \see DashMatrixConcept
 *
 * \ingroup Matrix
 */
template <
  typename T,
  dim_t NumDimensions,
  dim_t CUR = NumDimensions,
  class PatternT =
    TilePattern<NumDimensions, ROW_MAJOR, dash::default_index_t> >
class LocalMatrixRef
{
private:
  typedef MatrixRefView<T, NumDimensions, PatternT>
    MatrixRefView_t;
  typedef std::array<typename PatternT::size_type, NumDimensions>
    Extents_t;
  template <dim_t NumViewDim>
    using LocalMatrixRef_t =
          LocalMatrixRef<T, NumDimensions, NumViewDim, PatternT>;

public:
  template<
    typename T_,
    dim_t NumDimensions_,
    typename IndexT_,
    class PatternT_ >
  friend class Matrix;
  template<
    typename T_,
    dim_t NumDimensions1,
    dim_t NumDimensions2,
    class PatternT_ >
  friend class LocalMatrixRef;

public:
  typedef T                                                   value_type;
  typedef PatternT                                          pattern_type;
  typedef typename PatternT::index_type                       index_type;

  typedef typename PatternT::size_type                         size_type;
  typedef typename PatternT::index_type                  difference_type;

  typedef       GlobIter<value_type, PatternT>                  iterator;
  typedef const GlobIter<value_type, PatternT>            const_iterator;
  typedef std::reverse_iterator<iterator>               reverse_iterator;
  typedef std::reverse_iterator<const_iterator>   const_reverse_iterator;

  typedef       GlobRef<value_type>                            reference;
  typedef const GlobRef<value_type>                      const_reference;

  typedef       GlobIter<value_type, PatternT>                   pointer;
  typedef const GlobIter<value_type, PatternT>             const_pointer;

  template <dim_t NumViewDim>
    using View = LocalMatrixRef<T, NumDimensions, NumViewDim, PatternT>;

public:
  /**
   * Default constructor.
   */
  LocalMatrixRef<T, NumDimensions, CUR, PatternT>()
  : _refview(nullptr)
  {
    DASH_LOG_TRACE_VAR("LocalMatrixRef<T,D,C>()", NumDimensions);
    DASH_LOG_TRACE_VAR("LocalMatrixRef<T,D,C>()", CUR);
  }

  LocalMatrixRef<T, NumDimensions, CUR, PatternT>(
    const LocalMatrixRef<T, NumDimensions, CUR+1, PatternT> & previous,
    index_type coord);

#if 0
  /**
   * Constructor, creates a local view reference to a Matrix.
   */
  LocalMatrixRef<T, NumDimensions, CUR, PatternT>(
    MatrixRef<T, NumDimensions, CUR, PatternT> * matref);
#endif

  /**
   * Constructor, creates a local view reference to a Matrix view.
   */
  LocalMatrixRef<T, NumDimensions, CUR, PatternT>(
    Matrix<T, NumDimensions, index_type, PatternT> * mat);

  /**
   * View at local block at given local block coordinates.
   */
  LocalMatrixRef_t<NumDimensions> block(
    const std::array<index_type, NumDimensions> & block_lcoords)
  {
    // Note: This is equivalent to
    //   foreach (d in 0 ... NumDimensions):
    //     view = view.sub<d>(block_view.offset(d),
    //                        block_view.extent(d));
    //
    DASH_LOG_TRACE("LocalMatrixRef.block()", block_lcoords);
    auto pattern      = _refview->_mat->_pattern;
    auto block_lindex = pattern.blockspec().at(block_lcoords);
    DASH_LOG_TRACE("LocalMatrixRef.block()", block_lindex);
    // Global view of local block:
    auto l_block_g_view = pattern.local_block(block_lindex);
    // Local view of local block:
    auto l_block_l_view = pattern.local_block_local(block_lindex);
    // Return a view specified by the block's viewspec:
    LocalMatrixRef_t<NumDimensions> view;
    view._refview              = new MatrixRefView_t(_refview->_mat);
    view._refview->_viewspec   = l_block_g_view;
    view._refview->_l_viewspec = l_block_l_view;
    DASH_LOG_TRACE("LocalMatrixRef.block >",
                   "global:",
                   "offsets:", view._refview->_viewspec.offsets(),
                   "extents:", view._refview->_viewspec.extents(),
                   "local:",
                   "offsets:", view._refview->_l_viewspec.offsets(),
                   "extents:", view._refview->_l_viewspec.extents());
    return view;
  }

  /**
   * View at local block at given local block offset.
   */
  LocalMatrixRef_t<NumDimensions> block(
    index_type block_lindex)
  {
    // Note: This is equivalent to
    //   foreach (d in 0 ... NumDimensions):
    //     view = view.sub<d>(block_view.offset(d),
    //                        block_view.extent(d));
    //
    DASH_LOG_TRACE("LocalMatrixRef.block()", block_lindex);
    auto pattern      = _refview->_mat->_pattern;
    // Global view of local block:
    auto l_block_g_view = pattern.local_block(block_lindex);
    // Local view of local block:
    auto l_block_l_view = pattern.local_block_local(block_lindex);
    // Return a view specified by the block's viewspec:
    LocalMatrixRef_t<NumDimensions> view;
    view._refview              = new MatrixRefView_t(_refview->_mat);
    view._refview->_viewspec   = l_block_g_view;
    view._refview->_l_viewspec = l_block_l_view;
    DASH_LOG_TRACE("LocalMatrixRef.block >",
                   "global:",
                   "offsets:", view._refview->_viewspec.offsets(),
                   "extents:", view._refview->_viewspec.extents(),
                   "local:",
                   "offsets:", view._refview->_l_viewspec.offsets(),
                   "extents:", view._refview->_l_viewspec.extents());
    return view;
  }

  inline    operator LocalMatrixRef<T, NumDimensions, CUR-1, PatternT> && ();
  // SHOULD avoid cast from MatrixRef to LocalMatrixRef.
  // Different operation semantics.
  inline    operator MatrixRef<T, NumDimensions, CUR, PatternT> ();

  inline    T & local_at(size_type pos);

  inline    Team            & team();

  inline    size_type         size()                const noexcept;
  inline    size_type         local_size()          const noexcept;
  inline    size_type         local_capacity()      const noexcept;
  inline    size_type         extent(dim_t dim)     const noexcept;
  inline    Extents_t         extents()             const noexcept;
  inline    bool              empty()               const noexcept;

  inline    void              barrier()             const;

  /**
   * The pattern used to distribute matrix elements to units in its
   * associated team.
   */
  inline    const PatternT  & pattern()             const;

  inline    const_pointer     data()                const noexcept;
  inline    iterator          begin()                     noexcept;
  inline    const_iterator    begin()               const noexcept;
  inline    iterator          end()                       noexcept;
  inline    const_iterator    end()                 const noexcept;

  /**
   * Fortran-style subscript operator.
   * As an example, the operation \c matrix(i,j) is equivalent to
   * \c matrix[i][j].
   *
   * \returns  A global reference to the element at the given global
   *           coordinates.
   */
  template<typename ... Args>
  T & at(
    /// Global coordinates
    Args... args);

  /**
   * Fortran-style subscript operator, alias for \c at().
   * As an example, the operation \c matrix(i,j) is equivalent to
   * \c matrix[i][j].
   *
   * \returns  A global reference to the element at the given global
   *           coordinates.
   * \see  at
   */
  template<typename... Args>
  T & operator()(
    /// Global coordinates
    Args... args);

  /**
   * Subscript assignment operator, access element at given offset
   * in global element range.
   */
  LocalMatrixRef<T, NumDimensions, CUR-1, PatternT>
    operator[](index_type n);

  /**
   * Subscript operator, access element at given offset in
   * global element range.
   */
  const LocalMatrixRef<T, NumDimensions, CUR-1, PatternT>
    operator[](index_type n) const;

  template<dim_t NumSubDimensions>
  LocalMatrixRef<T, NumDimensions, NumDimensions-1, PatternT>
    sub(size_type n);
  inline LocalMatrixRef<T, NumDimensions, NumDimensions-1, PatternT> 
    col(size_type n);
  inline LocalMatrixRef<T, NumDimensions, NumDimensions-1, PatternT> 
    row(size_type n);

  template<dim_t SubDimension>
  LocalMatrixRef<T, NumDimensions, NumDimensions, PatternT> sub(
    size_type n,
    size_type range);

  /**
   * Create a view representing the matrix slice within a row
   * range.
   * Same as \c sub<0>(offset, extent).
   *
   * \returns  A matrix view
   *
   * \see  sub
   */
  inline LocalMatrixRef<T, NumDimensions, NumDimensions, PatternT> rows(
    /// Offset of first row in range
    size_type offset,
    /// Number of rows in the range
    size_type range);

  /**
   * Create a view representing the matrix slice within a column
   * range.
   * Same as \c sub<1>(offset, extent).
   *
   * \returns  A matrix view
   *
   * \see  sub
   */
  inline LocalMatrixRef<T, NumDimensions, NumDimensions, PatternT> cols(
    /// Offset of first column in range
    size_type offset,
    /// Number of columns in the range
    size_type extent);

private:
  MatrixRefView_t * _refview;
};

/**
 * Local Matrix representation, provides local operations.
 * Partial Specialization for value deferencing.
 *
 * \ingroup Matrix
 */
template <
  typename T,
  dim_t NumDimensions,
  class PatternT >
class LocalMatrixRef<T, NumDimensions, 0, PatternT>
{
 private:
   typedef LocalMatrixRef<T, NumDimensions, 0, PatternT> self_t;

 public:
  template<
    typename T_,
    dim_t NumDimensions_,
    typename IndexT_,
    class PatternT_ >
  friend class Matrix;
  template<
    typename T_,
    dim_t NumDimensions1,
    dim_t NumDimensions2,
    class PatternT_ > 
  friend class LocalMatrixRef;

 public:
  typedef typename PatternT::index_type  index_type;
  typedef typename PatternT::size_type   size_type;

 public:
  /**
   * Default constructor.
   */
  LocalMatrixRef<T, NumDimensions, 0, PatternT>()
  : _refview(nullptr) {
    DASH_LOG_TRACE_VAR("LocalMatrixRef<T,D,0>()", NumDimensions);
  }

  /**
   * Copy constructor.
   */
  LocalMatrixRef<T, NumDimensions, 0, PatternT>(
    const self_t & other)
  : _refview(other._refview) {
    DASH_LOG_TRACE_VAR("LocalMatrixRef<T,D,0>(other)", NumDimensions);
  }

  LocalMatrixRef<T, NumDimensions, 0, PatternT>(
    const LocalMatrixRef<T, NumDimensions, 1, PatternT> & previous,
    index_type coord);

  inline T * local_at(index_type pos);
  inline operator T();
  /**
   * Assignment operator.
   */
  inline T operator= (const T & value);
  inline T operator+=(const T & value);
  inline T operator+ (const T & value);
  inline T operator-=(const T & value);
  inline T operator- (const T & value);
  inline T operator*=(const T & value);
  inline T operator* (const T & value);
  inline T operator/=(const T & value);
  inline T operator/ (const T & value);

 private:
  MatrixRefView<T, NumDimensions, PatternT> * _refview;
};

} // namespace dash

#include <dash/matrix/internal/LocalMatrixRef-inl.h>

#endif
