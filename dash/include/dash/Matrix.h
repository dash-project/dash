#ifndef DASH__MATRIX_H_INCLUDED
#define DASH__MATRIX_H_INCLUDED

#include <dash/dart/if/dart.h>

#include <dash/Team.h>
#include <dash/Pattern.h>
#include <dash/GlobRef.h>
#include <dash/memory/GlobStaticMem.h>
#include <dash/Allocator.h>
#include <dash/HView.h>
#include <dash/Meta.h>

#include <dash/iterator/GlobIter.h>

#include <dash/matrix/MatrixRefView.h>
#include <dash/matrix/MatrixRef.h>
#include <dash/matrix/LocalMatrixRef.h>

#include <type_traits>


/**
 * \defgroup  DashMatrixConcept  Matrix Concept
 * Concept for a distributed n-dimensional matrix.
 *
 * Extends concepts \ref DashContainerConcept and \ref DashArrayConcept.
 *
 * \see DashContainerConcept
 * \see DashArrayConcept
 * \see DashViewConcept
 *
 * \ingroup DashContainerConcept
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
 * As defined in \ref DashContainerConcept.
 *
 * Type name                       | Description
 * ------------------------------- | -------------------------------------------------------------------------------------------------------------------
 * <tt>value_type</tt>             | Type of the container elements.
 * <tt>difference_type</tt>        | Integer type denoting a distance in cartesian index space.
 * <tt>index_type</tt>             | Integer type denoting an offset/coordinate in cartesian index space.
 * <tt>size_type</tt>              | Integer type denoting an extent in cartesian index space.
 * <tt>iterator</tt>               | Iterator on container elements in global index space.
 * <tt>const_iterator</tt>         | Iterator on const container elements in global index space.
 * <tt>reverse_iterator</tt>       | Reverse iterator on container elements in global index space.
 * <tt>const_reverse_iterator</tt> | Reverse iterator on const container elements in global index space.
 * <tt>reference</tt>              | Reference on container elements in global index space.
 * <tt>const_reference</tt>        | Reference on const container elements in global index space.
 * <tt>local_pointer</tt>          | Native pointer on local container elements.
 * <tt>const_local_pointer</tt>    | Native pointer on const local container elements.
 * <tt>view_type</tt>              | View specifier on container elements, model of \ref DashViewConcept.
 * <tt>local_type</tt>             | Reference to local element range, allows range-based iteration.
 * <tt>pattern_type</tt>           | Concrete model of the Pattern concept that specifies the container's data distribution and cartesian access pattern.
 *
 *
 * \par Methods
 *
 * Return Type              | Method                | Parameters                      | Description
 * ------------------------ | --------------------- | ------------------------------- | -------------------------------------------------------------------------------------------------------------
 * <tt>view_type<d></tt>    | <tt>block</tt>        | <tt>index_type    bi</tt>       | Matrix proxy object representing a view specifier on the matrix block at canonical block index <tt>bi</tt>.
 * <tt>view_type<d></tt>    | <tt>block</tt>        | <tt>index_type[d] bp</tt>       | Matrix proxy object representing a view specifier on the matrix block at block coordinate <tt>bc</tt>.
 *
 * As defined in \ref DashContainerConcept :
 *
 * Return Type              | Method                | Parameters                                              | Description
 * ------------------------ | --------------------- | ------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------
 * <tt>local_type</tt>      | <tt>local</tt>        | &nbsp;                                                  | Container proxy object representing a view specifier on the container's local elements.
 * <tt>pattern_type</tt>    | <tt>pattern</tt>      | &nbsp;                                                  | Object implementing the Pattern concept specifying the container's data distribution and iteration pattern.
 * <tt>iterator</tt>        | <tt>begin</tt>        | &nbsp;                                                  | Iterator referencing the first container element.
 * <tt>iterator</tt>        | <tt>end</tt>          | &nbsp;                                                  | Iterator referencing the element past the last container element.
 * <tt>Element *</tt>       | <tt>lbegin</tt>       | &nbsp;                                                  | Native pointer referencing the first local container element, same as <tt>local().begin()</tt>.
 * <tt>Element *</tt>       | <tt>lend</tt>         | &nbsp;                                                  | Native pointer referencing the element past the last local container element, same as <tt>local().end()</tt>.
 * <tt>size_type</tt>       | <tt>size</tt>         | &nbsp;                                                  | Number of elements in the container.
 * <tt>size_type</tt>       | <tt>local_size</tt>   | &nbsp;                                                  | Number of local elements in the container, same as <tt>local().size()</tt>.
 * <tt>bool</tt>            | <tt>is_local</tt>     | <tt>index_type gi</tt>                                  | Whether the element at the given linear offset in global index space <tt>gi</tt> is local.
 * <tt>bool</tt>            | <tt>allocate</tt>     | <tt>size_type n, DistributionSpec\<DD\> ds, Team t</tt> | Allocation of <tt>n</tt> container elements distributed in Team <tt>t</tt> as specified by distribution spec <tt>ds</tt>
 * <tt>void</tt>            | <tt>deallocate</tt>   | &nbsp;                                                  | Deallocation of the container and its elements.
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
 * \concept{DashMatrixConcept}
 *
 * \todo
 * Projection order matrix.sub().local() is not fully implemented yet.
 * Currently only matrix.local().sub() is supported.
 *
 * \note
 * Roughly follows the design presented in
 *   "The C++ Programming Language" (Bjarne Stroustrup)
 *   Chapter 29: A Matrix Design
 *
 */
