#ifndef DASH__MATRIX_H_INCLUDED
#define DASH__MATRIX_H_INCLUDED

#include <dash/dart/if/dart.h>

#include <dash/Team.h>
#include <dash/Pattern.h>
#include <dash/GlobIter.h>
#include <dash/GlobRef.h>
#include <dash/HView.h>
#include <dash/Container.h>

#include <dash/matrix/MatrixRefView.h>
#include <dash/matrix/MatrixRef.h>
#include <dash/matrix/LocalMatrixRef.h>

#include <type_traits>


/**
 * \defgroup  DashMatrixConcept  Matrix Concept
 * Concept for a distributed n-dimensional matrix.
 * Extends concepts Container and Array.
 *
 * \see DashArrayConcept
 * \see DashContainerConcept
 *
 * \ingroup DashConcept
 * \{
 * \par Description
 *
 * The Matrix concept extends the n-dimensional Array by
 * operations that are prevalent in linear algebra, such
 * as projection to lower dimensions (e.g. rows and columns)
 * or slices.
 *
 * \par Types
 *
 * Type name        | Description
 * ---------------- | ---------------------------------------------------------------
 * index_type       | Integer denoting an offset/coordinate in cartesian index space.
 * size_type        | Integer denoting an extent in cartesian index space.
 * local_type       | Reference to local element range, allows range-based iteration.
 * pattern_type     | Concrete type implementing the Pattern concept that specifies the container's data distribution and cartesian access pattern.
 *
 *
 * \par Methods
 *
 * \}
 */

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
 * An n-dimensional array supporting subranges and sub-dimensional 
 * projection.
 *
 * Roughly follows the design presented in
 *   "The C++ Programming Language" (Bjarne Stroustrup)
 *   Chapter 29: A Matrix Design
 *
 * \ingroup  DashMatrixConcept
 * \ingroup  DashContainerConcept
 *
 */
template<
  typename ElementT,
  dim_t NumDimensions,
  typename IndexT   = dash::default_index_t,
  class PatternT    = TilePattern<NumDimensions, ROW_MAJOR, IndexT> >
class Matrix {
  static_assert(std::is_trivial<ElementT>::value,
    "Element type must be trivial copyable");

 private:
  typedef Matrix<ElementT, NumDimensions, IndexT, PatternT>
    self_t;
  typedef typename std::make_unsigned<IndexT>::type
    SizeType;
  typedef MatrixRefView<ElementT, NumDimensions, PatternT>
    MatrixRefView_t;
  typedef LocalMatrixRef<ElementT, NumDimensions, NumDimensions, PatternT>
    LocalRef_t;
  typedef PatternT
    Pattern_t;
  typedef GlobIter<ElementT, Pattern_t>
    GlobIter_t;
  typedef GlobMem<ElementT>
    GlobMem_t;
  typedef DistributionSpec<NumDimensions>
    DistributionSpec_t;
  typedef SizeSpec<NumDimensions, typename PatternT::size_type>
    SizeSpec_t;
  typedef TeamSpec<NumDimensions, typename PatternT::index_type>
    TeamSpec_t;
  typedef std::array<typename PatternT::size_type, NumDimensions>
    Extents_t;
  template <dim_t NumViewDim>
    using MatrixRef_t =
          MatrixRef<ElementT, NumDimensions, NumViewDim, PatternT>;

 public:
  typedef ElementT                                            value_type;
  typedef typename PatternT::size_type                         size_type;
  typedef typename PatternT::index_type                  difference_type;
  typedef typename PatternT::index_type                       index_type;

  typedef GlobIter_t                                            iterator;
  typedef const GlobIter_t                                const_iterator;
  typedef std::reverse_iterator<iterator>               reverse_iterator;
  typedef std::reverse_iterator<const_iterator>   const_reverse_iterator;

  typedef GlobRef<value_type>                                  reference;
  typedef const GlobRef<value_type>                      const_reference;

  typedef GlobIter_t                                             pointer;
  typedef const GlobIter_t                                 const_pointer;

 public:
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

/// Public types as required by dash container concept
public:
  typedef LocalRef_t                                          local_type;
  typedef const LocalRef_t                              const_local_type;
  typedef LocalRef_t                                local_reference_type;
  typedef const LocalRef_t                    const_local_reference_type;
  /// The type of the pattern used to distribute array elements to units
  typedef PatternT                                          pattern_type;

