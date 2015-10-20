#ifndef ARRAY_H_INCLUDED
#define ARRAY_H_INCLUDED

#include <dash/Types.h>
#include <dash/GlobMem.h>
#include <dash/GlobIter.h>
#include <dash/GlobRef.h>
#include <dash/GlobAsyncRef.h>
#include <dash/Team.h>
#include <dash/Pattern.h>
#include <dash/HView.h>
#include <dash/Shared.h>
#include <dash/Exception.h>
#include <dash/Cartesian.h>
#include <dash/Dimensional.h>

#include <iterator>

/**
 * \defgroup  DashContainerConcept  Container Concept
 * Concept for iterable distributed containers
 *
 * \ingroup DashConcept
 * \{
 * \par Description
 *
 * \par Methods
 * <table>
 *   <tr>
 *     <th>Method Signature</th>
 *     <th>Semantics</th>
 *   </tr>
 *   <tr>
 *     <td>
 *       \c [](gindex)
 *     </td>
 *     <td>
 *       Returns the element located at the given global position
 *       in the container.
 *     </td>
 *   </tr>
 * </table>
 * \}
 */

/**
 * \defgroup  DashArrayConcept  Array Concept
 * A distributed array
 *
 * \ingroup DashConcept
 * \{
 * \par Description
 *
 * \par Methods
 * <table>
 *   <tr>
 *     <th>Method Signature</th>
 *     <th>Semantics</th>
 *   <tr>
 *     <td>
 *       \c [](gindex)
 *     </td>
 *     <td>
 *       Returns the element located at the given global position
 *       in the array.
 *     </td>
 *   </tr>
 * </table>
 * \}
 */

namespace dash {

/* 
   STANDARD TYPE DEFINITION CONVENTIONS FOR STL CONTAINERS 
   
            value_type  Type of element
        allocator_type  Type of memory manager
             size_type  Unsigned type of container 
                        subscripts, element counts, etc.
       difference_type  Signed type of difference between
                        iterators

              iterator  Behaves like value_type*
        const_iterator  Behaves like const value_type*
      reverse_iterator  Behaves like value_type*
const_reverse_iterator  Behaves like const value_type*

             reference  value_type&
       const_reference  const value_type&

               pointer  Behaves like value_type*         
         const_pointer  Behaves like const value_type*
*/

// forward declaration
template<
  typename ElementType,
  typename IndexType,
  class    PatternType >
class Array;

template<
  typename T,
  typename IndexType,
  class    PatternType >
class LocalArrayRef
{
private:
  typedef LocalArrayRef<T, IndexType, PatternType>
    self_t;

public:
  template <typename T_, typename I_, typename P_>
    friend class LocalArrayRef;

public: 
  typedef T                                                  value_type;

  typedef typename std::make_unsigned<IndexType>::type        size_type;
  typedef IndexType                                          index_type;

  typedef IndexType                                     difference_type;

  typedef T &                                                 reference;
  typedef const reference                               const_reference;

  typedef T *                                                   pointer;
  typedef const pointer                                   const_pointer;

private:
  Array<T, IndexType, PatternType> * const _array;
  
public:
  /**
   * Constructor, creates a local access proxy for the given array.
   */
  LocalArrayRef(
    Array<T, IndexType, PatternType> * const array)
  : _array(array) {
  }

  /**
   * Pointer to initial local element in the array.
   */
  constexpr const_pointer begin() const noexcept {
    return _array->m_lbegin;
  }

