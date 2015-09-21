#ifndef DASH_MATRIX_H_INCLUDED
#define DASH_MATRIX_H_INCLUDED

#include <type_traits>
#include <stdexcept>

#include <dash/dart/if/dart.h>

#include <dash/Team.h>
#include <dash/Pattern.h>
#include <dash/GlobIter.h>
#include <dash/GlobRef.h>
#include <dash/HView.h>
#include <dash/Container.h>

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
 * <table>
 *   <tr>
 *     <th>Type name</th>
 *     <th>Description</th>
 *   </tr>
 *
 *   <tr>
 *     <td>
 *       \code
 *         Container::index_type
 *       \endcode
 *     </td>
 *     <td>
 *       Integer denoting an offset/coordinate in cartesian index space
 *     </td>
 *   </tr>
 *
 *   <tr>
 *     <td>
 *       \code
 *         Container::size_type
 *       \endcode
 *     </td>
 *     <td>
 *       Integer denoting an extent in cartesian index space
 *     </td>
 *   </tr>
 *
 *   <tr>
 *     <td>
 *       \code
 *         Container::pattern_type
 *       \endcode
 *     </td>
 *     <td>
 *       Concrete type implementing the Pattern concept that is used to
 *       distribute elements to units in a team.
 *     </td>
 *   </tr>
 *
 *   <tr>
 *     <td>
 *       \code
 *         Container::local_type
 *       \endcode
 *     </td>
 *     <td>
 *       Reference to local element range, allows range-based
 *       iteration
 *     </td>
 *   </tr>
 *
 * </table>
 *
 * \par Methods
 *
 * <table>
 *   <tr>
 *     <th>Method Signature</th>
 *     <th>Semantics</th>
 *   </tr>
 *
 *   <tr>
 *     <td>
 *       \code
 *       const pattern_type & pattern()
 *       \endcode
 *     </td>
 *     <td>
 *       Pattern used to distribute container elements
 *     </td>
 *   </tr>
 *   
 *   <tr>
 *     <td>
 *       \code
 *        local_type local
 *       \endcode
 *     </td>
 *     <td>
 *       Local range of the container, allows range-based
 *       iteration
 *     </td>
 *   </tr>
 *   
 *   <tr>
 *     <td>
 *       \code
 *         GlobIter<Value> begin()
 *       \endcode
 *     </td>
 *     <td>
 *       Global iterator referencing the initial element
 *     </td>
 *   </tr>
 *   
 *   <tr>
 *     <td>
 *       \code
 *         GlobIter<Value> end()
 *       \endcode
 *     </td>
 *     <td>
 *       Global iterator referencing past the final element
 *     </td>
 *   </tr>
 *   
 *   <tr>
 *     <td>
 *       \code
 *         Value * lbegin()
 *       \endcode
 *     </td>
 *     <td>
 *       Native pointer to the initial local matrix element
 *     </td>
 *   </tr>
 *   
 *   <tr>
 *     <td>
 *       \code
 *         Value * lend()
 *       \endcode
 *     </td>
 *     <td>
 *       Native pointer past the final local matrix element
 *     </td>
 *   </tr>
 *
 * </table>
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
class LocalRef;

/**
 * Stores information needed by subscripting and subdim selection.
 * A new \c MatrixRefProxy instance is created once for every dimension in
 * multi-subscripting.
 *
 * \ingroup Matrix
 */
template <
  typename T,
  dim_t NumDimensions,
  class PatternT =
    TilePattern<NumDimensions, ROW_MAJOR, dash::default_index_t> >
class MatrixRefProxy {
 public:
  typedef typename PatternT::index_type             index_type;

 private:
  dim_t                                             _dim;
  Matrix<T, NumDimensions, index_type, PatternT>  * _mat;
  ::std::array<index_type, NumDimensions>           _coord     = {  };
  ViewSpec<NumDimensions, index_type>               _viewspec;

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
  friend class LocalRef;
  template<
    typename T_,
    dim_t NumDimensions1,
    typename IndexT_,
    class PatternT_ > 
  friend class Matrix;

