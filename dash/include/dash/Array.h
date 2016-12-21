#ifndef ARRAY_H_INCLUDED
#define ARRAY_H_INCLUDED

#include <dash/GlobMem.h>
#include <dash/GlobRef.h>
#include <dash/GlobAsyncRef.h>
#include <dash/Types.h>
#include <dash/Team.h>
#include <dash/Pattern.h>
#include <dash/HView.h>
#include <dash/Shared.h>
#include <dash/Exception.h>
#include <dash/Cartesian.h>
#include <dash/Dimensional.h>

#include <dash/iterator/GlobIter.h>

#include <iterator>
#include <initializer_list>
#include <type_traits>


/**
 * \defgroup  DashArrayConcept  Array Concept
 *
 * \ingroup DashContainerConcept
 * \{
 * \par Description
 *
 * A distributed array of fixed size.
 *
 * Like all DASH containers, \c dash::Array is initialized by specifying
 * an arrangement of units in a team (\c dash::TeamSpec) and a
 * distribution pattern (\c dash::Pattern).
 *
 * DASH arrays support delayed allocation (\c dash::Array::allocate),
 * so global memory of an array instance can be allocated any time after
 * declaring a \c dash::Array variable.
 *
 * \par Types
 *
 * Type name                       | Description
 * ------------------------------- | --------------------------------------------------------------------------------------------------------------------
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
 * <tt>view_type</tt>              | View specifier on container elements, model of \c DashViewConcept.
 * <tt>local_type</tt>             | Reference to local element range, allows range-based iteration.
 * <tt>pattern_type</tt>           | Concrete model of the Pattern concept that specifies the container's data distribution and cartesian access pattern.
 *
 * \par Methods
 *
 * Return Type              | Method                | Parameters                                            | Description
 * ------------------------ | --------------------- | ----------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------
 * <tt>local_type</tt>      | <tt>local</tt>        | &nbsp;                                                | Container proxy object representing a view specifier on the container's local elements.
 * <tt>pattern_type</tt>    | <tt>pattern</tt>      | &nbsp;                                                | Object implementing the Pattern concept specifying the container's data distribution and iteration pattern.
 * <tt>iterator</tt>        | <tt>begin</tt>        | &nbsp;                                                | Iterator referencing the first container element.
 * <tt>iterator</tt>        | <tt>end</tt>          | &nbsp;                                                | Iterator referencing the element past the last container element.
 * <tt>Element *</tt>       | <tt>lbegin</tt>       | &nbsp;                                                | Native pointer referencing the first local container element, same as <tt>local().begin()</tt>.
 * <tt>Element *</tt>       | <tt>lend</tt>         | &nbsp;                                                | Native pointer referencing the element past the last local container element, same as <tt>local().end()</tt>.
 * <tt>size_type</tt>       | <tt>size</tt>         | &nbsp;                                                | Number of elements in the container.
 * <tt>size_type</tt>       | <tt>local_size</tt>   | &nbsp;                                                | Number of local elements in the container, same as <tt>local().size()</tt>.
 * <tt>bool</tt>            | <tt>is_local</tt>     | <tt>index_type gi</tt>                                | Whether the element at the given linear offset in global index space <tt>gi</tt> is local.
 * <tt>bool</tt>            | <tt>allocate</tt>     | <tt>size_type n, DistributionSpec<DD> ds, Team t</tt> | Allocation of <tt>n</tt> container elements distributed in Team <tt>t</tt> as specified by distribution spec <tt>ds</tt>
 * <tt>void</tt>            | <tt>deallocate</tt>   | &nbsp;                                                | Deallocation of the container and its elements.
 *
 * \}
 *
 */

namespace dash {

// forward declaration
template<
  typename ElementType,
  typename IndexType,
  class    PatternType >
class Array;

/**
 * Proxy type representing local access to elements in a \c dash::Array.
 *
 * \concept{DashArrayConcept}
 */
template<
  typename T,
  typename IndexType,
  class    PatternType >
class LocalArrayRef
{
private:
  static const dim_t NumDimensions = 1;

  typedef LocalArrayRef<T, IndexType, PatternType>
    self_t;
  typedef Array<T, IndexType, PatternType>
    Array_t;
  typedef ViewSpec<NumDimensions, IndexType>
    ViewSpec_t;
  typedef std::array<typename PatternType::size_type, NumDimensions>
    Extents_t;

public:
  template <typename T_, typename I_, typename P_>
    friend class LocalArrayRef;

public:
  typedef T                                                  value_type;

  typedef typename std::make_unsigned<IndexType>::type        size_type;
  typedef IndexType                                          index_type;

  typedef IndexType                                     difference_type;

  typedef T &                                                 reference;
  typedef const T &                                     const_reference;

