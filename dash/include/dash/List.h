#ifndef DASH__LIST_H__
#define DASH__LIST_H__

#include <iterator>
#include <limits>

#include <dash/Types.h>
#include <dash/GlobIter.h>
#include <dash/GlobRef.h>
#include <dash/Team.h>
#include <dash/Shared.h>
#include <dash/Exception.h>
#include <dash/Cartesian.h>
#include <dash/Dimensional.h>
#include <dash/DynamicPattern.h>
#include <dash/GlobDynamicMem.h>
#include <dash/Allocator.h>

namespace dash {

/**
 * \defgroup  DashListConcept  List Concept
 * Concept of a distributed one-dimensional list container.
 *
 * \ingroup DashConcept
 * \{
 * \par Description
 *
 * \par Member types
 *
 * Type                            | Definition
 * ------------------------------- | ----------------------------------------------------------------------------------------
 * <b>STL</b>                      | &nbsp;
 * <tt>value_type</tt>             | First template parameter <tt>ElementType</tt>
 * <tt>allocator_type</tt>         | Second template parameter <tt>AllocatorType</tt>
 * <tt>reference</tt>              | <tt>value_type &</tt>
 * <tt>const_reference</tt>        | <tt>const value_type &</tt>
 * <tt>pointer</tt>                | <tt>allocator_traits<allocator_type>::pointer</tt>
 * <tt>const_pointer</tt>          | <tt>allocator_traits<allocator_type>::const_pointer</tt>
 * <tt>iterator</tt>               | A bidirectional iterator to <tt>value_type</tt>
 * <tt>const_iterator</tt>         | A bidirectional iterator to <tt>const value_type</tt>
 * <tt>reverse_iterator</tt>       | <tt>reverse_iterator<iterator></tt>
 * <tt>const_reverse_iterator</tt> | <tt>reverse_iterator<const_iterator></tt>
 * <tt>difference_type</tt>        | A signed integral type, identical to <tt>iterator_traits<iterator>::difference_type</tt>
 * <tt>size_type</tt>              | Unsigned integral type to represent any non-negative value of <tt>difference_type</tt>
 * <b>DASH-specific</b>            | &nbsp;
 * <tt>index_type</tt>             | A signed integgral type to represent positions in global index space
 * <tt>view_type</tt>              | Proxy type for views on list elements, implements \c DashListConcept
 * <tt>local_type</tt>             | Proxy type for views on list elements that are local to the calling unit
 * <tt>pattern_type</tt>           | Type implementing the \c DashPatternConcept used to distribute list elements to units
 *
 * \par Member functions
 *
 * Function                     | Return type         | Definition
 * ---------------------------- | ------------------- | -----------------------------------------------
 * <b>Initialization</b>        | &nbsp;              | &nbsp;
 * <tt>operator=</tt>           | <tt>self &</tt>     | Assignment operator
 * <b>Iterators</b>             | &nbsp;              | &nbsp;
 * <tt>begin</tt>               | <tt>iterator</tt>   | Iterator to first element in the list
 * <tt>end</tt>                 | <tt>iterator</tt>   | Iterator past last element in the list
 * <b>Capacity</b>              | &nbsp;              | &nbsp;
 * <tt>size</tt>                | <tt>size_type</tt>  | Number of elements in the list
 * <tt>max_size</tt>            | <tt>size_type</tt>  | Maximum number of elements the list can hold
 * <tt>empty</tt>               | <tt>bool</tt>       | Whether the list is empty, i.e. size is 0
 * <b>Element access</b>        | &nbsp;              | &nbsp;
 * <tt>front</tt>               | <tt>reference</tt>  | Access the first element in the list
 * <tt>back</tt>                | <tt>reference</tt>  | Access the last element in the list
 * <b>Modifiers</b>             | &nbsp;              | &nbsp;
 * <tt>push_front</tt>          | <tt>void</tt>       | Insert element at beginning
 * <tt>pop_front</tt>           | <tt>void</tt>       | Delete first element
 * <tt>push_back</tt>           | <tt>void</tt>       | Insert element at the end
 * <tt>pop_back</tt>            | <tt>void</tt>       | Delete last element
 * <tt>emplace</tt>             | <tt>iterator</tt>   | Construct and insert element at given position
 * <tt>emplace_front</tt>       | <tt>void</tt>       | Construct and insert element at beginning
 * <tt>emplace_back</tt>        | <tt>void</tt>       | Construct and insert element at the end
 * <tt>insert</tt>              | <tt>iterator</tt>   | Insert elements before given position
 * <tt>erase</tt>               | <tt>iterator</tt>   | Erase elements at position or in range
 * <tt>swap</tt>                | <tt>void</tt>       | Swap content
 * <tt>resize</tt>              | <tt>void</tt>       | Cbange the list's size
 * <tt>clear</tt>               | <tt>void</tt>       | Clear the list's content
 * <b>Operations</b>            | &nbsp;              | &nbsp;
 * <tt>splice</tt>              | <tt>void</tt>       | Transfer elements from one list to another
 * <tt>remove<tt>               | <tt>void</tt>       | Remove elements with a given value
 * <tt>remove_if<tt>            | <tt>void</tt>       | Remove elements fulfilling a given condition
 * <tt>unique</tt>              | <tt>void</tt>       | Remove duplicate elements
 * <tt>sort</tt>                | <tt>void</tt>       | Sort list elements
 * <tt>merge</tt>               | <tt>void</tt>       | Merge sorted lists
 * <tt>reverse</tt>             | <tt>void</tt>       | Reverse the order of list elements
 * <b>Views (DASH specific)</b> | &nbsp;              | &nbsp;
 * <tt>local</tt>               | <tt>local_type</tt> | View on list elements local to calling unit
 * <tt>block</tt>               | <tt>view_type</tt>  | View on elements in block at global block index
 * \}
 *
 * Usage examples:
 *
 * \code
 * size_t initial_local_capacity = 100;
 * size_t initial_capacity       = dash::size() * initial_local_capacity;
 * dash::List<int> list(initial_capacity);
 *
 * assert(list.size() == 0);
 * assert(list.capacity() == initial_capacity);
 *
 * list.local.push_back(dash::myid() + 2 + dash::myid() * 3);
 * list.local.push_back(dash::myid() + 3 + dash::myid() * 3);
 * list.local.push_back(dash::myid() + 4 + dash::myid() * 3);
 *
 * // Logical structure of list for 3 units:
 * //
 * //     | unit 0     | unit 1     | unit 2    |
 * // ----|------------|------------|-----------|---
 * // Nil ---> 2 --.  .---> 5 --.  .--->  8 --.
 * //      .-- 3 <-' |  .-- 6 <-' |  .--  9 <-'
 * //      `-> 4 ----'  `-> 7 ----'  `-> 10 ---> Nil
 *
 * assert(list.local.size()  == 1);
 * assert(list.local.front() == dash::myid() + 1);
 * assert(list.local.back()  == dash::myid() + 3);
 *
 * list.barrier();
 * assert(list.size() == dash::size() * 3);
 *
 * if (dash::myid() == 0) {
 *   list.push_front(0);
 *   list.push_front(1);
 *   list.push_back(11);
 *   list.push_back(12);
 *   list.push_back(13);
 *   list.push_back(14);
 * }
 *
 * // Logical structure of list for 3 units:
 * //
 * //     | unit 0     | unit 1     | unit 2    |
 * // ----|------------|------------|-----------|---
 * // Nil ---> 0 --.  .---> 5 --.  .--->  8 --.
 * //      .-- 1 <-' |  .-- 6 <-' |  .--  9 <-'
 * //      `-> 2 --. |  `-> 7 ----'  `-> 10 --.
 * //      .-- 3 <-' |               .-- 11 <-'
 * //      `-> 4 ----'               `-> 12 --.
 * //                                .-- 13 <-'
 * //                                `-> 14 ---> Nil
 *
 * list.balance();
 *
 * // Logical structure of list for 3 units:
 * //
 * //     | unit 0     | unit 1     | unit 2    |
 * // ----|------------|------------|-----------|---
 * // Nil ---> 0 --.  .---> 5 --.  .---> 10 --.
 * //      .-- 1 <-' |  .-- 6 <-' |  .-- 11 <-'
 * //      `-> 2 --. |  `-> 7 --. |  `-> 12 --.
 * //      .-- 3 <-' |  .-- 8 <-' |  .-- 13 --'
 * //      `-> 4 ----'  `-> 9 ----'  `-> 14 ---> Nil
 *
 * \endcode
 */

// forward declaration
template<
  typename ElementType,
  class    AllocatorType,
  class    PatternType >
class List;

/**
 * Proxy type representing a local view on a referenced \c dash::List.
 *
 * \concept{DashContainerConcept}
 * \concept{DashListConcept}
 */
template<
  typename T,
  class    AllocatorType,
  class    PatternType >
class LocalListRef
{
  template <typename T_, typename I_, typename P_>
    friend class LocalListRef;

private:
  static const dim_t NumDimensions = 1;

/// Type definitions required for dash::List concept:
public:
  typedef PatternType                                           pattern_type;
  typedef typename PatternType::index_type                        index_type;
  typedef LocalListRef<T, index_type, PatternType>                 view_type;
  typedef AllocatorType                                       allocator_type;
  typedef dash::GlobDynamicMem<T, AllocatorType>               glob_mem_type;

/// Type definitions required for std::list concept:
public:
  typedef T                                                       value_type;
  typedef typename std::make_unsigned<index_type>::type            size_type;
  typedef typename std::make_unsigned<index_type>::type      difference_type;

