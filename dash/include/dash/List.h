#ifndef DASH__LIST_H__
#define DASH__LIST_H__

#include <dash/Types.h>
#include <dash/GlobRef.h>
#include <dash/Team.h>
#include <dash/Exception.h>
#include <dash/GlobDynamicMem.h>
#include <dash/Allocator.h>
#include <dash/Array.h>

#include <dash/list/ListRef.h>
#include <dash/list/LocalListRef.h>
#include <dash/list/GlobListIter.h>
#include <dash/list/internal/ListTypes.h>

#include <iterator>
#include <limits>
#include <vector>

namespace dash {

/**
 * \defgroup  DashListConcept  List Concept
 * Concept of a distributed one-dimensional list container.
 *
 * \ingroup DashContainerConcept
 * \{
 * \par Description
 *
 * A dynamic double-linked list.
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

/**
 * A dynamic bi-directional list with support for workload balancing.
 *
 * \concept{DashListConcept}
 */
template<
  typename ElementType,
  class    AllocatorType = dash::allocator::DynamicAllocator<ElementType> >
class List
{
  template<typename T_, class A_>
  friend class LocalListRef;

private:
  typedef List<ElementType, AllocatorType> self_t;

/// Public types as required by DASH list concept
public:
  typedef ElementType                                             value_type;
  typedef typename dash::default_index_t                          index_type;
  typedef typename dash::default_size_t                            size_type;
  typedef AllocatorType                                       allocator_type;

  typedef ListRef<ElementType, AllocatorType>                      view_type;
  typedef LocalListRef<ElementType, AllocatorType>                local_type;

private:
  typedef internal::ListNode<value_type>
    node_type;

  typedef typename AllocatorType::template rebind<
                     internal::ListNode<ElementType> >::other
    node_allocator_type;

  typedef dash::GlobDynamicMem<node_type, node_allocator_type>
    glob_mem_type;

  typedef dash::Array<
            size_type, int, dash::CSRPattern<1, dash::ROW_MAJOR, int> >
    local_sizes_map;

/// Public types as required by STL list concept
public:
  typedef index_type                                         difference_type;

  typedef GlobListIter<value_type, glob_mem_type>                   iterator;
  typedef GlobListIter<const value_type, glob_mem_type>       const_iterator;

  typedef std::reverse_iterator<      iterator>             reverse_iterator;
  typedef std::reverse_iterator<const_iterator>       const_reverse_iterator;

  typedef GlobRef<value_type>                                      reference;
  typedef GlobRef<const value_type>                          const_reference;

  typedef       value_type &                                 local_reference;
  typedef const value_type &                           const_local_reference;

  typedef       iterator                                             pointer;
  typedef const_iterator                                       const_pointer;

  typedef typename glob_mem_type::local_iterator
    local_node_iterator;
  typedef typename glob_mem_type::const_local_iterator
    const_local_node_iterator;
  typedef typename glob_mem_type::reverse_local_iterator
    reverse_local_node_iterator;
  typedef typename glob_mem_type::const_reverse_local_iterator
    const_reverse_local_node_iterator;

  typedef typename glob_mem_type::local_reference
    local_node_reference;
  typedef typename glob_mem_type::const_local_reference
    const_local_node_reference;

  // TODO: define ListLocalIter to dereference node iterators
  typedef               local_node_iterator                   local_iterator;
  typedef         const_local_node_iterator             const_local_iterator;
  typedef       reverse_local_node_iterator           reverse_local_iterator;
  typedef const_reverse_local_node_iterator     const_reverse_local_iterator;

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
    _myid(team.myid()),
    _remote_size(0)
  {
    DASH_LOG_TRACE("List() >", "default constructor");
  }

  /**
   * Constructor, creates a new constainer instance with the specified
   * initial global container capacity and associated units.
   */
  List(
    size_type   nelem = 0,
    Team      & team  = dash::Team::All())
  : local(this),
    _team(&team),
    _myid(team.myid()),
    _remote_size(0)
  {
    DASH_LOG_TRACE("List(nelem,team)", "nelem:", nelem);
    if (_team->size() > 0) {
      _local_sizes.allocate(team.size(), dash::BLOCKED, team);
      _local_sizes.local[0] = 0;
    }
    allocate(nelem);
    barrier();
    DASH_LOG_TRACE("List(nelem,team) >");
  }