  typedef T *                                                   pointer;
  typedef const T *                                       const_pointer;

public:
  /// Type alias for LocalArrayRef<T,I,P>::view_type
  typedef LocalArrayRef<T, IndexType, PatternType>
    View;

public:
  /**
   * Constructor, creates a local access proxy for the given array.
   */
  LocalArrayRef(
    Array<T, IndexType, PatternType> * array)
  : _array(array)
  { }

  LocalArrayRef(
    /// Pointer to array instance referenced by this view.
    Array<T, IndexType, PatternType> * array,
    /// The view's offset and extent within the referenced array.
    const ViewSpec_t & viewspec)
  : _array(array),
    _viewspec(viewspec)
  { }

  /**
   * Pointer to initial local element in the array.
   */
  inline const_pointer begin() const noexcept {
    return _array->m_lbegin;
  }

  /**
   * Pointer to initial local element in the array.
   */
  inline pointer begin() noexcept {
    return _array->m_lbegin;
  }

  /**
   * Pointer past final local element in the array.
   */
  inline const_pointer end() const noexcept {
    return _array->m_lend;
  }

  /**
   * Pointer past final local element in the array.
   */
  inline pointer end() noexcept {
    return _array->m_lend;
  }

  /**
   * Number of array elements in local memory.
   */
  inline size_type size() const noexcept {
    return end() - begin();
  }

  /**
   * Subscript operator, access to local array element at given position.
   */
  inline value_type operator[](const size_t n) const {
    return (_array->m_lbegin)[n];
  }

  /**
   * Subscript operator, access to local array element at given position.
   */
  inline reference operator[](const size_t n) {
    return (_array->m_lbegin)[n];
  }

  /**
   * Checks whether the given global index is local to the calling unit.
   *
   * \return  True
   */
  constexpr bool is_local(
    /// A global array index
    index_type global_index) const {
    return true;
  }

  /**
   * View at block at given global block offset.
   */
  self_t block(index_type block_lindex)
  {
    DASH_LOG_TRACE("LocalArrayRef.block()", block_lindex);
    ViewSpec<1> block_view = pattern().local_block(block_lindex);
    DASH_LOG_TRACE("LocalArrayRef.block >", block_view);
    return self_t(_array, block_view);
  }

  /**
   * The pattern used to distribute array elements to units.
   */
  inline const PatternType & pattern() const {
    return _array->pattern();
  }

private:
  /// Pointer to array instance referenced by this view.
  Array_t * const _array;
  /// The view's offset and extent within the referenced array.
  ViewSpec_t      _viewspec;
};

#ifndef DOXYGEN

template<
  typename T,
  typename IndexType,
  class    PatternType >
class AsyncArrayRef
{
private:
  typedef AsyncArrayRef<T, IndexType, PatternType>
    self_t;

public:
  template <typename T_, typename I_, typename P_>
    friend class AsyncArrayRef;

public:
  typedef T                                                  value_type;
  typedef typename std::make_unsigned<IndexType>::type        size_type;
  typedef IndexType                                     difference_type;

  typedef T &                                                 reference;
  typedef const T &                                     const_reference;

  typedef T *                                                   pointer;
  typedef const T *                                       const_pointer;

  typedef GlobAsyncRef<T>                               async_reference;
  typedef const GlobAsyncRef<T>                   const_async_reference;

private:
  Array<T, IndexType, PatternType> * const _array;

public:
  /**
   * Constructor, creates a local access proxy for the given array.
   */
  AsyncArrayRef(
    Array<T, IndexType, PatternType> * const array)
  : _array(array) {
  }

  /**
   * Pointer to initial local element in the array.
   *
   * TODO: Should return GlobAsyncPtr<...>(_array->begin())
   */
  inline const_pointer begin() const noexcept {
    return _array->m_begin;
  }

  /**
   * Pointer to initial local element in the array.
   *
   * TODO: Should return GlobAsyncPtr<...>(_array->begin())
   */
  inline pointer begin() noexcept {
    return _array->m_begin;
  }

  /**
   * Pointer past final local element in the array.
   *
   * TODO: Should return GlobAsyncPtr<...>(_array->end())
   */
  inline const_pointer end() const noexcept {
    return _array->m_end;
  }

  /**
   * Pointer past final local element in the array.
   *
   * TODO: Should return GlobAsyncPtr<...>(_array->end())
   */
  inline pointer end() noexcept {
    return _array->m_end;
  }

  /**
   * Number of array elements in local memory.
   */
  inline size_type size() const noexcept {
    return _array->size();
  }

  /**
   * Subscript operator, access to local array element at given position.
   */
  inline const_async_reference operator[](const size_t n) const  {
    return async_reference(
             _array->m_globmem,
             (*(_array->begin() + n)).dart_gptr());
  }

  /**
   * Subscript operator, access to local array element at given position.
   */
  async_reference operator[](const size_t n) {
    return async_reference(
             _array->m_globmem,
             (*(_array->begin() + n)).dart_gptr());
  }

