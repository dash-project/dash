#ifndef DASH__MATRIX__LOCAL_MATRIX_REF_H__
#define DASH__MATRIX__LOCAL_MATRIX_REF_H__

#include <dash/dart/if/dart.h>

#include <dash/Team.h>
#include <dash/Pattern.h>
#include <dash/GlobRef.h>
#include <dash/HView.h>
#include <dash/Container.h>

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
  typedef T                                                        value_type;
  typedef PatternT                                               pattern_type;
  typedef typename PatternT::index_type                            index_type;

  typedef typename PatternT::size_type                              size_type;
  typedef typename PatternT::index_type                       difference_type;

  typedef       GlobViewIter<value_type, PatternT>                   iterator;
  typedef const GlobViewIter<value_type, PatternT>             const_iterator;
  typedef std::reverse_iterator<iterator>                    reverse_iterator;
  typedef std::reverse_iterator<const_iterator>        const_reverse_iterator;

  typedef       GlobRef<value_type>                                 reference;
  typedef const GlobRef<value_type>                           const_reference;

  typedef       GlobViewIter<value_type, PatternT>                    pointer;
  typedef const GlobViewIter<value_type, PatternT>              const_pointer;

  typedef       T *                                             local_pointer;
  typedef const T *                                       const_local_pointer;

  template <dim_t NumViewDim>
    using view_type =
          LocalMatrixRef<T, NumDimensions, NumViewDim, PatternT>;

public:
  typedef std::integral_constant<dim_t, CUR>
    rank;

  static constexpr dim_t ndim() {
    return CUR;
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
    Matrix<T, NumDimensions, index_type, PatternT> * mat
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

  inline operator LocalMatrixRef<T, NumDimensions, CUR-1, PatternT> && ();

  // SHOULD avoid cast from MatrixRef to LocalMatrixRef.
  // Different operation semantics.
  inline operator MatrixRef<T, NumDimensions, CUR, PatternT> ();

  inline T                 & local_at(size_type pos);

  inline Team              & team();

  inline size_type           size()                const noexcept;
  inline size_type           local_size()          const noexcept;
  inline size_type           local_capacity()      const noexcept;
  inline size_type           extent(dim_t dim)     const noexcept;
  inline Extents_t           extents()             const noexcept;
  inline index_type          offset(dim_t dim)     const noexcept;
  inline Offsets_t           offsets()             const noexcept;
  inline bool                empty()               const noexcept;

  /**
   * Synchronize units associated with the matrix.
   *
   * \see  DashContainerConcept
   */
  inline void                barrier()             const;

  /**
   * The pattern used to distribute matrix elements to units in its
   * associated team.
   *
   * NOTE: This method is not implemented as local matrix views do
   *       not have a pattern. The pattern of the referenced matrix
   *       refers to the global data domain.
   */
  inline const PatternT    & pattern()             const;

  inline       iterator      begin()                     noexcept;
  inline const_iterator      begin()               const noexcept;
  inline       iterator      end()                       noexcept;
  inline const_iterator      end()                 const noexcept;

  inline       local_pointer lbegin()                    noexcept;
  inline const_local_pointer lbegin()              const noexcept;
  inline       local_pointer lend()                      noexcept;
  inline const_local_pointer lend()                const noexcept;

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
   * \returns  A matrix local view
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
   * \returns  A matrix local view
   *
   * \see  sub
   */
  inline LocalMatrixRef<T, NumDimensions, NumDimensions, PatternT> cols(
    /// Offset of first column in range
    size_type offset,
    /// Number of columns in the range
    size_type extent);

private:
  MatrixRefView_t _refview;
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
  {
    DASH_LOG_TRACE_VAR("LocalMatrixRef<T,D,0>()", NumDimensions);
  }

  /**
   * Copy constructor.
   */
  LocalMatrixRef<T, NumDimensions, 0, PatternT>(
    const self_t & other)
  : _refview(other._refview)
  {
    DASH_LOG_TRACE_VAR("LocalMatrixRef<T,D,0>(other)", NumDimensions);
  }

  LocalMatrixRef<T, NumDimensions, 0, PatternT>(
    const LocalMatrixRef<T, NumDimensions, 1, PatternT> & previous,
    index_type coord);

  inline T * local_at(index_type pos) const;

  inline bool is_local() const {
    return true;
  }

  inline operator T() const;

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
  MatrixRefView<T, NumDimensions, PatternT> _refview;
};

} // namespace dash

#include <dash/matrix/internal/LocalMatrixRef-inl.h>

#endif