  typedef typename glob_mem_type::local_pointer                      pointer;
  typedef typename glob_mem_type::const_local_pointer          const_pointer;

  typedef typename glob_mem_type::local_reference                  reference;
  typedef typename glob_mem_type::const_local_reference      const_reference;

  typedef typename glob_mem_type::local_iterator                    iterator;
  typedef typename glob_mem_type::const_local_iterator        const_iterator;

  typedef std::reverse_iterator<      iterator>             reverse_iterator;
  typedef std::reverse_iterator<const_iterator>       const_reverse_iterator;

private:
  typedef LocalListRef<T, AllocatorType, PatternType>
    self_t;
  typedef List<T, AllocatorType, PatternType>
    List_t;
  typedef ViewSpec<NumDimensions, index_type>
    ViewSpec_t;
  typedef std::array<typename PatternType::size_type, NumDimensions>
    Extents_t;

public:
  /**
   * Constructor, creates a local access proxy for the given list.
   */
  LocalListRef(
    List_t * list)
  : _list(list)
  { }

  LocalListRef(
    /// Pointer to list instance referenced by this view.
    List<T, index_type, PatternType> * list,
    /// The view's offset and extent within the referenced list.
    const ViewSpec_t                 & viewspec)
  : _list(list),
    _viewspec(viewspec)
  { }