template<
  typename ElementT,
  dim_t    NumDimensions,
  typename IndexT         = dash::default_index_t,
  class    PatternT       = TilePattern<NumDimensions, ROW_MAJOR, IndexT> >
class Matrix
{
  static_assert(
    dash::is_container_compatible<ElementT>::value,
    "Type not supported for DASH containers");

  static_assert(std::is_same<IndexT, typename PatternT::index_type>::value,
    "Index type IndexT must be the same for Matrix and specified pattern");

private:
  typedef Matrix<ElementT, NumDimensions, IndexT, PatternT>
    self_t;
  typedef typename std::make_unsigned<IndexT>::type
    SizeType;
  typedef MatrixRefView<ElementT, NumDimensions, PatternT>
    MatrixRefView_t;
  typedef LocalMatrixRef<ElementT, NumDimensions, NumDimensions, PatternT>
    LocalRef_t;
  typedef LocalMatrixRef<
            const ElementT, NumDimensions, NumDimensions, PatternT>
    LocalRef_const_t;
  typedef PatternT
    Pattern_t;
  typedef GlobStaticMem<ElementT, dash::allocator::SymmetricAllocator<ElementT>>
    GlobMem_t;

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

public:
  typedef ElementT                                              value_type;
  typedef typename PatternT::size_type                           size_type;
  typedef typename PatternT::index_type                    difference_type;
  typedef typename PatternT::index_type                         index_type;

  typedef GlobIter<      value_type, Pattern_t>                   iterator;
  typedef GlobIter<const value_type, Pattern_t>             const_iterator;

  typedef std::reverse_iterator<iterator>                 reverse_iterator;
  typedef std::reverse_iterator<const_iterator>     const_reverse_iterator;

  typedef          GlobRef<value_type>                           reference;
  typedef typename GlobRef<value_type>::const_type         const_reference;

  typedef GlobIter<      value_type, Pattern_t>                    pointer;
  typedef GlobIter<const value_type, Pattern_t>              const_pointer;

  typedef       ElementT *                                   local_pointer;
  typedef const ElementT *                             const_local_pointer;

  typedef DistributionSpec<NumDimensions>                distribution_spec;
  typedef SizeSpec<NumDimensions, typename PatternT::size_type>
    size_spec;
  typedef TeamSpec<NumDimensions, typename PatternT::index_type>
    team_spec;
  typedef std::array<typename PatternT::size_type, NumDimensions>
    extents_type;
  typedef std::array<typename PatternT::index_type, NumDimensions>
    offsets_type;


// Public types as required by dash container concept
public:
  /// Type specifying the view on local matrix elements.
  typedef LocalMatrixRef<
            ElementT, NumDimensions, NumDimensions, PatternT>
    local_type;

  /// Type specifying the view on const local matrix elements.
  typedef LocalMatrixRef<
            const ElementT, NumDimensions, NumDimensions, PatternT>
    const_local_type;

  /// The type of the pattern specifying linear iteration order and how
  /// elements are distribute to units.
  typedef PatternT                                            pattern_type;

