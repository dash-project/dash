#ifndef ARRAY_H_INCLUDED
#define ARRAY_H_INCLUDED

#include <dash/Cartesian.h>
#include <dash/Dimensional.h>
#include <dash/Exception.h>
#include <dash/GlobRef.h>
#include <dash/GlobAsyncRef.h>
#include <dash/HView.h>
#include <dash/Meta.h>
#include <dash/Team.h>
#include <dash/Types.h>

#include <dash/allocator/GlobalAllocator.h>
#include <dash/iterator/GlobIter.h>
#include <dash/memory/MemorySpace.h>
#include <dash/memory/UniquePtr.h>
#include <dash/pattern/BlockPattern1D.h>

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
  * an arrangement of units in a team (\ref dash::TeamSpec) and a
 * distribution pattern (\ref dash::Pattern).
 *
 * DASH arrays support delayed allocation (\ref dash::Array::allocate),
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
 *
 */

namespace dash {

// forward declaration
template<
  typename ElementType,
  typename IndexType,
  class    PatternType,
  class    LocalMemSpaceT >
class Array;

/**
 * Proxy type representing local access to elements in a \c dash::Array.
 *
 * \todo  Expression \c dash::index(dash:begin(dash:local(array))) should
 *        be valid; requires \c dash::LocalArrayRef<T,...>::pointer to
 *        provide method \c .pos().
 *
 * \concept{DashArrayConcept}
 */
template<
  typename T,
  typename IndexType,
  class    PatternType,
  typename LocalMemSpaceT>
class LocalArrayRef
{
private:
  static const dim_t NumDimensions = 1;

  typedef LocalArrayRef<T, IndexType, PatternType, LocalMemSpaceT>
    self_t;
  typedef Array<T, IndexType, PatternType, LocalMemSpaceT>
    Array_t;
  typedef ViewSpec<NumDimensions, IndexType>
    ViewSpec_t;
  typedef std::array<typename PatternType::size_type, NumDimensions>
    Extents_t;

public:
  template <typename T_, typename I_, typename P_,typename M_>
    friend class LocalArrayRef;

public:
  typedef T                                                  value_type;

  typedef typename std::make_unsigned<IndexType>::type        size_type;
  typedef IndexType                                          index_type;

  typedef IndexType                                     difference_type;

  typedef       T &                                           reference;
  typedef const T &                                     const_reference;

  typedef       T *                                             pointer;
  typedef const T *                                       const_pointer;

  typedef       T *                                            iterator;
  typedef const T *                                      const_iterator;

public:
  /// Type alias for LocalArrayRef<T,I,P>::view_type
  typedef LocalArrayRef<T, IndexType, PatternType, LocalMemSpaceT>  View;
  typedef self_t                                              local_type;
  typedef PatternType                                       pattern_type;

public:
  typedef std::integral_constant<dim_t, 1>
    rank;

  static constexpr dim_t ndim() {
    return 1;
  }

public:
  /**
   * Constructor, creates a local access proxy for the given array.
   */
  LocalArrayRef(
    const Array_t * array)
  : _array(array)
  { }

  LocalArrayRef(
    /// Pointer to array instance referenced by this view.
    const Array_t * array,
    /// The view's offset and extent within the referenced array.
    const ViewSpec_t & viewspec)
  : _array(array),
    _viewspec(viewspec)
  { }

  LocalArrayRef(const self_t &) = default;
  LocalArrayRef(self_t &&)      = default;

  self_t & operator=(const self_t &) = default;
  self_t & operator=(self_t &&)      = default;

  /**
   * Pointer to initial local element in the array.
   */
  constexpr const_iterator begin() const noexcept {
    return _array->m_lbegin;
  }

  /**
   * Pointer to initial local element in the array.
   */
  inline iterator begin() noexcept {
    return _array->m_lbegin;
  }

  /**
   * Pointer past final local element in the array.
   */
  constexpr const_iterator end() const noexcept {
    return _array->m_lend;
  }

  /**
   * Pointer past final local element in the array.
   */
  inline iterator end() noexcept {
    return _array->m_lend;
  }