  /**
   * Pointer to initial local element in the list.
   */
  inline const_iterator begin() const noexcept
  {
    return _list->_lbegin;
  }

  /**
   * Pointer to initial local element in the list.
   */
  inline iterator begin() noexcept
  {
    return _list->_lbegin;
  }

  /**
   * Pointer past final local element in the list.
   */
  inline const_iterator end() const noexcept
  {
    return _list->_lend;
  }

  /**
   * Pointer past final local element in the list.
   */
  inline iterator end() noexcept
  {
    return _list->_lend;
  }

  /**
   * Inserts a new element at the end of the list, after its current
   * last element. The content of \c value is copied or moved to the
   * inserted element.
   * Increases the container size by one.
   */
  inline void push_back(const value_type & value)
  {
    // Increase local size in pattern:
    _list->pattern().local_resize(_list->_lsize + 1);
    // Acquire memory for new element:
    if (_list->_lcapacity > _list->_lsize) {
      // No reallocation required.
      _list->_lbegin[_list->_lsize] = value;
      _list->_lsize++;
      _list->_size++;
    } else{
      // Local capacity must be increased, reallocate:
      DASH_THROW(dash::exception::NotImplemented,
                 "dash::ListLocalRef.push_back: "
                 "reallocation is not implemented");
    }
  }

  /**
   * Removes and destroys the last element in the list, reducing the
   * container size by one.
   */
  void pop_back()
  {
    DASH_THROW(dash::exception::NotImplemented,
               "dash::LocalListRef.pop_back is not implemented");
  }

  /**
   * Accesses the last element in the list.
   */
  reference back()
  {
    DASH_THROW(dash::exception::NotImplemented,
               "dash::LocalListRef._back is not implemented");
  }

