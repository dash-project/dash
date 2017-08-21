#ifndef DASH__MATRIX__LOCAL_MATRIX_REF_H__
#define DASH__MATRIX__LOCAL_MATRIX_REF_H__

#include <dash/dart/if/dart.h>

#include <dash/Team.h>
#include <dash/Pattern.h>
#include <dash/GlobRef.h>
#include <dash/HView.h>

#include <dash/iterator/GlobIter.h>
#include <dash/iterator/GlobViewIter.h>

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
  dim_t    NumDimensions,
  dim_t    CUR      = NumDimensions,
  class    PatternT =
             TilePattern<NumDimensions, ROW_MAJOR, dash::default_index_t> >
class LocalMatrixRef
{
private:
  typedef LocalMatrixRef<T, NumDimensions, CUR, PatternT> self_t;

  typedef MatrixRefView<T, NumDimensions, PatternT>
    MatrixRefView_t;
  typedef std::array<typename PatternT::size_type, NumDimensions>
    Extents_t;
  typedef std::array<typename PatternT::index_type, NumDimensions>
    Offsets_t;
  typedef dash::ViewSpec<NumDimensions,typename PatternT::index_type>
    ViewSpec_t;
  template <dim_t NumViewDim>
    using LocalMatrixRef_t =
          LocalMatrixRef<T, NumDimensions, NumViewDim, PatternT>;

public:
  template<
    typename T_,
    dim_t    NumDimensions_,
    typename IndexT_,
    class    PatternT_ >
  friend class Matrix;
  template<
    typename T_,
    dim_t    NumDimensions1,
    dim_t    NumDimensions2,
    class    PatternT_ >
  friend class LocalMatrixRef;

public:
  typedef T                                                        value_type;
  typedef PatternT                                               pattern_type;
  typedef typename PatternT::index_type                            index_type;

  typedef typename PatternT::size_type                              size_type;
  typedef typename PatternT::index_type                       difference_type;

  typedef GlobViewIter<      value_type, PatternT>                   iterator;
  typedef GlobViewIter<const value_type, PatternT>             const_iterator;

  typedef std::reverse_iterator<iterator>                    reverse_iterator;
  typedef std::reverse_iterator<const_iterator>        const_reverse_iterator;

  typedef GlobRef<      value_type>                                 reference;
  typedef GlobRef<const value_type>                           const_reference;

  typedef GlobViewIter<      value_type, PatternT>                    pointer;
  typedef GlobViewIter<const value_type, PatternT>              const_pointer;

  typedef       T *                                             local_pointer;
  typedef const T *                                       const_local_pointer;

  typedef self_t                                                   local_type;

  template <dim_t NumViewDim>
    using ViewT =
          LocalMatrixRef<T, NumDimensions, NumViewDim, PatternT>;

public:
  typedef std::integral_constant<dim_t, CUR>
    rank;

  static constexpr dim_t ndim() {
    return CUR;
  }

protected:
  LocalMatrixRef<T, NumDimensions, CUR, PatternT>(
    MatrixRefView_t && refview)
  : _refview(std::move(refview))
  {
    DASH_LOG_TRACE_VAR("LocalMatrixRef<T,D,C>()", NumDimensions);
    DASH_LOG_TRACE_VAR("LocalMatrixRef<T,D,C>()", CUR);
  }

  LocalMatrixRef<T, NumDimensions, CUR, PatternT>(
    const MatrixRefView_t & refview)
  : _refview(refview)
  {
    DASH_LOG_TRACE_VAR("LocalMatrixRef<T,D,C>()", NumDimensions);
    DASH_LOG_TRACE_VAR("LocalMatrixRef<T,D,C>()", CUR);
  }

public:
  /**
   * Default constructor.
   */
  LocalMatrixRef<T, NumDimensions, CUR, PatternT>()
  {
    DASH_LOG_TRACE_VAR("LocalMatrixRef<T,D,C>()", NumDimensions);
    DASH_LOG_TRACE_VAR("LocalMatrixRef<T,D,C>()", CUR);
  }

  template <class T_>
  LocalMatrixRef<T, NumDimensions, CUR, PatternT>(
    const LocalMatrixRef<T_, NumDimensions, CUR+1, PatternT> & previous,
    size_type coord);

  /**
   * Constructor, creates a local view reference to a Matrix view.
   */
  template <class T_>
  LocalMatrixRef<T, NumDimensions, CUR, PatternT>(
    Matrix<T_, NumDimensions, index_type, PatternT> * mat
  );

  /**
   * View at local block at given local block coordinates.
   */
  LocalMatrixRef<T, NumDimensions, CUR, PatternT> block(
    const std::array<index_type, NumDimensions> & block_lcoords
  );

  /**
   * View at local block at given local block offset.
   */
  LocalMatrixRef<T, NumDimensions, CUR, PatternT> block(
    index_type block_lindex
  );

  inline operator LocalMatrixRef<T, NumDimensions, CUR-1, PatternT> && () &&;

  // SHOULD avoid cast from MatrixRef to LocalMatrixRef.
  // Different operation semantics.
  inline operator MatrixRef<T, NumDimensions, CUR, PatternT> ();