  /**
   * Number of array elements in local memory.
   */
  constexpr size_type size() const noexcept {
    return end() - begin();
  }

  /**
   * Subscript operator, access to local array element at given position.
   */
  constexpr const_reference operator[](const size_type n) const {
    return (_array->m_lbegin)[n];
  }

  /**
   * Subscript operator, access to local array element at given position.
   */
  inline reference operator[](const size_type n) {
    return (_array->m_lbegin)[n];
  }

  /**
   * Checks whether the given local index is local to the calling unit.
   *
   * \return  True
   */
  constexpr bool is_local(
    /// A global array index
    index_type local_index) const {
    return true;
  }

  /**
   * View at block at given global block offset.
   */
  constexpr self_t block(index_type block_lindex) const
  {
    return self_t(_array, pattern().local_block(block_lindex));
  }

  /**
   * The pattern used to distribute array elements to units.
   */
  constexpr const PatternType & pattern() const noexcept {
    return _array->pattern();
  }

private:
  /// Pointer to array instance referenced by this view.
  const Array_t * _array;
  /// The view's offset and extent within the referenced array.
  ViewSpec_t      _viewspec;
};

#ifndef DOXYGEN

template<
  typename T,
  typename IndexType,
  class    PatternType,
  typename LocalMemSpaceT>
class AsyncArrayRef
{
private:
  typedef AsyncArrayRef<T, IndexType, PatternType, LocalMemSpaceT>
    self_t;

public:
  template <typename T_, typename I_, typename P_, typename M_>
    friend class AsyncArrayRef;

public:
  typedef T                                                  value_type;
  typedef typename std::make_unsigned<IndexType>::type        size_type;
  typedef IndexType                                     difference_type;

  typedef       T &                                           reference;
  typedef const T &                                     const_reference;

  typedef       T *                                            iterator;
  typedef const T *                                      const_iterator;

  typedef       T *                                             pointer;
  typedef const T *                                       const_pointer;

  typedef          GlobAsyncRef<T>                      async_reference;
  typedef typename GlobAsyncRef<T>::const_type    const_async_reference;

public:
  typedef std::integral_constant<dim_t, 1>
    rank;

  static constexpr dim_t ndim() {
    return 1;
  }

private:
  Array<T, IndexType, PatternType, LocalMemSpaceT> * _array;

public:
  /**
   * Constructor, creates a local access proxy for the given array.
   */
  AsyncArrayRef(
    Array<T, IndexType, PatternType, LocalMemSpaceT> * const array)
  : _array(array) {
  }

  AsyncArrayRef(const self_t &) = default;
  AsyncArrayRef(self_t &&)      = default;

  self_t & operator=(const self_t &) = default;
  self_t & operator=(self_t &&)      = default;


  /**
   * Pointer to initial local element in the array.
   *
   * TODO: Should return GlobAsyncPtr<...>(_array->begin())
   */
  constexpr const_iterator begin() const noexcept {
    return _array->m_begin;
  }

  /**
   * Pointer to initial local element in the array.
   *
   * TODO: Should return GlobAsyncPtr<...>(_array->begin())
   */
  inline iterator begin() noexcept {
    return _array->m_begin;
  }

  /**
   * Pointer past final local element in the array.
   *
   * TODO: Should return GlobAsyncPtr<...>(_array->end())
   */
  constexpr const_iterator end() const noexcept {
    return _array->m_end;
  }

  /**
   * Pointer past final local element in the array.
   *
   * TODO: Should return GlobAsyncPtr<...>(_array->end())
   */
  inline iterator end() noexcept {
    return _array->m_end;
  }

  /**
   * Number of array elements in local memory.
   */
  constexpr size_type size() const noexcept {
    return _array->size();
  }

  /**
   * Subscript operator, access to local array element at given position.
   */
  constexpr const_async_reference operator[](const size_type n) const  {
    return const_async_reference(
             (*(_array->begin() + n)).dart_gptr());
  }

  /**
   * Subscript operator, access to local array element at given position.
   */
  async_reference operator[](const size_type n) {
    return async_reference(
             (*(_array->begin() + n)).dart_gptr());
  }