  /**
   * Inserts a new element at the beginning of the list, before its current
   * first element. The content of \c value is copied or moved to the
   * inserted element.
   * Increases the container size by one.
   */
  inline void push_front(const value_type & value)
  {
    if (_list->_lcapacity > _list->_lsize) {
      // no reallocation required
    } else{
      // local capacity must be increased, reallocate
    }
    DASH_THROW(dash::exception::NotImplemented,
               "dash::ListLocalRef.push_front is not implemented");
  }

  /**
   * Removes and destroys the first element in the list, reducing the
   * container size by one.
   */
  void pop_front()
  {
    DASH_THROW(dash::exception::NotImplemented,
               "dash::LocalListRef.pop_front is not implemented");
  }

  /**
   * Accesses the first element in the list.
   */
  reference front()
  {
    DASH_THROW(dash::exception::NotImplemented,
               "dash::LocalListRef.front is not implemented");
  }

  /**
   * Number of list elements in local memory.
   */
  inline size_type size() const noexcept
  {
    return end() - begin();
  }

  /**
   * Checks whether the given global index is local to the calling unit.
   *
   * \return  True
   */
  constexpr bool is_local(
    /// A global list index
    index_type global_index) const
  {
    return true;
  }

  /**
   * View at block at given global block offset.
   */
  self_t block(index_type block_lindex)
  {
    DASH_LOG_TRACE("LocalListRef.block()", block_lindex);
    ViewSpec<1> block_view = pattern().local_block(block_lindex);
    DASH_LOG_TRACE("LocalListRef.block >", block_view);
    return self_t(_list, block_view);
  }

  /**
   * The pattern used to distribute list elements to units.
   */
  inline const pattern_type & pattern() const
  {
    return _list->pattern();
  }

  /**
   * The pattern used to distribute list elements to units.
   */
  inline pattern_type & pattern()
  {
    return _list->pattern();
  }

private:
  /// Pointer to list instance referenced by this view.
  List_t * const _list;
  /// The view's offset and extent within the referenced list.
  ViewSpec_t     _viewspec;
};

/**
 * Proxy type referencing a \c dash::List.
 *
 * \concept{DashContainerConcept}
 * \concept{DashListConcept}
 */
template<
  typename ElementType,
  class    AllocatorType,
  class    PatternType>
class ListRef
{
private:
  static const dim_t NumDimensions = 1;

/// Type definitions required for DASH list concept:
public:
  typedef typename PatternType::index_type                        index_type;
  typedef ElementType                                             value_type;
  typedef PatternType                                           pattern_type;
  typedef ListRef<ElementType, AllocatorType, PatternType>         view_type;
  typedef LocalListRef<value_type, AllocatorType, PatternType>    local_type;
  typedef AllocatorType                                       allocator_type;

/// Public types as required by STL list concept:
public:
  typedef typename std::make_unsigned<index_type>::type            size_type;
  typedef typename std::make_unsigned<index_type>::type      difference_type;

  typedef       GlobIter<value_type, PatternType>                   iterator;
  typedef const GlobIter<value_type, PatternType>             const_iterator;
  typedef       std::reverse_iterator<      iterator>       reverse_iterator;
  typedef       std::reverse_iterator<const_iterator> const_reverse_iterator;

  typedef       GlobRef<value_type>                                reference;
  typedef const GlobRef<value_type>                          const_reference;

  typedef       GlobIter<value_type, PatternType>                    pointer;
  typedef const GlobIter<value_type, PatternType>              const_pointer;

private:
  typedef ListRef<ElementType, AllocatorType, PatternType>
    self_t;
  typedef List<ElementType, AllocatorType, PatternType>
    List_t;
  typedef ViewSpec<NumDimensions, index_type>
    ViewSpec_t;
  typedef std::array<typename PatternType::size_type, NumDimensions>
    Extents_t;

public:
  ListRef(
    /// Pointer to list instance referenced by this view.
    List_t         * list,
    /// The view's offset and extent within the referenced list.
    const ViewSpec_t & viewspec)
  : _list(list),
    _viewspec(viewspec)
  { }

public:
  /**
   * Inserts a new element at the end of the list, after its current
   * last element. The content of \c value is copied or moved to the
   * inserted element.
   * Increases the container size by one.
   */
  inline void push_back(const value_type & value)
  {
    if (_list->_lcapacity > _list->_lsize) {
      // no reallocation required
    } else{
      // local capacity must be increased, reallocate
    }
    DASH_THROW(dash::exception::NotImplemented,
               "dash::ListRef.push_back is not implemented");
  }