  /**
   * Complete all outstanding asynchronous operations on the referenced array
   * on all units.
   */
  void flush() {
    DASH_LOG_TRACE("AsyncArrayRef.flush()");
    // could also call _array->flush();
    _array->m_globmem->flush();
  }

  void flush_local() {
    DASH_LOG_TRACE("AsyncArrayRef.flush_local()");
    // could also call _array->flush_local();
    _array->m_globmem->flush_local();
  }

  void flush_all() {
    DASH_LOG_TRACE("AsyncArrayRef.flush()");
    // could also call _array->flush();
    _array->m_globmem->flush_all();
  }

  void flush_local_all() {
    DASH_LOG_TRACE("AsyncArrayRef.flush_local_all()");
    // could also call _array->flush_local_all();
    _array->m_globmem->flush_local_all();
  }

  /**
   * Block until all locally invoked operations on global memory have been
   * communicated.
   *
   * \see DashAsyncProxyConcept
   */
  void push() {
    flush_local_all();
  }

  /**
   * Block until all remote operations on this unit's local memory have been
   * completed.
   *
   * \see DashAsyncProxyConcept
   */
  void fetch() {
    flush_all();
  }
};

#endif // DOXYGEN

/**
 * Proxy type representing a view specifier on elements in a
 * \c dash::Array.
 *
 * \concept{DashArrayConcept}
 */
template<
  typename T,
  class    PatternT >
class ArrayRefView
{
 public:
  typedef typename PatternT::index_type             index_type;

 private:
  Array<T, index_type, PatternT>                  * _arr;
  ViewSpec<1, index_type>                           _viewspec;
};

/**
 * Proxy type representing an access modifier on elements in a
 * \c dash::Array.
 *
 * \concept{DashArrayConcept}
 */
template<
  typename ElementType,
  typename IndexType,
  class    PatternType>
class ArrayRef
{
private:
  static const dim_t NumDimensions = 1;

  typedef ArrayRef<ElementType, IndexType, PatternType>
    self_t;
  typedef Array<ElementType, IndexType, PatternType>
    Array_t;
  typedef ViewSpec<NumDimensions, IndexType>
    ViewSpec_t;
  typedef std::array<typename PatternType::size_type, NumDimensions>
    Extents_t;

/// Public types as required by iterator concept
public:
  typedef ElementType                                             value_type;
  typedef IndexType                                               index_type;
  typedef typename std::make_unsigned<IndexType>::type             size_type;
  typedef typename std::make_unsigned<IndexType>::type       difference_type;

  typedef       GlobIter<value_type, PatternType>                   iterator;
  typedef const GlobIter<value_type, PatternType>             const_iterator;
  typedef       std::reverse_iterator<      iterator>       reverse_iterator;
  typedef       std::reverse_iterator<const_iterator> const_reverse_iterator;

  typedef       GlobRef<value_type>                                reference;
  typedef const GlobRef<value_type>                          const_reference;

  typedef       GlobIter<value_type, PatternType>                    pointer;
  typedef const GlobIter<value_type, PatternType>              const_pointer;

/// Public types as required by dash container concept
public:
  /// The type of the pattern used to distribute array elements to units
  typedef PatternType
    pattern_type;
  typedef ArrayRef<ElementType, IndexType, PatternType>
    view_type;
  typedef LocalArrayRef<value_type, IndexType, PatternType>
    local_type;
  typedef AsyncArrayRef<value_type, IndexType, PatternType>
    async_type;
  /// Type alias for Array<T,I,P>::local_type
  typedef LocalArrayRef<value_type, IndexType, PatternType>
    Local;
  /// Type alias for Array<T,I,P>::view_type
  typedef ArrayRef<ElementType, IndexType, PatternType>
    View;

public:
  ArrayRef(
    /// Pointer to array instance referenced by this view.
    Array_t          * array,
    /// The view's offset and extent within the referenced array.
    const ViewSpec_t & viewspec)
  : _arr(array),
    _viewspec(viewspec)
  { }

public:
  inline    Team              & team();

  constexpr size_type           size()             const noexcept;
  constexpr size_type           local_size()       const noexcept;
  constexpr size_type           local_capacity()   const noexcept;
  constexpr size_type           extent(dim_t dim)  const noexcept;
  constexpr Extents_t           extents()          const noexcept;
  constexpr bool                empty()            const noexcept;

  inline    void                barrier()          const;

  inline    const_pointer       data()             const noexcept;
  inline    iterator            begin()                  noexcept;
  inline    const_iterator      begin()            const noexcept;
  inline    iterator            end()                    noexcept;
  inline    const_iterator      end()              const noexcept;
  /// View representing elements in the active unit's local memory.
  inline    local_type          sub_local()              noexcept;
  /// Pointer to first element in local range.
  inline    ElementType       * lbegin()           const noexcept;
  /// Pointer past final element in local range.
  inline    ElementType       * lend()             const noexcept;

