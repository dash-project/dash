#ifndef DASH__MATRIX__MATRIX_REF_H__
#define DASH__MATRIX__MATRIX_REF_H__

#include <dash/Matrix.h>

#include <dash/dart/if/dart.h>

#include <dash/Team.h>
#include <dash/Pattern.h>
#include <dash/GlobRef.h>
#include <dash/HView.h>
#include <dash/Container.h>

#include <dash/iterator/GlobIter.h>
#include <dash/iterator/GlobViewIter.h>


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
  dim_t NumViewDim,
  class PatternT >
class MatrixRef;
/// Forward-declaration
template <
  typename T,
  dim_t NumDimensions,
  dim_t NumViewDim,
  class PatternT >
class LocalMatrixRef;

/**
 * A view on a referenced \c Matrix object, such as a dimensional
 * projection returned by \c Matrix::sub.
 *
 * TODO:
 * Projection order matrix.sub().local() is not fully implemented yet.
 * Currently only matrix.local().sub() is supported.
 *
 * \see DashMatrixConcept
 *
 * \ingroup Matrix
 */
template <
  typename ElementT,
  dim_t NumDimensions,
  dim_t NumViewDim = NumDimensions,
  class PatternT =
    TilePattern<NumDimensions, ROW_MAJOR, dash::default_index_t> >
class MatrixRef
{
 private:
  typedef MatrixRef<ElementT, NumDimensions, NumViewDim, PatternT>
    self_t;
  typedef PatternT
    Pattern_t;
  typedef typename PatternT::index_type
    Index_t;
  typedef typename PatternT::size_type
    Size_t;
  typedef MatrixRefView<ElementT, NumDimensions, PatternT>
    MatrixRefView_t;
  typedef LocalMatrixRef<ElementT, NumDimensions, NumDimensions, PatternT>
    LocalRef_t;
  typedef GlobIter<ElementT, PatternT>
    GlobIter_t;
  typedef GlobViewIter<ElementT, PatternT>
    GlobViewIter_t;
  typedef CartesianIndexSpace<
            NumViewDim,
            PatternT::memory_order(),
            typename PatternT::index_type >
    IndexSpace_t;
  typedef std::array<typename PatternT::size_type, NumDimensions>
    Extents_t;

 public:
  typedef PatternT                                        pattern_type;
  typedef typename PatternT::index_type                     index_type;
  typedef ElementT                                          value_type;
  typedef IndexSpace_t                                index_space_type;

  typedef typename PatternT::size_type                       size_type;
  typedef typename PatternT::index_type                difference_type;

  typedef GlobViewIter_t                                      iterator;
  typedef const GlobViewIter_t                          const_iterator;
  typedef std::reverse_iterator<iterator>             reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

  typedef GlobRef<value_type>                                reference;
  typedef const GlobRef<value_type>                    const_reference;

  typedef GlobViewIter_t                                       pointer;
  typedef const GlobViewIter_t                           const_pointer;

  typedef LocalRef_t                                        local_type;
  typedef const LocalRef_t                            const_local_type;
  typedef LocalRef_t                              local_reference_type;
  typedef const LocalRef_t                  const_local_reference_type;

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
  friend class MatrixRef;
  template<
    typename T_,
    dim_t NumDimensions1,
    dim_t NumDimensions2,
    class PatternT_ >
  friend class LocalMatrixRef;

  inline operator
    MatrixRef<ElementT, NumDimensions, NumViewDim-1, PatternT> && ();

public:

  static constexpr dim_t ndim() {
    return NumViewDim;
  }

public:

  MatrixRef<ElementT, NumDimensions, NumViewDim, PatternT>()
  {
    DASH_LOG_TRACE_VAR("MatrixRef<T,D,C>()", NumDimensions);
    DASH_LOG_TRACE_VAR("MatrixRef<T,D,C>()", NumViewDim);
  }

  MatrixRef<ElementT, NumDimensions, NumViewDim, PatternT>(
    const MatrixRef<ElementT, NumDimensions, NumViewDim+1, PatternT> & prev,
    index_type coord);

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

  /// View representing elements in the active unit's local memory.
  inline    local_type        sub_local()                 noexcept;
  /// Pointer to first element in local range.
  inline    ElementT        * lbegin()                    noexcept;
  /// Pointer past final element in local range.
  inline    ElementT        * lend()                      noexcept;

  /**
   * Subscript operator, returns a submatrix reference at given offset
   * in global element range.
   */
  MatrixRef<ElementT, NumDimensions, NumViewDim-1, PatternT>
    operator[](index_type n);

  /**
   * Subscript operator, returns a submatrix reference at given offset
   * in global element range.
   */
  const MatrixRef<ElementT, NumDimensions, NumViewDim-1, PatternT>
    operator[](index_type n) const;