  /**
   * Removes and destroys the last element in the list, reducing the
   * container size by one.
   */
  void pop_back()
  {
    DASH_THROW(dash::exception::NotImplemented,
               "dash::ListRef.pop_back is not implemented");
  }

  /**
   * Accesses the last element in the list.
   */
  reference back()
  {
    DASH_THROW(dash::exception::NotImplemented,
               "dash::ListRef.back is not implemented");
  }

  /**
   * Inserts a new element at the beginning of the list, before its current
   * first element. The content of \c value is copied or moved to the
   * inserted element.
   * Increases the container size by one.
   */
  inline void push_front(const value_type & value)
  {
    if (_list->_lcapacity > _list->_lsize) {
      // no reallocation required
    } else{
      // local capacity must be increased, reallocate
    }
    DASH_THROW(dash::exception::NotImplemented,
               "dash::ListRef.push_front is not implemented");
  }

  /**
   * Removes and destroys the first element in the list, reducing the
   * container size by one.
   */
  void pop_front()
  {
    DASH_THROW(dash::exception::NotImplemented,
               "dash::ListRef.pop_front is not implemented");
  }

  /**
   * Accesses the first element in the list.
   */
  reference front()
  {
    DASH_THROW(dash::exception::NotImplemented,
               "dash::ListRef.front is not implemented");
  }

  inline Team              & team();

  inline size_type           size()             const noexcept;
  inline size_type           local_size()       const noexcept;
  inline size_type           local_capacity()   const noexcept;
  inline size_type           extent(dim_t dim)  const noexcept;
  inline Extents_t           extents()          const noexcept;
  inline bool                empty()            const noexcept;

  inline void                barrier()          const;

  inline const_pointer       data()             const noexcept;
  inline iterator            begin()                  noexcept;
  inline const_iterator      begin()            const noexcept;
  inline iterator            end()                    noexcept;
  inline const_iterator      end()              const noexcept;
  /// Pointer to first element in local range.
  inline ElementType       * lbegin()           const noexcept;
  /// Pointer past final element in local range.
  inline ElementType       * lend()             const noexcept;

  /**
   * The pattern used to distribute list elements to units.
   */
  inline const PatternType & pattern() const
  {
    return _list->pattern();
  }

  /**
   * The pattern used to distribute list elements to units.
   */
  inline PatternType & pattern()
  {
    return _list->pattern();
  }

private:
  /// Pointer to list instance referenced by this view.
  List_t    * _list;
  /// The view's offset and extent within the referenced list.
  ViewSpec_t  _viewspec;

}; // class ListRef


/**
 * A dynamic bi-directional list with support for workload balancing.
 *
 * \concept{DashContainerConcept}
 * \concept{DashListConcept}
 */
template<
  typename ElementType,
  class    AllocatorType = dash::allocator::DynamicAllocator<ElementType>,
  class    PatternType   = dash::DynamicPattern<
                             1, dash::ROW_MAJOR, dash::default_index_t> >
class List
{
  template<
    typename T_,
    class    A_,
    class    P_>
  friend class LocalListRef;

/// Public types as required by DASH list concept
public:
  /// The type of the pattern used to distribute list elements to units
  typedef ElementType                                             value_type;
  typedef typename PatternType::index_type                        index_type;
  typedef typename std::make_unsigned<index_type>::type            size_type;
  typedef PatternType                                           pattern_type;
  typedef AllocatorType                                       allocator_type;
  typedef LocalListRef<ElementType, AllocatorType, PatternType>   local_type;
  typedef ListRef<ElementType, AllocatorType, PatternType>         view_type;