  /**
   * Constructor, creates a new constainer instance with the specified
   * initial global container capacity and associated units.
   */
  List(
    size_type   nelem,
    size_type   nlbuf,
    Team      & team   = dash::Team::All())
  : local(this),
    _team(&team),
    _myid(team.myid()),
    _remote_size(0),
    _local_buffer_size(nlbuf)
  {
    DASH_LOG_TRACE("List(nelem,nlbuf,team)",
                   "nelem:", nelem, "nlbuf:", nlbuf);
    if (_team->size() > 0) {
      _local_sizes.allocate(team.size(), dash::BLOCKED, team);
      _local_sizes.local[0] = 0;
    }
    allocate(nelem);
    barrier();
    DASH_LOG_TRACE("List(nelem,nlbuf,team) >");
  }

  /**
   * Destructor, deallocates local and global memory acquired by the
   * container instance.
   */
  ~List()
  {
    DASH_LOG_TRACE_VAR("List.~List()", this);
    deallocate();
    DASH_LOG_TRACE_VAR("List.~List >", this);
  }

  /**
   * Inserts a new element at the end of the list, after its current
   * last element. The content of \c value is copied or moved to the
   * inserted element.
   * Increases the container size by one.
   *
   * The operation takes immediate effect for the calling unit.
   * For other units, changes will only be visible after the next call of
   * \c barrier.
   * As one-sided, non-collective allocation on remote units is not possible
   * with most DART communication backends, the new list element is allocated
   * locally and moved to its final position in global memory in \c barrier.
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
   *
   * The operation takes immediate effect for the calling unit.
   * For other units, changes will only be visible after the next call of
   * \c barrier.
   * As one-sided, non-collective allocation on remote units is not possible
   * with most DART communication backends, the new list element is allocated
   * locally and moved to its final position in global memory in \c barrier.
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
  const_local_iterator lbegin() const noexcept
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
  const_local_iterator lend() const noexcept
  {
    return _lend;
  }

  /**
   * Maximum number of elements a list container can hold, e.g. due to
   * system limitations.
   * The maximum size is not guaranteed.
   */
  constexpr size_type max_size() const noexcept
  {
    return std::numeric_limits<index_type>::max();
  }

  /**
   * The size of the list.
   *
   * \return  The number of elements in the list.
   */
  inline size_type size() const noexcept
  {
    return _remote_size + _local_sizes.local[0];
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
    return _globmem->size();
  }

  /**
   * Removes and destroys single element referenced by given iterator from
   * the container, decreasing the container size by 1.
   *
   * \return  iterator to the element that follows the last element removed,
   *          or \c end() if the last element was removed.
   */
  inline iterator erase(const_iterator position)
  {
    return _begin;
  }

  /**
   * Removes and destroys elements in the given range from the container,
   * decreasing the container size by the number of elements removed.
   *
   * \return  iterator to the element that follows the last element removed,
   *          or \c end() if the last element was removed.
   */
  inline iterator erase(const_iterator first, const_iterator last)
  {
    return _end;
  }

  /**
   * The team containing all units accessing this list.
   *
   * \return  A reference to the Team containing the units associated with
   *          the container instance.
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
    return _local_sizes.local[0];
  }

  /**
   * The capacity of the local part of the list.
   *
   * \return  The number of allocated elements in the list that are local
   *          to the calling unit.
   */
  inline size_type lcapacity() const noexcept
  {
    return _globmem != nullptr
           ? _globmem->local_size()
           : 0;
  }

  /**
   * Whether the list is empty.
   *
   * \return  true if \c size() is 0, otherwise false
   */
  inline bool empty() const noexcept
  {
    return size() == 0;
  }

  /**
   * Establish a barrier for all units operating on the list, publishing all
   * changes to all units.
   */
  void barrier()
  {
    DASH_LOG_TRACE_VAR("List.barrier()", _team);
    // Apply changes in local memory spaces to global memory space:
    if (_globmem != nullptr) {
      _globmem->commit();
    }
    // Accumulate local sizes of remote units:
    _remote_size = 0;
    for (int u = 0; u < _team->size(); ++u) {
      if (u != _myid) {
        size_type local_size_u  = _local_sizes[u];
        _remote_size           += local_size_u;
      }
    }
    DASH_LOG_TRACE("List.barrier()", "passed barrier");
  }