  /**
   * Pointer past final local element in the array.
   */
  constexpr const_pointer end() const noexcept {
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
  constexpr value_type operator[](const size_t n) const  {
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
};

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
  typedef const reference                               const_reference;

  typedef T *                                                   pointer;
  typedef const pointer                                   const_pointer;

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
  constexpr const_pointer begin() const noexcept {
    return _array->m_begin;
  }

  /**
   * Pointer past final local element in the array.
   *
   * TODO: Should return GlobAsyncPtr<...>(_array->end())
   */
  constexpr const_pointer end() const noexcept {
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
  constexpr const_async_reference operator[](const size_t n) const  {
    return async_reference(
             _array->m_globmem,
             (*(_array->begin() + n)).gptr());
  }
  
  /**
   * Subscript operator, access to local array element at given position.
   */
  async_reference operator[](const size_t n) {
    return async_reference(
             _array->m_globmem,
             (*(_array->begin() + n)).gptr());
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
  typedef ElementType                                              value_type;
  typedef IndexType                                                index_type;
  typedef typename std::make_unsigned<IndexType>::type              size_type;
  typedef typename std::make_unsigned<IndexType>::type        difference_type;

  typedef       GlobIter<value_type, PatternType>                    iterator;
  typedef const GlobIter<value_type, PatternType>              const_iterator;
  typedef       std::reverse_iterator<      iterator>        reverse_iterator;
  typedef       std::reverse_iterator<const_iterator>  const_reverse_iterator;
  
  typedef       GlobRef<value_type>                                 reference;
  typedef const GlobRef<value_type>                           const_reference;
  
  typedef       GlobIter<value_type, PatternType>                     pointer;
  typedef const GlobIter<value_type, PatternType>               const_pointer;

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
  inline    Team            & team();

  constexpr size_type         size()                const noexcept;
  constexpr size_type         local_size()          const noexcept;
  constexpr size_type         local_capacity()      const noexcept;
  constexpr size_type         extent(dim_t dim)     const noexcept;
  constexpr Extents_t         extents()             const noexcept;
  constexpr bool              empty()               const noexcept;

  inline    void              barrier()             const;

  inline    const_pointer     data()                const noexcept;
  inline    iterator          begin()                     noexcept;
  inline    const_iterator    begin()               const noexcept;
  inline    iterator          end()                       noexcept;
  inline    const_iterator    end()                 const noexcept;
  /// View representing elements in the active unit's local memory.
  inline    local_type        sub_local()                 noexcept;
  /// Pointer to first element in local range.
  inline    ElementType     * lbegin()                    noexcept;
  /// Pointer past final element in local range.
  inline    ElementType     * lend()                      noexcept;

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

private:
  /// Pointer to array instance referenced by this view.
  Array_t    * _arr;
  /// The view's offset and extent within the referenced array.
  ViewSpec_t   _viewspec;
};

/**
 * A distributed array.
 *
 * \related dash::min_element
 * \related dash::max_element
 *
 * \concept{DashContainerConcept}
 * \concept{DashArrayConcept}
 *
 * Note: Template parameter IndexType could be deduced from pattern type:
 *       PatternT::index_type
 */
template<
  typename ElementType,
  typename IndexType   = dash::default_index_t,
  class PatternType    = Pattern<1, ROW_MAJOR, IndexType> >
class Array
{
private:
  typedef Array<ElementType, IndexType, PatternType> self_t;

/// Public types as required by iterator concept
public: 
  typedef ElementType                                              value_type;
  typedef IndexType                                                index_type;
  typedef typename std::make_unsigned<IndexType>::type              size_type;
  typedef typename std::make_unsigned<IndexType>::type        difference_type;

  typedef       GlobIter<value_type, PatternType>                    iterator;
  typedef const GlobIter<value_type, PatternType>              const_iterator;
  typedef       std::reverse_iterator<      iterator>        reverse_iterator;
  typedef       std::reverse_iterator<const_iterator>  const_reverse_iterator;
  
  typedef       GlobRef<value_type>                                 reference;
  typedef const GlobRef<value_type>                           const_reference;
  
  typedef       GlobIter<value_type, PatternType>                     pointer;
  typedef const GlobIter<value_type, PatternType>               const_pointer;

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
  : m_team(&team),
    m_pattern(
      SizeSpec_t(0),
      DistributionSpec_t(dash::BLOCKED),
      team),
    m_size(0),
    m_lsize(0),
    m_lcapacity(0),
    local(this),
    async(this) {
    DASH_LOG_TRACE("Array()", "default constructor");
  }

  /**
   * Constructor, specifies distribution type explicitly.
   */
  Array(
    size_type nelem,
    const DistributionSpec_t & distribution,
    Team & team = dash::Team::All())
  : m_team(&team),
    m_pattern(
      SizeSpec_t(nelem),
      distribution,
      team),
    m_size(0),
    m_lsize(0),
    m_lcapacity(0),
    local(this),
    async(this) {
    DASH_LOG_TRACE("Array()", nelem);
    allocate(m_pattern);
  }

  /**
   * Constructor, specifies distribution pattern explicitly.
   */
  Array(
    const PatternType & pattern)
  : m_team(&pattern.team()),
    m_pattern(pattern),
    m_size(0),
    m_lsize(0),
    m_lcapacity(0),
    local(this),
    async(this) {
    DASH_LOG_TRACE("Array()", "pattern instance constructor");
    allocate(m_pattern);
  }
  
  /**
   * Delegating constructor, specifies the size of the array.
   */
  Array(
    size_type nelem,
    Team & team = dash::Team::All())
  : Array(nelem, dash::BLOCKED, team) {
    DASH_LOG_TRACE("Array()", "finished delegating constructor");
  }

  /**
   * Destructor, deallocates array elements.
   */
  ~Array() {
    DASH_LOG_TRACE_VAR("Array.~Array()", this);
    deallocate();
  }

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
  const_pointer data() const noexcept {
    return m_begin;
  }
  
  /**
   * Global pointer to the beginning of the array.
   */
  iterator begin() noexcept {
    iterator res = iterator(m_begin);
    return res;
  }
  
  /**
   * Global pointer to the beginning of the array.
   */
  const_iterator begin() const noexcept {
    return m_begin;
  }

  /**
   * Global pointer to the end of the array.
   */
  iterator end() noexcept {
    iterator res = iterator(m_end);
    return res;
  }

  /**
   * Global pointer to the end of the array.
   */
  const_iterator end() const noexcept {
    return m_end;
  }

  /**
   * Native pointer to the first local element in the array.
   */
  ElementType * const lbegin() const noexcept {
    return m_lbegin;
  }

  /**
   * Native pointer to the end of the array.
   */
  ElementType * const lend() const noexcept {
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
    size_type global_index) {
    DASH_LOG_TRACE("Array.[]=", global_index);
    return m_begin[global_index];
  }

  /**
   * Subscript operator, not range-checked.
   *
   * \return  A global reference to the element in the array at the given
   *          index.
   */
  const_reference operator[](
    /// The position of the element to return
    size_type global_index) const {
    DASH_LOG_TRACE("Array.[]", global_index);
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
    size_type global_pos) {
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
    size_type global_pos) const {
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
  constexpr size_type size() const noexcept {
    return m_size;
  }

  /**
   * The number of elements that can be held in currently allocated storage
   * of the array.
   *
   * \return  The number of elements in the array.
   */
  constexpr size_type capacity() const noexcept {
    return m_size;
  }

  /**
   * The team containing all units accessing this array.
   *
   * \return  The instance of Team that this array has been instantiated
   *          with
   */
  constexpr const Team & team() const noexcept {
    return *m_team;
  }

  /**
   * The number of elements in the local part of the array.
   *
   * \return  The number of elements in the array that are local to the
   *          calling unit.
   */
  constexpr size_type lsize() const noexcept {
    return m_lsize;
  }

  /**
   * The capacity of the local part of the array.
   *
   * \return  The number of allocated elements in the array that are local
   *          to the calling unit.
   */
  constexpr size_type lcapacity() const noexcept {
    return m_lcapacity;
  }
  
  /**
   * Checks whether the array is empty.
   *
   * \return  True if \c size() is 0, otherwise false
   */
  constexpr bool empty() const noexcept {
    return size() == 0;
  }

  /**
   * Checks whether the given global index is local to the calling unit.
   *
   * \return  True if the array element referenced by the index is held
   *          in the calling unit's local memory
   */
  bool is_local(
    /// A global array index
    index_type global_index) const {
    return m_pattern.is_local(global_index, m_myid);
  }
  
  /**
   * Establish a barrier for all units operating on the array, publishing all
   * changes to all units.
   */
  void barrier() const {
    m_globmem->flush();
    m_team->barrier();
  }

  /**
   * The pattern used to distribute array elements to units.
   */
  const PatternType & pattern() const {
    return m_pattern;
  }

  template<int level>
  dash::HView<self_t, level> hview() {
    return dash::HView<self_t, level>(*this);
  }

  bool allocate(
    size_type nelem,
    dash::DistributionSpec<1> distribution,
    dash::Team & team = dash::Team::All()) {
    DASH_LOG_TRACE("Array.allocate()", nelem);
    DASH_LOG_TRACE_VAR("Array.allocate", m_team->dart_id());
    DASH_LOG_TRACE_VAR("Array.allocate", team.dart_id());
    // Check requested capacity:
    if (nelem == 0) {
      DASH_THROW(
        dash::exception::InvalidArgument,
        "Tried to allocate dash::Array with size 0");
    }
    if (*m_team == dash::Team::Null()) {
      DASH_LOG_TRACE("Array.allocate",
                     "initializing pattern with Team::All()");
      m_pattern = PatternType(nelem, distribution, team);
      DASH_LOG_TRACE_VAR("Array.allocate", team.dart_id());
      DASH_LOG_TRACE_VAR("Array.allocate", m_pattern.team().dart_id());
    } else {
      DASH_LOG_TRACE("Array.allocate",
                     "initializing pattern with initial team");
      m_pattern = PatternType(nelem, distribution, *m_team);
    }
    return allocate(m_pattern);
  }

  void deallocate() {
    DASH_LOG_TRACE_VAR("Array.deallocate()", this);
    DASH_LOG_TRACE_VAR("Array.deallocate()", m_size);
    // Remove this function from team deallocator list to avoid
    // double-free:
    m_pattern.team().unregister_deallocator(
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

private:
  bool allocate(
    const PatternType & pattern) {
    DASH_LOG_TRACE("Array._allocate()", "pattern", 
                   pattern.memory_layout().extents());
    // Check requested capacity:
    m_size      = pattern.capacity();
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
    m_globmem   = new GlobMem_t(pattern.team(), m_lcapacity);
    // Global iterators:
    m_begin     = iterator(m_globmem, pattern);
    m_end       = iterator(m_begin) + m_size;
    // Local iterators:
    m_lbegin    = m_globmem->lbegin(m_myid);
    m_lend      = m_globmem->lend(m_myid);
    DASH_LOG_TRACE_VAR("Array._allocate", m_myid);
    DASH_LOG_TRACE_VAR("Array._allocate", m_size);
    DASH_LOG_TRACE_VAR("Array._allocate", m_lsize);
    // Register deallocator of this array instance at the team
    // instance that has been used to initialized it:
    pattern.team().register_deallocator(
      this, std::bind(&Array::deallocate, this));
    DASH_LOG_TRACE("Array._allocate() finished");
    return true;
  }

private:
  typedef dash::GlobMem<value_type> GlobMem_t;
  /// Team containing all units interacting with the array
  dash::Team         * m_team      = nullptr;
  /// DART id of the unit that created the array
  dart_unit_t          m_myid;
  /// Element distribution pattern
  PatternType          m_pattern;
  /// Global memory allocation and -access
  GlobMem_t          * m_globmem; 
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
  ElementType        * m_lbegin;
  /// Native pointer past last local element in the array
  ElementType        * m_lend;

};

} // namespace dash

#endif /* ARRAY_H_INCLUDED */