  typedef typename local_type::iterator
    local_iterator;
  typedef typename local_type::const_iterator
    local_const_iterator;
  typedef typename local_type::reverse_iterator
    local_reverse_iterator;
  typedef typename local_type::const_reverse_iterator
    local_const_reverse_iterator;

private:
  typedef DistributionSpec<1>
    DistributionSpec_t;
  typedef SizeSpec<1, size_type>
    SizeSpec_t;
  typedef ViewSpec<1, index_type>
    ViewSpec_t;
  typedef dash::GlobDynamicMem<value_type, allocator_type>
    GlobMem_t;

/// Public types as required by STL list concept
public:
  typedef typename std::make_unsigned<index_type>::type      difference_type;

  typedef GlobIter<value_type, PatternType, GlobMem_t>              iterator;
  typedef GlobIter<const value_type, PatternType, GlobMem_t>  const_iterator;
  typedef std::reverse_iterator<      iterator>             reverse_iterator;
  typedef std::reverse_iterator<const_iterator>       const_reverse_iterator;

  typedef GlobRef<value_type>                                      reference;
  typedef GlobRef<const value_type>                          const_reference;

  typedef GlobIter<value_type, PatternType>                          pointer;
  typedef GlobIter<const value_type, PatternType>              const_pointer;

private:
  typedef List<ElementType, index_type, PatternType> self_t;

public:
  /// Local proxy object, allows use in range-based for loops.
  local_type local;

public:
  /**
   * Default constructor, for delayed allocation.
   *
   * Sets the associated team to DART_TEAM_NULL for global list instances
   * that are declared before \c dash::Init().
   */
  List(
    Team & team = dash::Team::Null())
  : local(this),
    _team(&team),
    _pattern(
      SizeSpec_t(0),
      DistributionSpec_t(dash::BLOCKED),
      team),
    _size(0),
    _lsize(0),
    _lcapacity(0)
  {
    DASH_LOG_TRACE("List()", "default constructor");
  }

  /**
   * Constructor, specifies distribution type explicitly.
   */
  List(
    size_type                  nelem,
    const DistributionSpec_t & distribution,
    Team                     & team = dash::Team::All())
  : local(this),
    _team(&team),
    _pattern(
      SizeSpec_t(nelem),
      distribution,
      team),
    _size(0),
    _lsize(0),
    _capacity(0),
    _lcapacity(0)
  {
    DASH_LOG_TRACE("List()", nelem);
    allocate(_pattern);
  }

  /**
   * Constructor, specifies distribution pattern explicitly.
   */
  List(
    const PatternType & pattern)
  : local(this),
    _team(&pattern.team()),
    _pattern(pattern),
    _size(0),
    _lsize(0),
    _capacity(0),
    _lcapacity(0)
  {
    DASH_LOG_TRACE("List()", "pattern instance constructor");
    allocate(_pattern);
  }

  /**
   * Delegating constructor, specifies the size of the list.
   */
  List(
    size_type   nelem,
    Team      & team = dash::Team::All())
  : List(nelem, dash::BLOCKED, team)
  {
    DASH_LOG_TRACE("List()", "finished delegating constructor");
  }

  /**
   * Destructor, deallocates list elements.
   */
  ~List()
  {
    DASH_LOG_TRACE_VAR("List.~List()", this);
    deallocate();
  }

  /**
   * Inserts a new element at the end of the list, after its current
   * last element. The content of \c value is copied or moved to the
   * inserted element.
   * Increases the container size by one.
   */
  void push_back(const value_type & element)
  {
  }

  /**
   * Removes and destroys the last element in the list, reducing the
   * container size by one.
   */
  void pop_back()
  {
  }

  /**
   * Accesses the last element in the list.
   */
  reference back()
  {
  }

  /**
   * Inserts a new element at the beginning of the list, before its current
   * first element. The content of \c value is copied or moved to the
   * inserted element.
   * Increases the container size by one.
   */
  void push_front(const value_type & value)
  {
  }

  /**
   * Removes and destroys the first element in the list, reducing the
   * container size by one.
   */
  void pop_front()
  {
  }

  /**
   * Accesses the first element in the list.
   */
  reference front()
  {
  }

  /**
   * View at block at given global block offset.
   */
  view_type block(index_type block_gindex)
  {
    DASH_LOG_TRACE("List.block()", block_gindex);
    ViewSpec_t block_view = pattern().block(block_gindex);
    DASH_LOG_TRACE("List.block >", block_view);
    return view_type(this, block_view);
  }