  /**
   * Returns a reference to the element at local index \c pos.
   * The index is relative to the start of the local range.
   */
  inline    T                   & local_at(size_type pos);
  inline    const T             & local_at(size_type pos) const;

  constexpr Team                & team()                const noexcept;

  constexpr size_type             size()                const noexcept;
  constexpr size_type             local_size()          const noexcept;
  constexpr size_type             local_capacity()      const noexcept;
  inline    size_type             extent(dim_t dim)     const noexcept;
  constexpr Extents_t             extents()             const noexcept;
  inline    index_type            offset(dim_t dim)     const noexcept;
  constexpr Offsets_t             offsets()             const noexcept;
  constexpr bool                  empty()               const noexcept;

  /**
   * The pattern used to distribute matrix elements to units in its
   * associated team.
   *
   * NOTE: This method is not implemented as local matrix views do
   *       not have a pattern. The pattern of the referenced matrix
   *       refers to the global data domain.
   */
  constexpr const PatternT      & pattern()             const;

  inline          iterator        begin()                     noexcept;
  constexpr const_iterator        begin()               const noexcept;
  inline          iterator        end()                       noexcept;
  constexpr const_iterator        end()                 const noexcept;

  inline          local_pointer   lbegin()                    noexcept;
  constexpr const_local_pointer   lbegin()              const noexcept;
  inline          local_pointer   lend()                      noexcept;
  constexpr const_local_pointer   lend()                const noexcept;

  /**
   * Fortran-style subscript operator.
   * As an example, the operation \c matrix(i,j) is equivalent to
   * \c matrix[i][j].
   *
   * \returns  A global reference to the element at the given global
   *           coordinates.
   */
  template<typename ... Args>
  inline T & at(
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
  inline T & operator()(
    /// Coordinates of element in global cartesian index space.
    Args... args);

  /**
   * Subscript assignment operator, access element at given offset
   * in global element range.
   */
  template<dim_t __NumViewDim = CUR-1>
  typename std::enable_if<(__NumViewDim > 0),
    LocalMatrixRef<T, NumDimensions, __NumViewDim, PatternT>>::type
    operator[](size_type n);

  template<dim_t __NumViewDim = CUR-1>
  typename std::enable_if<(__NumViewDim == 0), T&>::type
    operator[](size_type n);

  /**
   * Subscript operator, access element at given offset in
   * global element range.
   */
  template<dim_t __NumViewDim = CUR-1>
  typename std::enable_if<(__NumViewDim > 0),
    LocalMatrixRef<const T, NumDimensions, __NumViewDim, PatternT>>::type
  constexpr operator[](size_type n) const;

  template<dim_t __NumViewDim = CUR-1>
  typename std::enable_if<(__NumViewDim == 0), const T&>::type
  operator[](size_type n) const;

  LocalMatrixRef<T, NumDimensions, NumDimensions-1, PatternT>
    col(size_type n);
  constexpr LocalMatrixRef<const T, NumDimensions, NumDimensions-1, PatternT>
    col(size_type n) const;

  LocalMatrixRef<T, NumDimensions, NumDimensions-1, PatternT>
    row(size_type n);
  constexpr LocalMatrixRef<const T, NumDimensions, NumDimensions-1, PatternT>
    row(size_type n) const;

  template<dim_t NumSubDimensions>
  LocalMatrixRef<T, NumDimensions, NumDimensions-1, PatternT>
    sub(size_type n);

  template<dim_t NumSubDimensions>
  LocalMatrixRef<const T, NumDimensions, NumDimensions-1, PatternT>
    sub(size_type n) const;

  template<dim_t SubDimension>
  LocalMatrixRef<T, NumDimensions, NumDimensions, PatternT>
    sub(size_type n,
        size_type range);

  template<dim_t SubDimension>
  LocalMatrixRef<const T, NumDimensions, NumDimensions, PatternT>
  sub(size_type n,
      size_type range) const;

  /**
   * Create a view representing the matrix slice within a row
   * range.
   * Same as \c sub<0>(offset, extent).
   *
   * \returns  A matrix local view
   *
   * \see  sub
   */
  LocalMatrixRef<T, NumDimensions, NumDimensions, PatternT>
  rows(
    /// Offset of first row in range
    size_type offset,
    /// Number of rows in the range
    size_type range);

  constexpr LocalMatrixRef<const T, NumDimensions, NumDimensions, PatternT>
  rows(
    /// Offset of first row in range
    size_type offset,
    /// Number of rows in the range
    size_type range) const;

  /**
   * Create a view representing the matrix slice within a column
   * range.
   * Same as \c sub<1>(offset, extent).
   *
   * \returns  A matrix local view
   *
   * \see  sub
   */
  LocalMatrixRef<T, NumDimensions, NumDimensions, PatternT>
  cols(
    /// Offset of first column in range
    size_type offset,
    /// Number of columns in the range
    size_type extent);

  constexpr LocalMatrixRef<const T, NumDimensions, NumDimensions, PatternT>
  cols(
    /// Offset of first column in range
    size_type offset,
    /// Number of columns in the range
    size_type extent) const;

private:

  MatrixRefView_t _refview;
};

} // namespace dash

#include <dash/matrix/internal/LocalMatrixRef-inl.h>

#endif