  MatrixRefProxy<T, NumDimensions, PatternT>();
  MatrixRefProxy<T, NumDimensions, PatternT>(
    Matrix<T, NumDimensions, index_type, PatternT> * matrix);
  MatrixRefProxy<T, NumDimensions, PatternT>(
    const MatrixRefProxy<T, NumDimensions, PatternT> & other);

  GlobRef<T> global_reference() const;
};

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
class LocalRef {
 public:
  template<
    typename T_,
    dim_t NumDimensions_,
    typename IndexT_,
    class PatternT_ >
  friend class Matrix;

 public:
  typedef T                              value_type;
  typedef PatternT                       pattern_type;
  typedef typename PatternT::index_type  index_type;

  typedef typename PatternT::size_type   size_type;
  typedef typename PatternT::index_type  difference_type;

  typedef GlobIter<value_type, PatternT> iterator;
  typedef const GlobIter<value_type, PatternT> const_iterator;
  typedef std::reverse_iterator<iterator> reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

  typedef GlobRef<value_type> reference;
  typedef const GlobRef<value_type> const_reference;

  typedef GlobIter<value_type, PatternT> pointer;
  typedef const GlobIter<value_type, PatternT> const_pointer;

 public:
  LocalRef<T, NumDimensions, CUR, PatternT>() = default;

  /**
   * Constructor, creates a local reference to a Matrix instance.
   */
  LocalRef<T, NumDimensions, CUR, PatternT>(
    Matrix<T, NumDimensions, index_type, PatternT> * mat);

  inline operator LocalRef<T, NumDimensions, CUR-1, PatternT> && ();
  // SHOULD avoid cast from MatrixRef to LocalRef.
  // Different operation semantics.
  inline operator MatrixRef<T, NumDimensions, CUR, PatternT> ();
  inline size_type extent(dim_t dim) const;
  inline std::array<size_type, NumDimensions> extents() const noexcept;
  inline size_type size() const;
  inline T & local_at(size_type pos);

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
  LocalRef<T, NumDimensions, CUR-1, PatternT> &&
    operator[](index_type n);

  /**
   * Subscript operator, access element at given offset in
   * global element range.
   */
  LocalRef<T, NumDimensions, CUR-1, PatternT>
    operator[](index_type n) const;

  template<dim_t NumSubDimensions>
  LocalRef<T, NumDimensions, NumDimensions-1, PatternT>
    sub(size_type n);
  inline LocalRef<T, NumDimensions, NumDimensions-1, PatternT> 
    col(size_type n);
  inline LocalRef<T, NumDimensions, NumDimensions-1, PatternT> 
    row(size_type n);

  template<dim_t SubDimension>
  LocalRef<T, NumDimensions, NumDimensions, PatternT> submat(
    size_type n,
    size_type range);

  /**
   * Create a view representing the matrix slice within a row
   * range.
   * Same as \c submat<0>(offset, extent).
   *
   * \returns  A matrix view
   *
   * \see  submat
   */
  inline LocalRef<T, NumDimensions, NumDimensions, PatternT> rows(
    /// Offset of first row in range
    size_type offset,
    /// Number of rows in the range
    size_type range);

  /**
   * Create a view representing the matrix slice within a column
   * range.
   * Same as \c submat<1>(offset, extent).
   *
   * \returns  A matrix view
   *
   * \see  submat
   */
  inline LocalRef<T, NumDimensions, NumDimensions, PatternT> cols(
    /// Offset of first column in range
    size_type offset,
    /// Number of columns in the range
    size_type extent);

 private:
  MatrixRefProxy<T, NumDimensions, PatternT> * _proxy;
};

/**
 * Local reference to a Matrix element, provides local operations.
 * Partial Specialization for value deferencing.
 *
 * \ingroup Matrix
 */
template <
  typename T,
  dim_t NumDimensions,
  class PatternT >
class LocalRef<T, NumDimensions, 0, PatternT> {
 public:
  template<
    typename T_,
    dim_t NumDimensions_,
    typename IndexT_,
    class PatternT_ >
  friend class Matrix;

 public:
  typedef typename PatternT::index_type  index_type;
  typedef typename PatternT::size_type   size_type;