  typedef LocalRef_t
    Local;  
  template <dim_t NumViewDim>
    using View = MatrixRef<ElementT, NumDimensions, NumViewDim, PatternT>;

 public:
  /// Local proxy object, allows use in range-based for loops.
  local_type local;

 public:
  /**
   * Default constructor, for delayed allocation.
   *
   * Sets the associated team to DART_TEAM_NULL for global matrix instances
   * that are declared before \c dash::Init().
   */
  inline Matrix(
    Team & team = dash::Team::Null());

  /**
   * Constructor, creates a new instance of Matrix.
   */
  inline Matrix(
    const SizeSpec_t & ss,
    const DistributionSpec_t & ds = DistributionSpec_t(),
    Team & t                      = dash::Team::All(),
    const TeamSpec_t & ts         = TeamSpec_t());

  /**
   * Constructor, creates a new instance of Matrix from a pattern instance.
   */
  inline Matrix(
    const PatternT & pat);

  /**
   * Constructor, creates a new instance of Matrix.
   */
  inline Matrix(
    /// Number of elements
    size_t nelem,
    /// Team containing all units operating on the Matrix instance
    Team & t = dash::Team::All())
  : Matrix(PatternT(nelem, t)) { }

  /**
   * Destructor, frees underlying memory.
   */
  inline ~Matrix();

  /**
   * View at block at given global block coordinates.
   */
  MatrixRef_t<NumDimensions> block(
    const std::array<index_type, NumDimensions> & block_gcoords)
  {
    // Note: This is equivalent to
    //   foreach (d in 0 ... NumDimensions):
    //     view = view.sub<d>(block_view.offset(d),
    //                        block_view.extent(d));
    //
    DASH_LOG_TRACE("Matrix.block()", block_gcoords);
    auto block_gindex = pattern().blockspec().at(block_gcoords);
    DASH_LOG_TRACE("Matrix.block()", block_gindex);
    // Resolve the block's viewspec:
    ViewSpec<NumDimensions> block_view = pattern().block(block_gindex);
    // Return a view specified by the block's viewspec:
    MatrixRef_t<NumDimensions> view;
    view._refview            = new MatrixRefView_t(this);
    view._refview->_viewspec = block_view;
    DASH_LOG_TRACE("Matrix.block >", block_view);
    return view;
  }

  /**
   * View at block at given global block offset.
   */
  MatrixRef_t<NumDimensions> block(
    index_type block_gindex)
  {
    // Note: This is equivalent to
    //   foreach (d in 0 ... NumDimensions):
    //     view = view.sub<d>(block_view.offset(d),
    //                        block_view.extent(d));
    //
    DASH_LOG_TRACE("Matrix.block()", block_gindex);
    // Resolve the block's viewspec:
    ViewSpec<NumDimensions> block_view = pattern().block(block_gindex);
    // Return a view specified by the block's viewspec:
    MatrixRef_t<NumDimensions> view;
    view._refview            = new MatrixRefView_t(this);
    view._refview->_viewspec = block_view;
    DASH_LOG_TRACE("Matrix.block >", block_view);
    return view;
  }

  /**
   * Explicit allocation of matrix elements, used for delayed allocation
   * of default-constructed Matrix instance.
   */
  bool allocate(
    size_type nelem,
    dash::DistributionSpec<1> distribution,
    dash::Team & team = dash::Team::All());

  /**
   * Explicit deallocation of matrix elements, called implicitly in
   * destructor and team deallocation.
   */
  void deallocate();

  inline    Team            & team();

  constexpr size_type         size()                const noexcept;
  constexpr size_type         local_size()          const noexcept;
  constexpr size_type         local_capacity()      const noexcept;
  constexpr size_type         extent(dim_t dim)     const noexcept;
  constexpr Extents_t         extents()             const noexcept;
  constexpr bool              empty()               const noexcept;

  inline    void              barrier()             const;

  /**
   * The pattern used to distribute matrix elements to units in its
   * associated team.
   */
  inline    const Pattern_t & pattern()             const;