  reference operator[](
    /// The position of the element to return
    size_type global_index)
  {
    DASH_LOG_TRACE("ArrayRef.[]=", global_index);
    return _arr->_begin[global_index];
  }

  const_reference operator[](
    /// The position of the element to return
    size_type global_index) const
  {
    DASH_LOG_TRACE("ArrayRef.[]", global_index);
    return _arr->_begin[global_index];
  }

  reference at(
    /// The position of the element to return
    size_type global_pos)
  {
    if (global_pos >= size()) {
      DASH_THROW(
          dash::exception::OutOfRange,
          "Position " << global_pos
          << " is out of range " << size()
          << " in ArrayRef.at()" );
    }
    return _arr->_begin[global_pos];
  }

  const_reference at(
    /// The position of the element to return
    size_type global_pos) const
  {
    if (global_pos >= size()) {
      DASH_THROW(
          dash::exception::OutOfRange,
          "Position " << global_pos
          << " is out of range " << size()
          << " in ArrayRef.at()" );
    }
    return _arr->_begin[global_pos];
  }

  /**
   * The pattern used to distribute array elements to units.
   */
  inline const PatternType & pattern() const {
    return _arr->pattern();
  }

private:
  /// Pointer to array instance referenced by this view.
  Array_t    * _arr;
  /// The view's offset and extent within the referenced array.
  ViewSpec_t   _viewspec;

}; // class ArrayRef

/**
 * A distributed array.
 *
 * \concept{DashArrayConcept}
 *
 * \todo  Add template parameter:
 *        <tt>class GlobMemType = dash::GlobMem<ElementType></tt>
 *
 * \note: Template parameter IndexType could be deduced from pattern
 *        type <tt>PatternT::index_type</tt>
 */
template<
  typename ElementType,
  typename IndexType    = dash::default_index_t,
  /// Pattern type used to distribute array elements among units.
  /// Default is \c dash::BlockPattern<1, ROW_MAJOR> as it supports all
  /// distribution types (BLOCKED, TILE, BLOCKCYCLIC, CYCLIC).
  class    PatternType  = Pattern<1, ROW_MAJOR, IndexType>
>
class Array
{
private:
  typedef Array<ElementType, IndexType, PatternType> self_t;

/// Public types as required by iterator concept
public:
  typedef ElementType                                             value_type;
  typedef IndexType                                               index_type;
  typedef typename std::make_unsigned<IndexType>::type             size_type;
  typedef typename std::make_unsigned<IndexType>::type       difference_type;

  typedef       GlobIter<value_type, PatternType>                   iterator;
  typedef const GlobIter<value_type, PatternType>             const_iterator;
  typedef       std::reverse_iterator<      iterator>       reverse_iterator;
  typedef       std::reverse_iterator<const_iterator> const_reverse_iterator;

  typedef       GlobRef<value_type>                                reference;
  typedef const GlobRef<value_type>                          const_reference;

  typedef       GlobIter<value_type, PatternType>                    pointer;
  typedef const GlobIter<value_type, PatternType>              const_pointer;

  typedef dash::GlobMem<value_type>                            glob_mem_type;

public:
  template<
    typename T_,
    typename I_,
    class P_>
  friend class LocalArrayRef;
  template<
    typename T_,
    typename I_,
    class P_>
  friend class AsyncArrayRef;

/// Public types as required by dash container concept
public:
  /// The type of the pattern used to distribute array elements to units
  typedef PatternType
    pattern_type;
  typedef LocalArrayRef<value_type, IndexType, PatternType>
    local_type;
  typedef AsyncArrayRef<value_type, IndexType, PatternType>
    async_type;

  typedef LocalArrayRef<value_type, IndexType, PatternType>
    Local;
  typedef ArrayRef<ElementType, IndexType, PatternType>
    View;

private:
  typedef DistributionSpec<1>
    DistributionSpec_t;
  typedef SizeSpec<1, size_type>
    SizeSpec_t;

public:
  /// Local proxy object, allows use in range-based for loops.
  local_type           local;
  /// Proxy object, provides non-blocking operations on array.
  async_type           async;

public:
/*
   Check requirements on element type
   is_trivially_copyable is not implemented presently, and is_trivial
   is too strict (e.g. fails on std::pair).

   static_assert(std::is_trivially_copyable<ElementType>::value,
     "Element type must be trivially copyable");
   static_assert(std::is_trivial<ElementType>::value,
     "Element type must be trivially copyable");
*/

public:
  /**
   * Default constructor, for delayed allocation.
   *
   * Sets the associated team to DART_TEAM_NULL for global array instances
   * that are declared before \c dash::Init().
   */
  Array(
    Team & team = dash::Team::Null())
  : local(this),
    async(this),
    m_team(&team),
    m_pattern(
      SizeSpec_t(0),
      DistributionSpec_t(dash::BLOCKED),
      team),
    m_globmem(nullptr),
    m_size(0),
    m_lsize(0),
    m_lcapacity(0),
    m_lbegin(nullptr),
    m_lend(nullptr)
  {
    DASH_LOG_TRACE("Array() >", "finished default constructor");
  }