 public:
  LocalRef<T, NumDimensions, 0, PatternT>() = default;

  inline T * local_at(index_type pos);
  inline operator T();
  inline T operator=(const T & value);

 private:
  MatrixRefProxy<T, NumDimensions, PatternT> * _proxy;
};

/**
 * A view on a referenced \c Matrix object, such as a dimensional
 * projection returned by \c Matrix::sub.
 *
 * \see DashMatrixConcept
 *
 * \ingroup Matrix
 */
template <
  typename ElementT,
  dim_t NumDimensions,
  dim_t CUR = NumDimensions,
  class PatternT =
    TilePattern<NumDimensions, ROW_MAJOR, dash::default_index_t> >
class MatrixRef {
 private:
  typedef MatrixRef<ElementT, NumDimensions, CUR, PatternT>
    self_t;
  typedef PatternT
    Pattern_t;
  typedef typename PatternT::index_type
    Index_t;
  typedef MatrixRefProxy<ElementT, NumDimensions, PatternT>
    MatrixRefProxy_t;
  typedef LocalRef<ElementT, NumDimensions, NumDimensions, PatternT>
    LocalRef_t;
  typedef GlobIter<ElementT, PatternT>
    GlobIter_t;

 public:
  typedef PatternT                       pattern_type;
  typedef typename PatternT::index_type  index_type;
  typedef ElementT                       value_type;

  typedef typename PatternT::size_type   size_type;
  typedef typename PatternT::index_type  difference_type;

  typedef GlobIter_t                                          iterator;
  typedef const GlobIter_t                              const_iterator;
  typedef std::reverse_iterator<iterator>             reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

  typedef GlobRef<value_type>                                reference;
  typedef const GlobRef<value_type>                    const_reference;

  typedef GlobIter_t                                           pointer;
  typedef const GlobIter_t                               const_pointer;
  
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
  friend class LocalRef;

  inline operator
    MatrixRef<ElementT, NumDimensions, CUR-1, PatternT> && ();

 public:
  MatrixRef<ElementT, NumDimensions, CUR, PatternT>()
  : _proxy(nullptr) { // = default;
    DASH_LOG_TRACE_VAR("MatrixRef<T,D,C>()", NumDimensions);
  }
  MatrixRef<ElementT, NumDimensions, CUR, PatternT>(
    const MatrixRef<ElementT, NumDimensions, CUR+1, PatternT> & previous,
    index_type coord);

  PatternT & pattern();

  Team & team();

  inline constexpr size_type size() const noexcept;
  inline size_type extent(size_type dim) const noexcept;
  inline std::array<size_type, NumDimensions> extents() const noexcept;
  inline constexpr bool empty() const noexcept;
  inline void barrier() const;
  inline Pattern_t pattern() const;

  /**
   * Subscript operator, returns a submatrix reference at given offset
   * in global element range.
   */
  MatrixRef<ElementT, NumDimensions, CUR-1, PatternT>
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
  submat(
    size_type n,
    size_type range);