  /**
   * Allocate memory for this container in global memory.
   *
   * Calls implicit barrier on the team associated with the container
   * instance.
   */
  bool allocate(
    /// Initial global capacity of the container.
    size_type    nelem = 0,
    /// Team containing all units associated with the container.
    dash::Team & team  = dash::Team::All())
  {
    DASH_LOG_TRACE("List.allocate()");
    DASH_LOG_TRACE_VAR("List.allocate", nelem);
    DASH_LOG_TRACE_VAR("List.allocate", _local_buffer_size);
    if (_team == nullptr || *_team == dash::Team::Null()) {
      DASH_LOG_TRACE("List.allocate",
                     "initializing with Team::All()");
      _team = &team;
      DASH_LOG_TRACE_VAR("List.allocate", team.dart_id());
    } else {
      DASH_LOG_TRACE("List.allocate",
                     "initializing with initial team");
    }
    DASH_ASSERT_GT(_local_buffer_size, 0, "local buffer size must not be 0");
    if (nelem < _team->size() * _local_buffer_size) {
      nelem = _team->size() * _local_buffer_size;
    }
    _remote_size = 0;
    auto lcap    = dash::math::div_ceil(nelem, _team->size());
    // Initialize members:
    _myid        = _team->myid();
    // Allocate local memory of identical size on every unit:
    DASH_LOG_TRACE_VAR("List.allocate", lcap);

    _globmem     = new glob_mem_type(lcap, *_team);
    // Global iterators:
    _begin       = iterator(_globmem, _nil_node);
    _end         = _begin;
    // Local iterators:
    _lbegin      = _globmem->lbegin(_myid);
    // More efficient than using _globmem->lend as this a second mapping
    // of the local memory segment:
    _lend        = _lbegin;
    DASH_LOG_TRACE_VAR("List.allocate", _myid);
    // Register deallocator of this list instance at the team
    // instance that has been used to initialized it:
    _team->register_deallocator(
             this, std::bind(&List::deallocate, this));
    // Assure all units are synchronized after allocation, otherwise
    // other units might start working on the list before allocation
    // completed at all units:
    if (dash::is_initialized()) {
      DASH_LOG_TRACE("List.allocate",
                     "waiting for allocation of all units");
      _team->barrier();
    }
    DASH_LOG_TRACE("List.allocate >", "finished");
    return true;
  }

  /**
   * Free global memory allocated by this container instance.
   *
   * Calls implicit barrier on the team associated with the container
   * instance.
   */
  void deallocate()
  {
    DASH_LOG_TRACE_VAR("List.deallocate()", this);
    // Assure all units are synchronized before deallocation, otherwise
    // other units might still be working on the list:
    if (dash::is_initialized()) {
      barrier();
    }
    // Remove this function from team deallocator list to avoid
    // double-free:
    _team->unregister_deallocator(
      this, std::bind(&List::deallocate, this));
    // Deallocate list elements:
    DASH_LOG_TRACE_VAR("List.deallocate()", _globmem);
    if (_globmem != nullptr) {
      delete _globmem;
      _globmem = nullptr;
    }
    _local_sizes.local[0] = 0;
    _remote_size          = 0;
    DASH_LOG_TRACE_VAR("List.deallocate >", this);
  }

private:
  /// Team containing all units interacting with the list.
  dash::Team         * _team
                         = nullptr;
  /// DART id of the unit that created the list.
  team_unit_t          _myid;
  /// Global memory allocation and -access.
  glob_mem_type      * _globmem
                         = nullptr;
  /// Iterator to initial element in the list.
  iterator             _begin;
  /// Iterator past the last element in the list.
  iterator             _end;
  /// Number of elements in the list.
  size_type            _remote_size
                         = 0;
  /// Native pointer to first local element in the list.
  local_iterator       _lbegin;
  /// Native pointer past the last local element in the list.
  local_iterator       _lend;
  /// Sentinel node in empty list.
  node_type            _nil_node;
  /// Mapping units to their number of local list elements.
  local_sizes_map      _local_sizes;
  /// Capacity of local buffer containing locally added node elements that
  /// have not been committed to global memory yet.
  /// Default is 4 KB.
  size_type            _local_buffer_size
                         = 4096 / sizeof(value_type);

};

} // namespace dash

#endif // DASH__LIST_H__