  /**
   * Constructor, specifies the array's global capacity and distribution.
   */
  Array(
    size_type                  nelem,
    const DistributionSpec_t & distribution,
    Team                     & team = dash::Team::All())
  : local(this),
    async(this),
    m_team(&team),
    m_pattern(
      SizeSpec_t(nelem),
      distribution,
      team),
    m_size(0),
    m_lsize(0),
    m_lcapacity(0)
  {
    DASH_LOG_TRACE("Array(nglobal,dist,team)()", "size:", nelem);
    allocate(m_pattern);
    DASH_LOG_TRACE("Array(nglobal,dist,team) >");
  }

  /**
   * Delegating constructor, specifies the array's global capacity.
   */
  Array(
    size_type   nelem,
    Team      & team = dash::Team::All())
  : Array(nelem, dash::BLOCKED, team)
  {
    DASH_LOG_TRACE("Array(nglobal,team) >",
                   "finished delegating constructor");
  }

  /**
   * Constructor, specifies the array's global capacity, values of local
   * elements and distribution.
   */
  Array(
    size_type                           nelem,
    std::initializer_list<value_type>   local_elements,
    const DistributionSpec_t          & distribution,
    Team                              & team = dash::Team::All())
  : local(this),
    async(this),
    m_team(&team),
    m_myid(team.myid()),
    m_pattern(
      SizeSpec_t(nelem),
      distribution,
      team),
    m_size(0),
    m_lsize(0),
    m_lcapacity(0)
  {
    DASH_LOG_TRACE("Array(nglobal,lvals,dist,team)()",
                   "size:",   nelem,
                   "nlocal:", local_elements.size());
    allocate(m_pattern, local_elements);
    DASH_LOG_TRACE("Array(nglobal,lvals,dist,team) >");
  }

  /**
   * Delegating constructor, specifies the array's global capacity and values
   * of local elements.
   */
  Array(
    size_type                           nelem,
    std::initializer_list<value_type>   local_elements,
    Team                              & team = dash::Team::All())
  : Array(nelem, local_elements, dash::BLOCKED, team)
  {
    DASH_LOG_TRACE("Array(nglobal,lvals,team) >",
                   "finished delegating constructor");
  }

  /**
   * Constructor, specifies distribution pattern explicitly.
   */
  Array(
    const PatternType & pattern)
  : local(this),
    async(this),
    m_team(&pattern.team()),
    m_myid(m_team->myid()),
    m_pattern(pattern),
    m_size(0),
    m_lsize(0),
    m_lcapacity(0)
  {
    DASH_LOG_TRACE("Array()", "pattern instance constructor");
    allocate(m_pattern);
  }

  /**
   * Copy constructor is deleted to prevent unintentional copies of - usually
   * huge - distributed arrays.
   *
   * To create a copy of a \c dash::Array instance, instantiate the copy
   * instance explicitly and use \c dash::copy to clone elements.
   *
   * Example:
   *
   * \code
   * dash::Array<int> a1(1024 * dash::size());
   * dash::fill(a1.begin(), a1.end(), 123);
   * 
   * // create copy of array a1:
   * dash::Array<int> a2(a1.size());
   * dash::copy(a1.begin(), a1.end(), a2.begin());
   * \endcode
   */
  Array(const self_t & other) = delete;

  /**
   * Move constructor is deleted as move semantics are non-trivial for
   * distributed arrays.
   */
  Array(self_t && other) = delete;

  /**
   * Destructor, deallocates array elements.
   */
  ~Array()
  {
    DASH_LOG_TRACE_VAR("Array.~Array()", this);
    deallocate();
    DASH_LOG_TRACE_VAR("Array.~Array >", this);
  }

  /**
   * Move assignment operator is deleted as move semantics are non-trivial
   * for distributed arrays.
   */
  self_t & operator=(self_t && other) = delete;

  /**
   * Assignment operator is deleted to prevent unintentional copies of
   * - usually huge - distributed arrays.
   *
   * To create a copy of a \c dash::Array instance, instantiate the copy
   * instance explicitly and use \c dash::copy to clone elements.
   *
   * Example:
   *
   * \code
   * dash::Array<int> a1(1024 * dash::size());
   * dash::fill(a1.begin(), a1.end(), 123);
   * 
   * // create copy of array a1:
   * dash::Array<int> a2(a1.size());
   * dash::copy(a1.begin(), a1.end(), a2.begin());
   * \endcode
   */
  self_t & operator=(const self_t & rhs) = delete;