  /// Type of views on matrix elements such as sub-matrices, row- and
  /// column vectors.
  template <dim_t NumViewDim>
    using view_type =
          MatrixRef<ElementT, NumDimensions, NumViewDim, PatternT>;

  /// Type of views on matrix elements such as sub-matrices, row- and
  /// column vectors.
  template <dim_t NumViewDim>
    using const_view_type =
          MatrixRef<const ElementT, NumDimensions, NumViewDim, PatternT>;

// public types exposed in Matrix interface
public:

public:
  /// Local view proxy object.
  local_type local;

public:

  typedef std::integral_constant<dim_t, NumDimensions>                rank;

  static constexpr dim_t ndim() {
    return NumDimensions;
  }

private:
  /// Team containing all units that collectively instantiated the
  /// Matrix instance
  dash::Team                 * _team = nullptr;
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
  GlobMem_t                  * _glob_mem;
  /// Native pointer to first local element in the array
  ElementT                   * _lbegin;
  /// Native pointer past last local element in the array
  ElementT                   * _lend;
  /// Proxy instance for applying a view, e.g. in subscript operator
  view_type<NumDimensions>     _ref;

public:
  /**
   * Default constructor, for delayed allocation.
   *
   * Sets the associated team to DART_TEAM_NULL for global matrix instances
   * that are declared before \ref dash::Init().
   */
  explicit
  Matrix(
    Team & team = dash::Team::Null());

  /**
   * Constructor, creates a new instance of Matrix.
   */
  explicit
  Matrix(
    const size_spec         & ss,
    const distribution_spec & ds  = distribution_spec(),
    Team                    & t   = dash::Team::All(),
    const team_spec         & ts  = team_spec());

  /**
   * Constructor, creates a new instance of Matrix from a pattern instance.
   */
  explicit
  Matrix(
    const PatternT & pat);

  /**
   * Constructor, creates a new instance of Matrix
   * of given extents.
   */
  template<typename... Args>
  Matrix(SizeType arg, Args&&... args)
  : Matrix(PatternT(arg, std::forward<Args>(args)...))
  { }

  /**
   * Copy-constructor, deleted.
   */
  Matrix(const self_t &) = delete;

  /**
   * Move-constructor, supported
   */
  Matrix(self_t && other);

  /**
   * Destructor, frees underlying memory.
   */
  ~Matrix();

  /**
   * Copy-assignment operator, deleted.
   */
  self_t & operator=(const self_t & other) = delete;

  /**
   * Move-assignment operator, supported.
   */
  self_t & operator=(self_t && other);

  /**
   * View at block at given global block coordinates.
   */
  view_type<NumDimensions> block(
    const std::array<index_type, NumDimensions> & block_gcoords);

  /**
   * View at block at given global block offset.
   */
  view_type<NumDimensions> block(
    index_type block_gindex);

  /**
   * Explicit allocation of matrix elements, used for delayed allocation
   * of default-constructed Matrix instance.
   *
   * \see  DashContainerConcept
   */
  bool allocate(
    const size_spec         & sizespec,
    const distribution_spec & distribution,
    const team_spec         & teamspec,
    dash::Team              & team = dash::Team::All()
  );

  /**
   * Allocation and distribution of matrix elements as specified by a given
   * Pattern instance.
   */
  bool allocate(
    const PatternT & pattern
  );


  /**
   * Allocation and distribution of matrix elements as specified by given
   * extents. See variadic constructor.
   */
  template<typename... Args>
  bool
  allocate(SizeType arg, Args... args)
  {
    return allocate(PatternT(arg, args... ));
  }

  /**
   * Explicit deallocation of matrix elements, called implicitly in
   * destructor and team deallocation.
   *
   * \see  DashContainerConcept
   */
  void deallocate();

  constexpr Team            & team()                const noexcept;

  constexpr size_type         size()                const noexcept;
  constexpr size_type         local_size()          const noexcept;
  constexpr size_type         local_capacity()      const noexcept;
  constexpr size_type         extent(dim_t dim)     const noexcept;
  constexpr extents_type      extents()             const noexcept;
  constexpr index_type        offset(dim_t dim)     const noexcept;
  constexpr offsets_type      offsets()             const noexcept;
  constexpr bool              empty()               const noexcept;

  /**
   * Synchronize units associated with the matrix.
   *
   * \see  DashContainerConcept
   */
  inline void                 barrier() const;