  /**
   * Create a view representing the matrix slice within a row
   * range.
   * Same as \c submat<0>(offset, extent).
   *
   * \returns  A matrix view
   *
   * \see  submat
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
   * Same as \c submat<1>(offset, extent).
   *
   * \returns  A matrix view
   *
   * \see  submat
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

  template <int level>
  dash::HView<Matrix<ElementT, NumDimensions, Index_t, PatternT>, level>
  inline hview();

 private:
  MatrixRefProxy<ElementT, NumDimensions, PatternT> * _proxy;
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
class MatrixRef< ElementT, NumDimensions, 0, PatternT > {
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
  
  inline const GlobRef<ElementT> local_at(
    dart_unit_t unit,
    index_type elem) const;

  inline GlobRef<ElementT> local_at(
    dart_unit_t unit,
    index_type elem);

  MatrixRef<ElementT, NumDimensions, 0, PatternT>()
  : _proxy(nullptr) {
    DASH_LOG_TRACE_VAR("MatrixRef<T,D,0>()", NumDimensions);
  }

  MatrixRef<ElementT, NumDimensions, 0, PatternT>(
    const MatrixRef<ElementT, NumDimensions, 1, PatternT> & previous,
    index_type coord);

  operator ElementT();
  ElementT operator=(const ElementT & value);

 private:
  MatrixRefProxy<ElementT, NumDimensions, PatternT> * _proxy;
};

/**
 * An n-dimensional array supporting subranges and sub-dimensional 
 * projection.
 *
 * Roughly follows the design presented in
 *   "The C++ Programming Language" (Bjarne Stroustrup)
 *   Chapter 29: A Matrix Design
 *
 * \see  DashContainerConcept
 * \see  DashMatrixConcept
 *
 * \ingroup Matrix
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
  typedef MatrixRef<ElementT, NumDimensions, NumDimensions, PatternT>
    MatrixRef_t;
  typedef MatrixRefProxy<ElementT, NumDimensions, PatternT>
    MatrixRefProxy_t;
  typedef LocalRef<ElementT, NumDimensions, NumDimensions, PatternT>
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

 public:
  typedef ElementT                                            value_type;
  typedef typename PatternT::size_type                         size_type;
  typedef typename PatternT::index_type                  difference_type;

  typedef GlobIter_t                                            iterator;
  typedef const GlobIter_t                                const_iterator;
  typedef std::reverse_iterator<iterator>               reverse_iterator;
  typedef std::reverse_iterator<const_iterator>   const_reverse_iterator;

  typedef GlobRef<value_type>                                  reference;
  typedef const GlobRef<value_type>                      const_reference;

  typedef GlobIter_t                                             pointer;
  typedef const GlobIter_t                                 const_pointer;

  typedef LocalRef_t                                          local_type;
  typedef const LocalRef_t                              const_local_type;
  typedef LocalRef_t                                local_reference_type;
  typedef const LocalRef_t                    const_local_reference_type;

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
  friend class LocalRef;

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

  inline Team & team();
  inline constexpr size_type size() const noexcept;
  inline constexpr size_type local_size() const noexcept;
  inline constexpr size_type local_capacity() const noexcept;
  inline constexpr size_type extent(dim_t dim) const noexcept;
  inline constexpr std::array<size_type, NumDimensions> extents() const noexcept;
  inline constexpr bool empty() const noexcept;
  inline void barrier() const;
  inline const_pointer data() const noexcept;
  inline iterator begin() noexcept;
  inline const_iterator begin() const noexcept;
  inline iterator end() noexcept;
  inline const_iterator end() const noexcept;
  inline ElementT * lbegin() noexcept;
  inline ElementT * lend() noexcept;

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
  submat(
    size_type n,
    size_type range);

  /**
   * Create a view representing the matrix slice within a row
   * range.
   * Same as \c submat<0>(offset, extent).
   *
   * \returns  A matrix view
   *
   * \see  submat
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
   * Same as \c submat<1>(offset, extent).
   *
   * \returns  A matrix view
   *
   * \see  submat
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
   * The pattern used to distribute matrix elements to units in its
   * associated team.
   */
  inline const PatternT & pattern() const;

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
  bool allocate(const PatternT & pattern);

 private:
  /// Team containing all units that collectively instantiated the
  /// Matrix instance
  dash::Team           & _team;
  /// DART id of the unit that owns this matrix instance
  dart_unit_t            _myid;
  /// Capacity (total number of elements) of the matrix
  size_type              _size;
  /// Number of local elements in the array
  size_type              _lsize;
  /// Number allocated local elements in the array
  size_type              _lcapacity;
  /// Global pointer to initial element in the array
  pointer                _begin;
  /// The matrix elements' distribution pattern
  Pattern_t              _pattern;
  /// Global memory allocation and -access
  GlobMem<ElementT>    * _glob_mem;
  /// Native pointer to first local element in the array
  ElementT             * _lbegin;
  /// Native pointer past last local element in the array
  ElementT             * _lend;
  /// Proxy instance for applying a view, e.g. in subscript operator
  MatrixRef_t            _ref;
};

}  // namespace dash

#include "internal/Matrix-inl.h"

#endif  // DASH_MATRIX_H_INCLUDED