  /**
   * View at block at given global block offset.
   */
  View block(index_type block_gindex)
  {
    DASH_LOG_TRACE("Array.block()", block_gindex);
    ViewSpec<1> block_view = pattern().block(block_gindex);
    DASH_LOG_TRACE("Array.block >", block_view);
    return View(this, block_view);
  }

  /**
   * Global const pointer to the beginning of the array.
   */
  const_pointer data() const noexcept
  {
    return m_begin;
  }

  /**
   * Global pointer to the beginning of the array.
   */
  iterator begin() noexcept
  {
    return m_begin;
  }

  /**
   * Global pointer to the beginning of the array.
   */
  const_iterator begin() const noexcept
  {
    return m_begin;
  }

  /**
   * Global pointer to the end of the array.
   */
  iterator end() noexcept
  {
    return m_end;
  }

  /**
   * Global pointer to the end of the array.
   */
  const_iterator end() const noexcept
  {
    return m_end;
  }

  /**
   * Native pointer to the first local element in the array.
   */
  ElementType * lbegin() const noexcept
  {
    return m_lbegin;
  }

  /**
   * Native pointer to the end of the array.
   */
  ElementType * lend() const noexcept
  {
    return m_lend;
  }

  /**
   * Subscript assignment operator, not range-checked.
   *
   * \return  A global reference to the element in the array at the given
   *          index.
   */
  reference operator[](
    /// The position of the element to return
    size_type global_index)
  {
    DASH_LOG_TRACE_VAR("Array.[]=()", global_index);
    auto global_ref = m_begin[global_index];
    DASH_LOG_TRACE_VAR("Array.[]= >", global_ref);
    return global_ref;
  }

  /**
   * Subscript operator, not range-checked.
   *
   * \return  A global reference to the element in the array at the given
   *          index.
   */
  const_reference operator[](
    /// The position of the element to return
    size_type global_index) const
  {
    DASH_LOG_TRACE_VAR("Array.[]()", global_index);
    auto global_ref = m_begin[global_index];
    DASH_LOG_TRACE_VAR("Array.[] >", global_ref);
    return global_ref;
  }

  /**
   * Random access assignment operator, range-checked.
   *
   * \see operator[]
   *
   * \return  A global reference to the element in the array at the given
   *          index.
   */
  reference at(
    /// The position of the element to return
    size_type global_pos)
  {
    if (global_pos >= size())  {
      DASH_THROW(
          dash::exception::OutOfRange,
          "Position " << global_pos
          << " is out of range " << size()
          << " in Array.at()" );
    }
    return m_begin[global_pos];
  }

  /**
   * Random access operator, range-checked.
   *
   * \see operator[]
   *
   * \return  A global reference to the element in the array at the given
   *          index.
   */
  const_reference at(
    /// The position of the element to return
    size_type global_pos) const
  {
    if (global_pos >= size())  {
      DASH_THROW(
          dash::exception::OutOfRange,
          "Position " << global_pos
          << " is out of range " << size()
          << " in Array.at()" );
    }
    return m_begin[global_pos];
  }

  /**
   * The size of the array.
   *
   * \return  The number of elements in the array.
   */
  inline size_type size() const noexcept
  {
    return m_size;
  }

  /**
   * The number of elements that can be held in currently allocated storage
   * of the array.
   *
   * \return  The number of elements in the array.
   */
  inline size_type capacity() const noexcept
  {
    return m_size;
  }

  /**
   * The team containing all units accessing this array.
   *
   * \return  The instance of Team that this array has been instantiated
   *          with
   */
  inline const Team & team() const noexcept
  {
    return *m_team;
  }

  /**
   * The number of elements in the local part of the array.
   *
   * \return  The number of elements in the array that are local to the
   *          calling unit.
   */
  inline size_type lsize() const noexcept
  {
    return m_lsize;
  }

  /**
   * The capacity of the local part of the array.
   *
   * \return  The number of allocated elements in the array that are local
   *          to the calling unit.
   */
  inline size_type lcapacity() const noexcept
  {
    return m_lcapacity;
  }

  /**
   * Checks whether the array is empty.
   *
   * \return  True if \c size() is 0, otherwise false
   */
  inline bool empty() const noexcept
  {
    return size() == 0;
  }

  inline View local_in(dash::util::Locality::Scope scope)
  {
    return View(); // TODO
  }

  /**
   * Checks whether the given global index is local to the calling unit.
   *
   * \return  True if the array element referenced by the index is held
   *          in the calling unit's local memory
   */
  bool is_local(
    /// A global array index
    index_type global_index) const
  {
    return m_pattern.is_local(global_index, m_myid);
  }