  /**
   * Complete all outstanding non-blocking operations to all units
   * on the container's underlying global memory.
   *
   * \see  DashContainerConcept
   */
  inline void                 flush();

  /**
   * Complete all outstanding non-blocking operations to the specified unit
   * on the container's underlying global memory.
   *
   * \see  DashContainerConcept
   */
  inline void                 flush(dash::team_unit_t target);

  /**
   * Locally complete all outstanding non-blocking operations to all units
   * on the container's underlying global memory.
   */
  inline void                 flush_local();

  /**
   * Locally complete all outstanding non-blocking operations to the specified
   * unit on the container's underlying global memory.
   */
  inline void                 flush_local(dash::team_unit_t target);

  /**
   * The pattern used to distribute matrix elements to units in its
   * associated team.
   *
   * \see  DashContainerConcept
   */
  constexpr const Pattern_t & pattern() const;

  /**
   * Iterator referencing first matrix element in global index space.
   *
   * \see  DashContainerConcept
   */
                  iterator    begin()         noexcept;

  /**
   * Iterator referencing first matrix element in global index space.
   *
   * \see  DashContainerConcept
   */
  constexpr const_iterator    begin()   const noexcept;

  /**
   * Iterator referencing past the last matrix element in global index
   * space.
   *
   * \see  DashContainerConcept
   */
                  iterator    end()           noexcept;

  /**
   * Iterator referencing past the last matrix element in global index
   * space.
   *
   * \see  DashContainerConcept
   */
  constexpr const_iterator    end( )    const noexcept;

  /**
   * Pointer to first element in local range.
   *
   * \see  DashContainerConcept
   */
                  ElementT *  lbegin()        noexcept;

  /**
   * Pointer to first element in local range.
   *
   * \see  DashContainerConcept
   */
  constexpr const ElementT *  lbegin()  const noexcept;

  /**
   * Pointer past final element in local range.
   *
   * \see  DashContainerConcept
   */
                  ElementT *  lend()          noexcept;

  /**
   * Pointer past final element in local range.
   *
   * \see  DashContainerConcept
   */
  constexpr const ElementT *  lend()    const noexcept;

  /**
   * Subscript operator, returns a submatrix reference at given offset
   * in global element range.
   */
  template<dim_t __NumViewDim = NumDimensions-1>
  typename std::enable_if<
             (__NumViewDim != 0),
             const_view_type<__NumViewDim>
           >::type
  constexpr operator[](
    size_type n       ///< Offset in highest matrix dimension.
  ) const;

  /**
   * Subscript operator, returns a \ref GlobRef if matrix has only one dimension
   */
  template<dim_t __NumViewDim = NumDimensions-1>
  typename std::enable_if<
             (__NumViewDim == 0),
             const_reference
           >::type
  constexpr operator[](
    size_type n       ///< Offset in highest matrix dimension.
  ) const;

  /**
   * Subscript operator, returns a submatrix reference at given offset
   * in global element range.
   */
  template<dim_t __NumViewDim = NumDimensions-1>
  typename std::enable_if<(__NumViewDim != 0), view_type<__NumViewDim>>::type
  operator[](
    size_type n       ///< Offset in highest matrix dimension.
  );

  /**
   * Subscript operator, returns a \ref GlobRef if matrix has only one dimension
   */
  template<dim_t __NumViewDim = NumDimensions-1>
  typename std::enable_if<(__NumViewDim == 0), reference>::type
  operator[](
    size_type n       ///< Offset in highest matrix dimension.
  );

  template<dim_t SubDimension>
  const_view_type<NumDimensions> sub(
    size_type n,      ///< Offset of the sub-range.
    size_type range   ///< Width of the sub-range.
  ) const;

  template<dim_t SubDimension>
  view_type<NumDimensions> sub(
    size_type n,      ///< Offset of the sub-range.
    size_type range   ///< Width of the sub-range.
  );

  /**
   * Projection to given offset in a sub-dimension.
   *
   * \see  row
   * \see  col
   */
  template<dim_t SubDimension>
  const_view_type<NumDimensions-1> sub(
    size_type n       ///< Offset in selected dimension.
  ) const;

  /**
   * Projection to given offset in a sub-dimension.
   *
   * \see  row
   * \see  col
   */
  template<dim_t SubDimension>
  view_type<NumDimensions-1> sub(
    size_type n       ///< Offset in selected dimension.
  );