  /**
   * Complete all outstanding asynchronous operations on the referenced array
   * on all units.
   */
  inline void flush() const {
    _array->flush();
  }

  /**
   * Complete all outstanding asynchronous operations on the referenced array
   * to the specified unit.
   */
  inline void flush(dash::team_unit_t target) const {
    _array->flush(target);
  }

  /**
   * Locally complete all outstanding asynchronous operations on the referenced array
   * on all units.
   */
  inline void flush_local() const {
    _array->flush_local();
  }

  /**
   * Locally complete all outstanding asynchronous operations on the referenced array
   * to the specified unit.
   */
  inline void flush_local(dash::team_unit_t target) const {
    _array->flush_local(target);
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
  class    PatternT,
  typename MSpaceC>
class ArrayRefView
{
 public:
   typedef typename PatternT::index_type index_type;

 private:
   Array<T, index_type, PatternT, MSpaceC> *_arr;
   ViewSpec<1, index_type>                  _viewspec;
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
  class    PatternType,
  typename LocalMemSpaceT>
class ArrayRef
{
private:
  static const dim_t NumDimensions = 1;

  typedef ArrayRef<ElementType, IndexType, PatternType, LocalMemSpaceT>
    self_t;
  typedef Array<ElementType, IndexType, PatternType, LocalMemSpaceT>
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

  typedef GlobIter<value_type, PatternType, typename Array_t::memory_type>
      iterator;

  typedef GlobIter<
      const value_type,
      PatternType,
      typename Array_t::memory_type>
      const_iterator;

  typedef std::reverse_iterator<iterator>       reverse_iterator;
  typedef std::reverse_iterator<const_iterator> const_reverse_iterator;

  typedef GlobRef<value_type>       reference;
  typedef GlobRef<const value_type> const_reference;

  typedef iterator       pointer;
  typedef const_iterator const_pointer;

  /// Public types as required by dash container concept
public:
  /// The type of the pattern used to distribute array elements to units
  typedef PatternType
    pattern_type;
  typedef self_t
    view_type;
  typedef LocalArrayRef<value_type, IndexType, PatternType, LocalMemSpaceT>
    local_type;
  typedef AsyncArrayRef<value_type, IndexType, PatternType, LocalMemSpaceT>
    async_type;

public:
  typedef std::integral_constant<dim_t, 1>
    rank;

  static constexpr dim_t ndim() {
    return 1;
  }

public:
  ArrayRef(
    /// Pointer to array instance referenced by this view.
    const Array_t    * array,
    /// The view's offset and extent within the referenced array.
    const ViewSpec_t & viewspec)
  : _arr(array),
    _viewspec(viewspec),
    _begin(array->begin() + _viewspec.offsets()[0]),
    _end(array->begin()   + _viewspec.offsets()[0] + _viewspec.extents()[0]),
    _size(_viewspec.size())
  { }

public:
  constexpr Team              & team() const {
    return _arr->team();
  }

  constexpr size_type           size()             const noexcept {
    return _size;
  }

  constexpr size_type           local_size()       const noexcept;
  constexpr size_type           local_capacity()   const noexcept;

  constexpr size_type           extent(dim_t dim)  const noexcept {
    return _viewspec.extents()[dim];
  }
  constexpr Extents_t           extents()          const noexcept {
    return _viewspec.extents();
  }
  constexpr bool                empty()            const noexcept {
    return _size == 0;
  }

  inline    void                barrier()          const;

  constexpr const_pointer       data()             const noexcept {
    return _begin;
  }
  inline    iterator            begin()                  noexcept {
    return _begin;
  }
  constexpr const_iterator      begin()            const noexcept {
    return _begin;
  }
  inline    iterator            end()                    noexcept {
    return _end;
  }
  constexpr const_iterator      end()              const noexcept {
    return _end;
  }
  /// View representing elements in the active unit's local memory.
  inline    local_type          sub_local()              noexcept;
  /// Pointer to first element in local range.
  constexpr ElementType       * lbegin()           const noexcept;
  /// Pointer past final element in local range.
  constexpr ElementType       * lend()             const noexcept;

  reference operator[](
    /// The position of the element to return
    size_type global_index)
  {
    DASH_LOG_TRACE("ArrayRef.[]=", global_index);
    return _arr->_begin[global_index];
  }

  constexpr const_reference operator[](
    /// The position of the element to return
    size_type global_index) const
  {
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
  constexpr const PatternType & pattern() const {
    return _arr->pattern();
  }

private:
  /// Pointer to array instance referenced by this view.
  const Array_t * const _arr;
  /// The view's offset and extent within the referenced array.
  ViewSpec_t            _viewspec;
  /// Iterator to initial element in the array
  iterator              _begin;
  /// Iterator to final element in the array
  iterator              _end;
  /// Total number of elements in the array
  size_type             _size;
}; // class ArrayRef

/**
 * A distributed array.
 *
 * \concept{DashArrayConcept}
 *
 * \todo  Add template parameter:
 *        <tt>class GlobMemType = dash::GlobStaticMem<ElementType></tt>
 *
 * \note: Template parameter IndexType could be deduced from pattern
 *        type <tt>PatternT::index_type</tt>
 */
template <
    typename ElementType,
    typename IndexType = dash::default_index_t,
    /// Pattern type used to distribute array elements among units.
    /// Default is \c dash::BlockPattern<1, ROW_MAJOR> as it supports all
    /// distribution types (BLOCKED, TILE, BLOCKCYCLIC, CYCLIC).
    class PatternType       = BlockPattern<1, ROW_MAJOR, IndexType>,
    typename LocalMemSpaceT = HostSpace>
class Array {
  static_assert(
    dash::is_container_compatible<ElementType>::value,
    "Type not supported for DASH containers");

private:
  typedef Array<ElementType, IndexType, PatternType, LocalMemSpaceT> self_t;


  /// Public types as required by iterator concept
public:
  typedef ElementType                                                 value_type;
  typedef IndexType                                                   index_type;
  typedef typename std::make_unsigned<IndexType>::type                 size_type;
  typedef typename std::make_unsigned<IndexType>::type           difference_type;

  typedef dash::GlobStaticMem<LocalMemSpaceT>            memory_type;
  typedef dash::GlobalAllocator<value_type, memory_type> allocator_type;

  typedef GlobIter<      value_type, PatternType, memory_type>        iterator;
  typedef GlobIter<const value_type, PatternType, memory_type>  const_iterator;

  typedef std::reverse_iterator<      iterator>                 reverse_iterator;
  typedef std::reverse_iterator<const_iterator>           const_reverse_iterator;

  typedef          GlobRef<value_type>                                 reference;
  typedef typename GlobRef<value_type>::const_type               const_reference;

  typedef typename iterator::pointer                                     pointer;
  typedef typename pointer::const_type                             const_pointer;

  typedef typename pointer::local_type                             local_pointer;
  typedef typename pointer::const_local_type                 const_local_pointer;

  typedef DistributionSpec<1>                                  distribution_spec;

public:
  template<
    typename T_,
    typename I_,
    class P_,
    typename LM_>
  friend class LocalArrayRef;
  template<
    typename T_,
    typename I_,
    class P_,
    typename M_>
  friend class AsyncArrayRef;

/// Public types as required by dash container concept
public:
  /// The type of the pattern used to distribute array elements to units
  typedef PatternType
    pattern_type;

  typedef LocalArrayRef<value_type, IndexType, PatternType, LocalMemSpaceT>
    local_type;
  typedef ArrayRef<ElementType, IndexType, PatternType, LocalMemSpaceT>
    view_type;
  typedef AsyncArrayRef<value_type, IndexType, PatternType, LocalMemSpaceT>
    async_type;

public:
  typedef std::integral_constant<dim_t, 1>
    rank;

  static constexpr dim_t ndim() {
    return 1;
  }

private:
  typedef SizeSpec<1, size_type>
    SizeSpec_t;

  using unique_gptr_t = decltype(
      dash::allocate_unique<value_type>(allocator_type{}, std::size_t{}));

  using allocator_traits =
      std::allocator_traits<typename memory_type::allocator_type>;

  static constexpr size_t alignment = alignof(value_type);

public:
  /// Local proxy object, allows use in range-based for loops.
  local_type           local;
  /// Proxy object, provides non-blocking operations on array.
  async_type           async;

private:
  /// Team containing all units interacting with the array
  dash::Team *m_team{nullptr};
  /// Element distribution pattern
  PatternType m_pattern{};
  /// Global memory resource
  memory_type m_globmem{};
  /// GlobalAllocator
  allocator_type m_allocator{};
  /// Unique Pointer to a global memory object allocated through m_allocator
  unique_gptr_t m_data{nullptr};
  /// Iterator to initial element in the array
  iterator m_begin{};
  /// Iterator to final element in the array
  iterator m_end{};
  /// Total number of elements in the array
  size_type m_size{};
  /// Number of local elements in the array
  size_type m_lsize{};
  /// Number allocated local elements in the array
  size_type m_lcapacity{};
  /// Native pointer to first local element in the array
  ElementType * m_lbegin{};
  /// Native pointer past last local element in the array
  ElementType * m_lend{};
  /// DART id of the unit that created the array
  team_unit_t m_myid{};
public:
  /**
   * Default constructor, for delayed allocation.
   *
   * Sets the associated team to DART_TEAM_NULL for global array instances
   * that are declared before \c dash::Init().
   */
  explicit constexpr Array(Team &team = dash::Team::Null())
    : local(this)
    , async(this)
    , m_team(&team)
    , m_pattern(SizeSpec_t(0), distribution_spec(dash::BLOCKED), team)
  {
    DASH_LOG_TRACE("Array() >", "finished default constructor");
  }

  /**
   * Constructor, specifies the array's global capacity and distribution.
   */
  Array(
      size_type                nelem,
      const distribution_spec &distribution,
      Team &                   team = dash::Team::All())
    : local(this)
    , async(this)
    , m_team(&team)
    , m_pattern(SizeSpec_t(nelem), distribution, team)
  {
    DASH_LOG_TRACE("Array(nglobal,dist,team)()", "size:", nelem);
    allocate(m_pattern);
    DASH_LOG_TRACE("Array(nglobal,dist,team) >");
  }

  /**
   * Delegating constructor, specifies the array's global capacity.
   */
  explicit Array(size_type nelem, Team &team = dash::Team::All())
    : Array(nelem, dash::BLOCKED, team)
  {
    DASH_LOG_TRACE(
        "Array(nglobal,team) >", "finished delegating constructor");
  }

  /**
   * Constructor, specifies the array's global capacity, values of local
   * elements and distribution.
   */
  Array(
      size_type                         nelem,
      std::initializer_list<value_type> local_elements,
      const distribution_spec &         distribution,
      Team &                            team = dash::Team::All())
    : local(this)
    , async(this)
    , m_team(&team)
    , m_pattern(SizeSpec_t(nelem), distribution, team)
    , m_globmem(*m_team)
    , m_allocator(&m_globmem)
    , m_myid(team.myid())
  {
    DASH_LOG_TRACE("Array(nglobal,lvals,dist,team)()",
                   "size:",   nelem,
                   "nlocal:", local_elements.size());
    allocate(local_elements);
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
  explicit
  Array(
    const PatternType & pattern)
  : local(this),
    async(this),
    m_team(&pattern.team()),
    m_pattern(pattern),
    m_myid(m_team->myid())
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
   * Move construction is supported for the container with the following
   * limitations:
   *
   * The pattern has to be movable or copyable
   * The underlying memory does not have to be movable (it might).
   */
  Array(self_t &&other) noexcept(
      std::is_nothrow_move_constructible<memory_type>::value)
    : local(this)
    , async(this)
    , m_team(other.m_team)
    , m_pattern(std::move(other.m_pattern))
    , m_globmem(std::move(other.m_globmem))
    , m_allocator(&m_globmem)
    , m_data(
          other.m_data.release(),
          typename unique_gptr_t::deleter_type{m_allocator,
                                               m_pattern.local_size()})
    , m_begin(other.m_begin)
    , m_end(other.m_end)
    , m_size(other.m_size)
    , m_lsize(other.m_lsize)
    , m_lcapacity(other.m_lcapacity)
    , m_lbegin(other.m_lbegin)
    , m_lend(other.m_lend)
    , m_myid(other.m_myid)
  {
    other.m_begin  = iterator{};
    other.m_end    = iterator{};
    other.m_lbegin = nullptr;
    other.m_lend   = nullptr;
    other.m_lsize  = 0;
    other.m_size   = 0;

    other.m_team->unregister_deallocator(&other, std::bind(&Array::deallocate, &other));

    // Register deallocator of this array instance at the team
    // instance that has been used to initialized it:
    m_team->register_deallocator(this, std::bind(&Array::deallocate, this));
  }

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
   * Move assignment is supported for the container with the following
   * limitations:
   *
   * The pattern has to be movable or copyable
   * The underlying memory does not have to be movable (it might).
   */
  self_t & operator=(self_t && other) {
    if (this == &other) {
      return *this;
    }

    m_pattern = std::move(other.m_pattern);

    DASH_ASSERT_EQ(
        m_allocator.resource(),
        &m_globmem,
        "invalid state in move assignment");

    // a) reset own data
    if (m_data) {
      destruct_at_end(m_lbegin);
    }
    m_data.reset();
    // b) swap memory resources
    m_globmem = std::move(other.m_globmem);
    // c) move the unique_gptr from other into this
    unique_gptr_t __tmp{other.m_data.release(),
                        typename unique_gptr_t::deleter_type{
                            m_allocator, m_pattern.local_size()}};
    m_data = std::move(__tmp);


    //TODO rko: we have to provide a proper dash::swap for iterators;
    this->m_begin = iterator{&m_globmem, m_pattern, other.m_begin.pos()};
    this->m_end = iterator{&m_globmem, m_pattern, other.m_end.pos()};

    this->m_lbegin    = other.m_lbegin;
    this->m_lcapacity = other.m_lcapacity;
    this->m_lend      = other.m_lend;
    this->m_lsize     = other.m_lsize;
    this->m_myid      = other.m_myid;
    this->m_size      = other.m_size;
    this->m_team      = other.m_team;

    other.m_begin = iterator{};
    other.m_end   = iterator{};

    other.m_lbegin  = nullptr;
    other.m_lend    = nullptr;
    other.m_lsize   = 0;
    other.m_size    = 0;

    other.m_team->unregister_deallocator(&other, std::bind(&Array::deallocate, &other));

    m_team->register_deallocator(this, std::bind(&Array::deallocate, this));

    return *this;
  }

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
   * View at block at given global block offset.
   */
  constexpr const view_type block(index_type block_gindex) const
  {
    return View(this, ViewSpec<1>(pattern().block(block_gindex)));
  }

  /**
   * The instance of \c GlobStaticMem used by this iterator to resolve addresses
   * in global memory.
   */
  constexpr const memory_type & globmem() const noexcept
  {
    return m_globmem;
  }

  /**
   * Global const pointer to the beginning of the array.
   */
  constexpr const_pointer data() const noexcept
  {
    return const_pointer(m_data.get());
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
  constexpr const_iterator begin() const noexcept
  {
    return (const_cast<iterator &>(m_begin));
  }

  /**
   * Global pointer to the end of the array.
   */
  iterator end() noexcept {
    return m_end;
  }

  /**
   * Global pointer to the end of the array.
   */
  constexpr const_iterator end() const noexcept {
    return (const_cast<iterator &>(m_end));
  }

  /**
   * Native pointer to the first local element in the array.
   */
  constexpr const ElementType * lbegin() const noexcept {
    return m_lbegin;
  }

  /**
   * Native pointer to the first local element in the array.
   */
  ElementType * lbegin() noexcept {
    return m_lbegin;
  }

  /**
   * Native pointer to the end of the array.
   */
  constexpr const ElementType * lend() const noexcept {
    return m_lend;
  }

  /**
   * Native pointer to the end of the array.
   */
  ElementType * lend() noexcept {
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
    return m_begin[global_index];
  }

  /**
   * Subscript operator, not range-checked.
   *
   * \return  A global reference to the element in the array at the given
   *          index.
   */
  constexpr const_reference operator[](
    /// The position of the element to return
    size_type global_index) const
  {
    return m_begin[global_index];
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
  constexpr size_type size() const noexcept
  {
    return m_size;
  }

  /**
   * The number of elements that can be held in currently allocated storage
   * of the array.
   *
   * \return  The number of elements in the array.
   */
  constexpr size_type capacity() const noexcept
  {
    return m_size;
  }

  /**
   * The team containing all units accessing this array.
   *
   * \return  The instance of Team that this array has been instantiated
   *          with
   */
  constexpr Team & team() const noexcept
  {
    return *m_team;
  }

  /**
   * The number of elements in the local part of the array.
   *
   * \return  The number of elements in the array that are local to the
   *          calling unit.
   */
  constexpr size_type lsize() const noexcept
  {
    return m_lsize;
  }

  /**
   * The capacity of the local part of the array.
   *
   * \return  The number of allocated elements in the array that are local
   *          to the calling unit.
   */
  constexpr size_type lcapacity() const noexcept
  {
    return m_lcapacity;
  }

  /**
   * Checks whether the array is empty.
   *
   * \return  True if \c size() is 0, otherwise false
   */
  constexpr bool empty() const noexcept
  {
    return size() == 0;
  }

  constexpr view_type local_in(dash::util::Locality::Scope scope) const
  {
    return view_type(); // TODO
  }

  /**
   * Checks whether the given global index is local to the calling unit.
   *
   * \return  True if the array element referenced by the index is held
   *          in the calling unit's local memory
   */
  constexpr bool is_local(
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
    flush();
    if (m_team && *m_team != dash::Team::Null()) {
      m_team->barrier();
    }
    DASH_LOG_TRACE("Array.barrier >", "passed barrier");
  }

  /**
   * Complete all outstanding non-blocking operations to all units
   * on the array's underlying global memory.
   */
  inline void flush() const {
    //if (m_globmem && m_begin != m_end) {
    if (m_data) {
      //If we have some allocated memory -> flush it
      m_globmem.flush(m_data.get());
    }
  }

  /**
   * Complete all outstanding non-blocking operations to the specified unit
   * on the array's underlying global memory.
   */
  inline void flush(dash::team_unit_t target) const {
    if (m_data) {
      m_globmem.flush(m_data.get(), target);
    }
  }

  /**
   * Locally complete all outstanding non-blocking operations to all units on
   * the array's underlying global memory.
   */
  inline void flush_local() const {
    if (m_data) {
      m_globmem.flush_local(m_data.get());
    }
  }


  /**
   * Locally complete all outstanding non-blocking operations to the
   * specified unit on the array's underlying global memory.
   */
  inline void flush_local(dash::team_unit_t target) const {
    if (m_data) {
      m_globmem.flush_local(m_data.get(), target);
    }
  }

  /**
   * The pattern used to distribute array elements to units.
   */
  constexpr const PatternType & pattern() const noexcept
  {
    return m_pattern;
  }

  /**
   * Delayed allocation of global memory using a
   * one-dimensional distribution spec.
   */
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


  /**
   * Delayed allocation of global memory using the default blocked
   * distribution spec.
   */
  bool allocate(
    size_type   nelem,
    Team      & team = dash::Team::All())
  {
    return allocate(nelem, dash::BLOCKED, team);
  }

  /**
   * Delayed allocation of global memory using a
   * one-dimensional distribution spec and
   * initializing values.
   */
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
    bool ret = allocate(local_elements);
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

    m_team->unregister_deallocator(
      this, std::bind(&Array::deallocate, this));

    // Actual destruction of the array instance:
    DASH_LOG_TRACE_VAR("Array.deallocate()", m_data.get());

    if (m_data) {
      destruct_at_end(m_lbegin);
    }

    m_data.reset();

    m_lsize = 0;
    m_size = 0;
    m_begin = iterator{};
    m_end = iterator{};
    m_lbegin = nullptr;
    m_lend = nullptr;
    DASH_LOG_TRACE_VAR("Array.deallocate >", this);
  }

private:
  void destruct_at_end(local_pointer new_last)
  {
    if (0 == m_lsize || m_lend == nullptr) return;

    local_pointer soon_to_be_new_last = m_lend;

    auto local_alloc = this->globmem().get_allocator();

    while (new_last != soon_to_be_new_last) {
      allocator_traits::destroy(local_alloc, --soon_to_be_new_last);
    }
    m_lend = new_last;
  }

  void do_allocate() {
    //reset old data
    m_data.reset();

    m_team      = &(m_pattern.team());
    m_globmem   = memory_type{*m_team};
    m_allocator = allocator_type{&m_globmem};

    // Check requested capacity:
    m_size = m_pattern.capacity();
    m_team = &m_pattern.team();
    if (m_size == 0) {
      DASH_LOG_WARN("Array.allocate", "allocating dash::Array with size 0");
    }
    // Initialize members:
    m_lsize     = m_pattern.local_size();
    m_lcapacity = m_pattern.local_capacity();
    m_myid      = m_pattern.team().myid();
    // Allocate local memory of identical size on every unit:
    DASH_LOG_TRACE_VAR("Array._allocate", m_myid);
    DASH_LOG_TRACE_VAR("Array._allocate", m_lcapacity);
    DASH_LOG_TRACE_VAR("Array._allocate", m_lsize);

    m_data = std::move(dash::allocate_unique<value_type>(
        m_allocator, m_pattern.local_size()));

    if (m_pattern.local_size()) {
      DASH_ASSERT(m_data);
    }

    // Global iterators:
    m_begin = iterator(&m_globmem, m_pattern);
    m_end   = iterator(m_begin) + m_size;
  }

  bool allocate(std::initializer_list<value_type> local_elements)
  {
    DASH_LOG_TRACE("< Array.allocate", "finished");

    DASH_ASSERT_EQ(
        m_pattern.local_size(), local_elements.size(), "invalid arguments");

    do_allocate();

    // Local iterators:
    auto soon_to_be_lbegin = m_data.get();
    soon_to_be_lbegin.set_unit(m_myid);
    m_lbegin = soon_to_be_lbegin.local();

    DASH_ASSERT(m_lbegin);

    //construct all elements and properly set m_lend
    std::uninitialized_copy(std::begin(local_elements), std::end(local_elements), m_lbegin);
    m_lend = std::next(m_lbegin, local_elements.size());

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
    DASH_LOG_TRACE("Array._allocate", "waiting for allocation of all units");
    m_team->barrier();

    DASH_LOG_TRACE("Array._allocate >", "finished");
    return true;
  }

public:
  /**
   * Delayed allocation of global memory using the specified pattern.
   */
  bool allocate(const PatternType &pattern)
  {
    DASH_LOG_TRACE("< Array.allocate", "finished");
    if (&m_pattern != &pattern) {
      DASH_LOG_TRACE("Array.allocate()", "using specified pattern");
      m_pattern = pattern;
    }

    do_allocate();

    // Local iterators:
    auto soon_to_be_lbegin = m_data.get();
    soon_to_be_lbegin.set_unit(m_myid);
    m_lbegin = soon_to_be_lbegin.local();
    // More efficient than using m_globmem->lend as this a second mapping
    // of the local memory segment:
    m_lend = std::next(m_lbegin, m_lsize);

    // We do no construct the elements to prevent NUMA effects
    // construct_at_end(m_lsize);

    // Register deallocator of this array instance at the team
    // instance that has been used to initialized it:
    m_team->register_deallocator(this, std::bind(&Array::deallocate, this));
    // Assure all units are synchronized after allocation, otherwise
    // other units might start working on the array before allocation
    // completed at all units:
    DASH_LOG_TRACE("Array.allocate", "waiting for allocation of all units");
    m_team->barrier();
    DASH_LOG_TRACE("Array.allocate >", "finished");
    return true;
  }
};

} // namespace dash

#endif /* ARRAY_H_INCLUDED */