  /**
   * Establish a barrier for all units operating on the array, publishing all
   * changes to all units.
   */
  void barrier() const
  {
    DASH_LOG_TRACE_VAR("Array.barrier()", m_team);
    if (nullptr != m_globmem) {
      m_globmem->flush_all();
    }
    if (nullptr != m_team && *m_team != dash::Team::Null()) {
      m_team->barrier();
    }
    DASH_LOG_TRACE("Array.barrier >", "passed barrier");
  }

  /**
   * The pattern used to distribute array elements to units.
   */
  inline const PatternType & pattern() const
  {
    return m_pattern;
  }

  template<int level>
  dash::HView<self_t, level> hview()
  {
    return dash::HView<self_t, level>(*this);
  }

  bool allocate(
    size_type                   nelem,
    dash::DistributionSpec<1>   distribution,
    dash::Team                & team = dash::Team::All())
  {
    DASH_LOG_TRACE_VAR("Array.allocate(nlocal,ds,team)", nelem);
    DASH_LOG_TRACE_VAR("Array.allocate", m_team->dart_id());
    DASH_LOG_TRACE_VAR("Array.allocate", team.dart_id());
    // Check requested capacity:
    if (nelem == 0) {
      DASH_LOG_WARN("Array.allocate", "allocating dash::Array with size 0");
    }
    if (m_team == nullptr || *m_team == dash::Team::Null()) {
      DASH_LOG_TRACE("Array.allocate",
                     "initializing with specified team -",
                     "team size:", team.size());
      m_team    = &team;
      m_pattern = PatternType(nelem, distribution, team);
      DASH_LOG_TRACE_VAR("Array.allocate", team.dart_id());
      DASH_LOG_TRACE_VAR("Array.allocate", m_pattern.team().dart_id());
    } else {
      DASH_LOG_TRACE("Array.allocate",
                     "initializing pattern with initial team");
      m_pattern = PatternType(nelem, distribution, *m_team);
    }
    bool ret = allocate(m_pattern);
    DASH_LOG_TRACE("Array.allocate(nlocal,ds,team) >");
    return ret;
  }

  bool allocate(
    size_type                           nelem,
    std::initializer_list<value_type>   local_elements,
    dash::DistributionSpec<1>           distribution,
    dash::Team                        & team = dash::Team::All())
  {
    DASH_LOG_TRACE_VAR("Array.allocate(lvals,ds,team)",
                       local_elements.size());
    DASH_LOG_TRACE_VAR("Array.allocate", m_team->dart_id());
    DASH_LOG_TRACE_VAR("Array.allocate", team.dart_id());
    // Check requested capacity:
    if (nelem == 0) {
      DASH_THROW(
        dash::exception::InvalidArgument,
        "Tried to allocate dash::Array with size 0");
    }
    if (m_team == nullptr || *m_team == dash::Team::Null()) {
      DASH_LOG_TRACE("Array.allocate",
                     "initializing pattern with Team::All()");
      m_team    = &team;
      m_pattern = PatternType(nelem, distribution, team);
      DASH_LOG_TRACE_VAR("Array.allocate", team.dart_id());
      DASH_LOG_TRACE_VAR("Array.allocate", m_pattern.team().dart_id());
    } else {
      DASH_LOG_TRACE("Array.allocate",
                     "initializing pattern with initial team");
      m_pattern = PatternType(nelem, distribution, *m_team);
    }
    bool ret = allocate(m_pattern, local_elements);
    DASH_LOG_TRACE("Array.allocate(lvals,ds,team) >");
    return ret;
  }

  void deallocate()
  {
    DASH_LOG_TRACE_VAR("Array.deallocate()", this);
    DASH_LOG_TRACE_VAR("Array.deallocate()", m_size);
    // Assure all units are synchronized before deallocation, otherwise
    // other units might still be working on the array:
    if (dash::is_initialized()) {
      barrier();
    }
    // Remove this function from team deallocator list to avoid
    // double-free:
    m_team->unregister_deallocator(
      this, std::bind(&Array::deallocate, this));
    // Actual destruction of the array instance:
    DASH_LOG_TRACE_VAR("Array.deallocate()", m_globmem);
    if (m_globmem != nullptr) {
      delete m_globmem;
      m_globmem = nullptr;
    }
    m_size = 0;
    DASH_LOG_TRACE_VAR("Array.deallocate >", this);
  }