  /**
   * Global const pointer to the beginning of the list.
   */
  const_pointer data() const noexcept
  {
    return _begin;
  }

  /**
   * Global pointer to the beginning of the list.
   */
  iterator begin() noexcept
  {
    return _begin;
  }

  /**
   * Global pointer to the beginning of the list.
   */
  const_iterator begin() const noexcept
  {
    return _begin;
  }

  /**
   * Global pointer to the end of the list.
   */
  iterator end() noexcept
  {
    return _end;
  }

  /**
   * Global pointer to the end of the list.
   */
  const_iterator end() const noexcept
  {
    return _end;
  }

  /**
   * Native pointer to the first local element in the list.
   */
  local_iterator lbegin() noexcept
  {
    return _lbegin;
  }

  /**
   * Native pointer to the first local element in the list.
   */
  local_const_iterator lbegin() const noexcept
  {
    return _lbegin;
  }

  /**
   * Native pointer to the end of the list.
   */
  local_iterator lend() noexcept
  {
    return _lend;
  }

  /**
   * Native pointer to the end of the list.
   */
  local_const_iterator lend() const noexcept
  {
    return _lend;
  }

  /**
   * The size of the list.
   *
   * \return  The number of elements in the list.
   */
  inline size_type size() const noexcept
  {
    return _size;
  }

  /**
   * Maximum number of elements a list container can hold, e.g. due to
   * system limitations.
   * The maximum size is not guaranteed.
   */
  inline size_type max_size() const noexcept
  {
    return std::numeric_limits<int>::max();
  }

  /**
   * Resizes the list so its capacity is changed to the given number of
   * elements. Elements are removed and destroying elements from the back,
   * if necessary.
   */
  void resize(size_t num_elements)
  {
  }

  /**
   * The number of elements that can be held in currently allocated storage
   * of the list.
   *
   * \return  The number of elements in the list.
   */
  inline size_type capacity() const noexcept
  {
    return _capacity;
  }

  inline iterator erase(const_iterator position)
  {
    return _begin;
  }

  inline iterator erase(const_iterator first, const_iterator last)
  {
    return _end;
  }

  /**
   * The team containing all units accessing this list.
   *
   * \return  The instance of Team that this list has been instantiated
   *          with
   */
  inline const Team & team() const noexcept
  {
    return *_team;
  }

  /**
   * The number of elements in the local part of the list.
   *
   * \return  The number of elements in the list that are local to the
   *          calling unit.
   */
  inline size_type lsize() const noexcept
  {
    return _lsize;
  }

  /**
   * The capacity of the local part of the list.
   *
   * \return  The number of allocated elements in the list that are local
   *          to the calling unit.
   */
  inline size_type lcapacity() const noexcept
  {
    return _lcapacity;
  }

  /**
   * Checks whether the list is empty.
   *
   * \return  True if \c size() is 0, otherwise false
   */
  inline bool empty() const noexcept
  {
    return size() == 0;
  }

  /**
   * Checks whether the given global index is local to the calling unit.
   *
   * \return  True if the list element referenced by the index is held
   *          in the calling unit's local memory
   */
  bool is_local(
    /// A global list index
    index_type global_index) const
  {
    return _pattern.is_local(global_index, _myid);
  }

  /**
   * Establish a barrier for all units operating on the list, publishing all
   * changes to all units.
   */
  void barrier() const
  {
    DASH_LOG_TRACE_VAR("List.barrier()", _team);
    _team->barrier();
    DASH_LOG_TRACE("List.barrier()", "passed barrier");
  }

  /**
   * The pattern used to distribute list elements to units.
   */
  inline const PatternType & pattern() const
  {
    return _pattern;
  }

  /**
   * The pattern used to distribute list elements to units.
   */
  inline PatternType & pattern()
  {
    return _pattern;
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
    DASH_LOG_TRACE("List.allocate()", nelem);
    DASH_LOG_TRACE_VAR("List.allocate", _team->dart_id());
    DASH_LOG_TRACE_VAR("List.allocate", team.dart_id());
    // Check requested capacity:
    if (nelem == 0) {
      DASH_THROW(
        dash::exception::InvalidArgument,
        "Tried to allocate dash::List with size 0");
    }
    if (_team == nullptr || *_team == dash::Team::Null()) {
      DASH_LOG_TRACE("List.allocate",
                     "initializing pattern with Team::All()");
      _team    = &team;
      _pattern = PatternType(nelem, distribution, team);
      DASH_LOG_TRACE_VAR("List.allocate", team.dart_id());
      DASH_LOG_TRACE_VAR("List.allocate", _pattern.team().dart_id());
    } else {
      DASH_LOG_TRACE("List.allocate",
                     "initializing pattern with initial team");
      _pattern = PatternType(nelem, distribution, *_team);
    }
    return allocate(_pattern);
  }