  /**
   * Local proxy object representing a view consisting of matrix elements
   * that are located in the active unit's local memory.
   */
  local_type sub_local() noexcept;

  /**
   * Projection to given offset in first sub-dimension (column), same as
   * \c sub<0>(n).
   *
   * \returns  A \ref MatrixRef object representing the nth column
   *
   * \see  sub
   * \see  row
   */
  const_view_type<NumDimensions-1> col(
    size_type n       ///< Column offset.
  ) const;

  /**
   * Projection to given offset in first sub-dimension (column), same as
   * \c sub<0>(n).
   *
   * \returns  A \ref MatrixRef object representing the nth column
   *
   * \see  sub
   * \see  row
   */
  view_type<NumDimensions-1> col(
    size_type n       ///< Column offset.
  );

  /**
   * Projection to given offset in second sub-dimension (rows), same as
   * \c sub<1>(n).
   *
   * \returns  A \ref MatrixRef object representing the nth row
   *
   * \see  sub
   * \see  col
   */
  const_view_type<NumDimensions-1> row(
    size_type n       ///< Row offset.
  ) const;

  /**
   * Projection to given offset in second sub-dimension (rows), same as
   * \c sub<1>(n).
   *
   * \returns  A \ref MatrixRef object representing the nth row
   *
   * \see  sub
   * \see  col
   */
  view_type<NumDimensions-1> row(
    size_type n       ///< Row offset.
  );

  /**
   * Create a view representing the matrix slice within a column
   * range.
   * Same as \c sub<1>(offset, extent).
   *
   * \returns  A matrix view
   *
   * \see  sub
   */
  view_type<NumDimensions> cols(
    size_type offset, ///< Offset of first column in range.
    size_type range   ///< Number of columns in the range.
  );

  /**
   * Create a view representing the matrix slice within a row
   * range.
   * Same as \c sub<0>(offset, extent).
   *
   * \returns  A matrix view
   *
   * \see  sub
   */
  view_type<NumDimensions> rows(
    size_type n,      ///< Offset of first row in range.
    size_type range   ///< Number of rows in the range.
  );

  /**
   * Fortran-style subscript operator.
   * As an example, the operation \c matrix(i,j) is equivalent to
   * \c matrix[i][j].
   *
   * \returns  A global reference to the element at the given global
   *           coordinates.
   */
  template<typename ... Args>
  const_reference at(
    Args... args      ///< Global coordinates
  ) const;

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
    Args... args      ///< Global coordinates
  );

  /**
   * Fortran-style subscript operator, alias for \ref at().
   * As an example, the operation \c matrix(i,j) is equivalent to
   * \c matrix[i][j].
   *
   * \returns  A global reference to the element at the given global
   *           coordinates.
   * \see  at
   */
  template<typename... Args>
  const_reference operator()(
    Args... args      ///< Global coordinates
  ) const;

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
    Args... args      ///< Global coordinates
  );

  /**
   * Whether the element at a global, canonical offset in the matrix
   * is local to the active unit.
   *
   * \see  DashContainerConcept
   */
  constexpr bool is_local(
    size_type g_pos   ///< Canonical offset in global index space.
  ) const;

  /**
   * Whether the element at a global, canonical offset in a specific
   * dimension of the matrix is local to the active unit.
   */
  template<dim_t Dimension>
  constexpr bool is_local(
    size_type g_pos   ///< Linear offset in the selected dimension.
  ) const;

  template <int level>
  dash::HView<self_t, level> hview();

  /**
   * Conversion operator to type \ref MatrixRef.
   */
  operator
    MatrixRef<ElementT, NumDimensions, NumDimensions, PatternT> ();

};

/**
 * Template alias for dash::Matrix with the same default template
 * arguments
 *
 * \see Matrix
 */
template <
  typename T,
  dim_t    NumDimensions,
  typename IndexT   = dash::default_index_t,
  class    PatternT = Pattern<NumDimensions, ROW_MAJOR, IndexT> >
using NArray = dash::Matrix<T, NumDimensions, IndexT, PatternT>;

}  // namespace dash

#include <dash/matrix/internal/Matrix-inl.h>

#endif  // DASH__MATRIX_H_INCLUDED