  bool allocate(const PatternType & pattern)
  {
		DASH_LOG_TRACE("Array._allocate()", "pattern",
                   pattern.memory_layout().extents());
    if (&m_pattern != &pattern) {
      DASH_LOG_TRACE("Array.allocate()", "using specified pattern");
      m_pattern = pattern;
    }
    // Check requested capacity:
    m_size      = m_pattern.capacity();
    m_team      = &(m_pattern.team());
    if (m_size == 0) {
      DASH_LOG_WARN("Array.allocate", "allocating dash::Array with size 0");
    }
    // Initialize members:
    m_lsize     = m_pattern.local_size();
    m_lcapacity = m_pattern.local_capacity();
    m_myid      = m_team->myid();
    // Allocate local memory of identical size on every unit:
    DASH_LOG_TRACE_VAR("Array._allocate", m_lcapacity);
    DASH_LOG_TRACE_VAR("Array._allocate", m_lsize);
    m_globmem   = new glob_mem_type(m_lcapacity, *m_team);
    // Global iterators:
    m_begin     = iterator(m_globmem, m_pattern);
    m_end       = iterator(m_begin) + m_size;
    // Local iterators:
    m_lbegin    = m_globmem->lbegin();
    // More efficient than using m_globmem->lend as this a second mapping
    // of the local memory segment:
    m_lend      = m_lbegin + m_lsize;
    DASH_LOG_TRACE_VAR("Array._allocate", m_myid);
    DASH_LOG_TRACE_VAR("Array._allocate", m_size);
    DASH_LOG_TRACE_VAR("Array._allocate", m_lsize);
    // Register deallocator of this array instance at the team
    // instance that has been used to initialized it:
    m_team->register_deallocator(
      this, std::bind(&Array::deallocate, this));
    // Assure all units are synchronized after allocation, otherwise
    // other units might start working on the array before allocation
    // completed at all units:
    if (dash::is_initialized()) {
      DASH_LOG_TRACE("Array._allocate",
                     "waiting for allocation of all units");
      m_team->barrier();
    }
    DASH_LOG_TRACE("Array._allocate >", "finished");
    return true;
  }

private:

#if 0
  typename std::enable_if<
    std::is_move_constructible<value_type>::value &&
    std::is_move_assignable<value_type>::value,
    bool
  >::type
#else
  bool
#endif
  allocate(
    const PatternType                 & pattern,
    std::initializer_list<value_type>   local_elements)
  {
    DASH_LOG_TRACE("Array._allocate()", "pattern",
                   pattern.memory_layout().extents());
    // Check requested capacity:
    m_size      = pattern.capacity();
    m_team      = &pattern.team();
    if (m_size == 0) {
      DASH_THROW(
        dash::exception::InvalidArgument,
        "Tried to allocate dash::Array with size 0");
    }
    // Initialize members:
    m_lsize     = pattern.local_size();
    m_lcapacity = pattern.local_capacity();
    m_myid      = pattern.team().myid();
    // Allocate local memory of identical size on every unit:
    DASH_LOG_TRACE_VAR("Array._allocate", m_lcapacity);
    DASH_LOG_TRACE_VAR("Array._allocate", m_lsize);
    m_globmem   = new glob_mem_type(local_elements, *m_team);
    // Global iterators:
    m_begin     = iterator(m_globmem, pattern);
    m_end       = iterator(m_begin) + m_size;
    // Local iterators:
    m_lbegin    = m_globmem->lbegin();
    // More efficient than using m_globmem->lend as this a second mapping
    // of the local memory segment:
    m_lend      = m_lbegin + pattern.local_size();
    DASH_LOG_TRACE_VAR("Array._allocate", m_myid);
    DASH_LOG_TRACE_VAR("Array._allocate", m_size);
    DASH_LOG_TRACE_VAR("Array._allocate", m_lsize);
    // Register deallocator of this array instance at the team
    // instance that has been used to initialized it:
    m_team->register_deallocator(
      this, std::bind(&Array::deallocate, this));
    // Assure all units are synchronized after allocation, otherwise
    // other units might start working on the array before allocation
    // completed at all units:
    if (dash::is_initialized()) {
      DASH_LOG_TRACE("Array._allocate",
                     "waiting for allocation of all units");
      m_team->barrier();
    }
    DASH_LOG_TRACE("Array._allocate >", "finished");
    return true;
  }

private:
  /// Team containing all units interacting with the array
  dash::Team         * m_team      = nullptr;
  /// DART id of the unit that created the array
  team_unit_t          m_myid;
  /// Element distribution pattern
  PatternType          m_pattern;
  /// Global memory allocation and -access
  glob_mem_type      * m_globmem   = nullptr;
  /// Iterator to initial element in the array
  iterator             m_begin;
  /// Iterator to final element in the array
  iterator             m_end;
  /// Total number of elements in the array
  size_type            m_size;
  /// Number of local elements in the array
  size_type            m_lsize;
  /// Number allocated local elements in the array
  size_type            m_lcapacity;
  /// Native pointer to first local element in the array
  ElementType        * m_lbegin    = nullptr;
  /// Native pointer past last local element in the array
  ElementType        * m_lend      = nullptr;

};

} // namespace dash

#endif /* ARRAY_H_INCLUDED */