  template<dim_t NumSubDimensions>
  MatrixRef<ElementT, NumDimensions, NumDimensions-1, PatternT>
  sub(size_type n);

  MatrixRef<ElementT, NumDimensions, NumDimensions-1, PatternT>
  col(size_type n);

  MatrixRef<ElementT, NumDimensions, NumDimensions-1, PatternT>
  row(size_type n);

  template<dim_t SubDimension>
  MatrixRef<ElementT, NumDimensions, NumDimensions, PatternT>
  sub(
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
  MatrixRef<ElementT, NumDimensions, NumDimensions, PatternT>
  rows(
    /// Offset of first row in range
    size_type n,
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
  MatrixRef<ElementT, NumDimensions, NumDimensions, PatternT>
  cols(
    /// Offset of first column in range
    size_type offset,
    /// Number of columns in the range
    size_type range);

  /**
   * Fortran-style subscript operator.
   * As an example, the operation \c matrix(i,j) is equivalent to
   * \c matrix[i][j].
   *
   * \returns  A global reference to the element at the given global
   *           coordinates.
   */
  template<typename ... Args>
  reference at(
    /// Global coordinates
    Args... args);

  /**
   * Fortran-style subscript operator.
   * As an example, the operation \c matrix(i,j) is equivalent to
   * \c matrix[i][j].
   *
   * \returns  A global reference to the element at the given global
   *           coordinates.
   */
  reference at(
    /// Global coordinates
    const ::std::array<index_type, NumDimensions> & coords);

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
  reference operator()(
    /// Global coordinates
    Args... args);

  inline bool is_local(index_type n) const;

  template<dim_t Dimension>
  inline bool is_local(index_type n) const;

  inline const ViewSpec<NumDimensions, index_type> & viewspec() const {
    return _refview._viewspec;
  }

  template <int level>
  dash::HView<Matrix<ElementT, NumDimensions, Index_t, PatternT>, level>
  inline hview();

 private:
  MatrixRefView<ElementT, NumDimensions, PatternT> _refview;
};

/**
 * A view on a referenced \c Matrix object, such as a dimensional
 * projection returned by \c Matrix::sub.
 * Partial Specialization for value deferencing.
 *
 * \ingroup Matrix
 */
template <
  typename ElementT,
  dim_t NumDimensions,
  class PatternT >
class MatrixRef< ElementT, NumDimensions, 0, PatternT >
{
 private:
   typedef MatrixRef<ElementT, NumDimensions, 0, PatternT> self_t;

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
  friend class MatrixRef;

 public:
  typedef PatternT                       pattern_type;
  typedef typename PatternT::index_type  index_type;
  typedef ElementT                       value_type;

  /**
   * Default constructor.
   */
  MatrixRef<ElementT, NumDimensions, 0, PatternT>()
  {
    DASH_LOG_TRACE_VAR("MatrixRef<T,D,0>()", NumDimensions);
  }

  /**
   * Copy constructor.
   */
  MatrixRef<ElementT, NumDimensions, 0, PatternT>(
    const self_t & other)
  : _refview(other._refview) {
    DASH_LOG_TRACE_VAR("MatrixRef<T,D,0>(other)", NumDimensions);
  }

  MatrixRef<ElementT, NumDimensions, 0, PatternT>(
    const MatrixRef<ElementT, NumDimensions, 1, PatternT> & previous,
    index_type coord);

  /**
   * TODO[TF] The following two functions don't seem to be implemented.
   */
  inline const GlobRef<ElementT> local_at(
    team_unit_t unit,
    index_type   elem) const;

  inline GlobRef<ElementT> local_at(
    team_unit_t unit,
    index_type   elem);

  inline bool is_local() const;

  inline const ViewSpec<NumDimensions, index_type> & viewspec() const {
    return _refview._viewspec;
  }

  operator ElementT() const;
  operator GlobPtr<ElementT, PatternT>() const;

  /**
   * Assignment operator.
   */
  inline ElementT operator= (const ElementT & value);
  inline ElementT operator+=(const ElementT & value);
  inline ElementT operator+ (const ElementT & value);
  inline ElementT operator-=(const ElementT & value);
  inline ElementT operator- (const ElementT & value);
  inline ElementT operator*=(const ElementT & value);
  inline ElementT operator* (const ElementT & value);
  inline ElementT operator/=(const ElementT & value);
  inline ElementT operator/ (const ElementT & value);

 private:
  MatrixRefView<ElementT, NumDimensions, PatternT> _refview;
};

} // namespace dash

#include <dash/matrix/internal/MatrixRef-inl.h>

#endif // DASH__MATRIX__MATRIX_REF_H__