  inline    const_pointer     data()                const noexcept;
  inline    iterator          begin()                     noexcept;
  inline    const_iterator    begin()               const noexcept;
  inline    iterator          end()                       noexcept;
  inline    const_iterator    end()                 const noexcept;

  /* TODO:
   * Projection order matrix.sub().local() is not fully implemented yet.
   * Currently only matrix.local().sub() is supported.
   */

  /// Local proxy object representing a view consisting of matrix elements
  /// that are located in the active unit's local memory.
  inline    local_type        sub_local()                 noexcept;
  /// Pointer to first element in local range.
  inline    ElementT        * lbegin()                    noexcept;
  /// Pointer past final element in local range.
  inline    ElementT        * lend()                      noexcept;

  /**
   * Subscript operator, returns a submatrix reference at given offset
   * in global element range.
   */
  inline const MatrixRef<ElementT, NumDimensions, NumDimensions-1, PatternT>
  operator[](
    size_type n) const;

  /**
   * Subscript operator, returns a submatrix reference at given offset
   * in global element range.
   */
  inline MatrixRef<ElementT, NumDimensions, NumDimensions-1, PatternT>
  operator[](
    size_type n);

  /**
   * Projection to given offset in a sub-dimension.
   *
   * \see  row
   * \see  col
   */
  template<dim_t NumSubDimensions>
  inline MatrixRef<ElementT, NumDimensions, NumDimensions-1, PatternT> 
  sub(
    size_type n);
  
  /**
   * Projection to given offset in first sub-dimension (column), same as
   * \c sub<0>(n).
   *
   * \returns  A \c MatrixRef object representing the nth column
   *
   * \see  sub
   * \see  row
   */
  inline MatrixRef<ElementT, NumDimensions, NumDimensions-1, PatternT> 
  col(
    size_type n);
  
  /**
   * Projection to given offset in second sub-dimension (rows), same as
   * \c sub<1>(n).
   *
   * \returns  A \c MatrixRef object representing the nth row
   *
   * \see  sub
   * \see  col
   */
  inline MatrixRef<ElementT, NumDimensions, NumDimensions-1, PatternT> 
  row(
    size_type n);

  template<dim_t SubDimension>
  inline MatrixRef<ElementT, NumDimensions, NumDimensions, PatternT> 
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
  inline MatrixRef<ElementT, NumDimensions, NumDimensions, PatternT> 
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
  inline MatrixRef<ElementT, NumDimensions, NumDimensions, PatternT>
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
  inline reference at(
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
  inline reference operator()(
    /// Global coordinates
    Args... args);

  /**
   * Whether the element at a global, canonical offset in the matrix
   * is local to the active unit.
   */
  inline bool is_local(
    size_type g_pos) const;

  /**
   * Whether the element at a global, canonical offset in a specific
   * dimension of the matrix is local to the active unit.
   */
  template<dim_t Dimension>
  inline bool is_local(
    size_type g_pos) const;

  template <int level>
  inline dash::HView<self_t, level> hview();

  /**
   * Conversion operator to type \c MatrixRef.
   */
  inline operator
    MatrixRef<ElementT, NumDimensions, NumDimensions, PatternT> ();

 private:
  /**
   * Actual implementation of memory acquisition.
   */
  bool allocate(
    const PatternT & pattern);

 private:
  /// Team containing all units that collectively instantiated the
  /// Matrix instance
  dash::Team                 & _team;
  /// DART id of the unit that owns this matrix instance
  dart_unit_t                  _myid;
  /// Capacity (total number of elements) of the matrix
  size_type                    _size;
  /// Number of local elements in the array
  size_type                    _lsize;
  /// Number allocated local elements in the array
  size_type                    _lcapacity;
  /// Global pointer to initial element in the array
  pointer                      _begin;
  /// The matrix elements' distribution pattern
  Pattern_t                    _pattern;
  /// Global memory allocation and -access
  GlobMem<ElementT>          * _glob_mem;
  /// Native pointer to first local element in the array
  ElementT                   * _lbegin;
  /// Native pointer past last local element in the array
  ElementT                   * _lend;
  /// Proxy instance for applying a view, e.g. in subscript operator
  MatrixRef_t<NumDimensions>   _ref;
};

}  // namespace dash

#include <dash/matrix/internal/Matrix-inl.h>

#endif  // DASH__MATRIX_H_INCLUDED