  void deallocate()
  {
    DASH_LOG_TRACE_VAR("List.deallocate()", this);
    DASH_LOG_TRACE_VAR("List.deallocate()", _size);
    // Assure all units are synchronized before deallocation, otherwise
    // other units might still be working on the list:
    if (dash::is_initialized()) {
      barrier();
    }
    // Remove this function from team deallocator list to avoid
    // double-free:
    _pattern.team().unregister_deallocator(
      this, std::bind(&List::deallocate, this));
    // Deallocate list elements:
    DASH_LOG_TRACE_VAR("List.deallocate()", _globmem);
    if (_globmem != nullptr) {
      delete _globmem;
      _globmem = nullptr;
    }
    _size = 0;
    DASH_LOG_TRACE_VAR("List.deallocate >", this);
  }

private:
  bool allocate(
    const PatternType & pattern)
  {
    DASH_LOG_TRACE("List._allocate()", "pattern",
                   pattern.memory_layout().extents());
    // Check requested capacity:
    _size      = 0;
    _lsize     = 0;
    _capacity  = pattern.capacity();
    _lcapacity = pattern.local_capacity();
    _team      = &pattern.team();
    // Initialize members:
    _myid      = pattern.team().myid();
    // Allocate local memory of identical size on every unit:
    DASH_LOG_TRACE_VAR("List._allocate", _lcapacity);
    DASH_LOG_TRACE_VAR("List._allocate", _lsize);
    if (_lcapacity == 0) {
      DASH_THROW(
        dash::exception::InvalidArgument,
        "Tried to allocate dash::List with local capacity 0");
    }
    _globmem   = new GlobMem_t(_lcapacity, pattern.team());
    // Global iterators:
    _begin     = iterator(_globmem, pattern);
    _end       = iterator(_begin) + _size;
    // Local iterators:
    _lbegin    = _globmem->lbegin(_myid);
    // More efficient than using _globmem->lend as this a second mapping
    // of the local memory segment:
    _lend      = _lbegin + pattern.local_size();
    DASH_LOG_TRACE_VAR("List._allocate", _myid);
    DASH_LOG_TRACE_VAR("List._allocate", _size);
    DASH_LOG_TRACE_VAR("List._allocate", _lsize);
    // Register deallocator of this list instance at the team
    // instance that has been used to initialized it:
    pattern.team().register_deallocator(
                     this, std::bind(&List::deallocate, this));
    // Assure all units are synchronized after allocation, otherwise
    // other units might start working on the list before allocation
    // completed at all units:
    if (dash::is_initialized()) {
      DASH_LOG_TRACE("List._allocate",
                     "waiting for allocation of all units");
      _team->barrier();
    }
    DASH_LOG_TRACE("List._allocate >", "finished");
    return true;
  }

private:
  /// Team containing all units interacting with the list
  dash::Team         * _team      = nullptr;
  /// DART id of the unit that created the list
  dart_unit_t          _myid;
  /// Element distribution pattern
  PatternType          _pattern;
  /// Global memory allocation and -access
  GlobMem_t          * _globmem   = nullptr;
  /// Iterator to initial element in the list
  iterator             _begin;
  /// Iterator past the last element in the list
  iterator             _end;
  /// Number of elements in the list
  size_type            _size;
  /// Number of local elements in the list
  size_type            _lsize;
  /// Element capacity in the list's currently allocated storage.
  size_type            _capacity;
  /// Element capacity in the list's currently allocated local storage.
  size_type            _lcapacity;
  /// Native pointer to first local element in the list
  local_iterator       _lbegin;
  /// Native pointer past the last local element in the list
  local_iterator       _lend;

};

} // namespace dash

#endif // DASH__LIST_H__
