#ifndef DASH__MAP_H__INCLUDED
#define DASH__MAP_H__INCLUDED

#include <dash/Types.h>
#include <dash/GlobRef.h>
#include <dash/Team.h>
#include <dash/Exception.h>
#include <dash/GlobDynamicMem.h>
#include <dash/Allocator.h>
#include <dash/Array.h>
#include <dash/Atomic.h>

#include <dash/map/MapRef.h>
#include <dash/map/LocalMapRef.h>
#include <dash/map/GlobMapIter.h>

#include <iterator>
#include <utility>
#include <limits>
#include <vector>
#include <functional>

namespace dash {

/**
 * \defgroup  DashMapConcept  Map Concept
 * Concept of a distributed map container.
 *
 * Container properties:
 *
 * - Associative: Elements are referenced by their key and not by their
 *   absolute position in the container.
 * - Ordered: Elements follow a strict order at all times. All inserted
 *   elements are given a position in this order.
 * - Map: Each element associates a key to a mapped value: Keys identify the
 *   elements which contain the mapped value.
 * - Unique keys: No to elements can have equivalent keys.
 * - Allocator-aware: The container uses an allocator object to manage
 *   acquisition and release of storage space.
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
 * <tt>key_type</tt>               | First template parameter <tt>Key</tt>
 * <tt>mapped_type</tt>            | Second template parameter <tt>Value</tt>
 * <tt>compare_type</tt>           | Third template parameter <tt>Compare</tt>
 * <tt>allocator_type</tt>         | Fourth template parameter <tt>AllocatorType</tt>
 * <tt>value_type</tt>             | <tt>std::pair<const key_type, mapped_type></tt>
 * <tt>difference_type</tt>        | A signed integral type, identical to <tt>iterator_traits<iterator>::difference_type</tt>
 * <tt>size_type</tt>              | Unsigned integral type to represent any non-negative value of <tt>difference_type</tt>
 * <tt>reference</tt>              | <tt>value_type &</tt>
 * <tt>const_reference</tt>        | <tt>const value_type &</tt>
 * <tt>pointer</tt>                | <tt>allocator_traits<allocator_type>::pointer</tt>
 * <tt>const_pointer</tt>          | <tt>allocator_traits<allocator_type>::const_pointer</tt>
 * <tt>iterator</tt>               | A bidirectional iterator to <tt>value_type</tt>
 * <tt>const_iterator</tt>         | A bidirectional iterator to <tt>const value_type</tt>
 * <tt>reverse_iterator</tt>       | <tt>reverse_iterator<iterator></tt>
 * <tt>const_reverse_iterator</tt> | <tt>reverse_iterator<const_iterator></tt>
 * <b>DASH-specific</b>            | &nbsp;
 * <tt>index_type</tt>             | A signed integgral type to represent positions in global index space
 * <tt>view_type</tt>              | Proxy type for views on map elements, implements \c DashMapConcept
 * <tt>local_type</tt>             | Proxy type for views on map elements that are local to the calling unit
 *
 * \par Member functions
 *
 * Function                     | Return type         | Definition
 * ---------------------------- | ------------------- | -----------------------------------------------
 * <b>Initialization</b>        | &nbsp;              | &nbsp;
 * <tt>operator=</tt>           | <tt>self &</tt>     | Assignment operator
 * <b>Iterators</b>             | &nbsp;              | &nbsp;
 * <tt>begin</tt>               | <tt>iterator</tt>   | Iterator to first element in the map
 * <tt>end</tt>                 | <tt>iterator</tt>   | Iterator past last element in the map
 * <b>Capacity</b>              | &nbsp;              | &nbsp;
 * <tt>size</tt>                | <tt>size_type</tt>  | Number of elements in the map
 * <tt>max_size</tt>            | <tt>size_type</tt>  | Maximum number of elements the map can hold
 * <tt>empty</tt>               | <tt>bool</tt>       | Whether the map is empty, i.e. size is 0
 * <b>Modifiers</b>             | &nbsp;              | &nbsp;
 * <tt>emplace</tt>             | <tt>iterator</tt>   | Construct and insert element at given position
 * <tt>insert</tt>              | <tt>iterator</tt>   | Insert elements before given position
 * <tt>erase</tt>               | <tt>iterator</tt>   | Erase elements at position or in range
 * <tt>swap</tt>                | <tt>void</tt>       | Swap content
 * <tt>clear</tt>               | <tt>void</tt>       | Clear the map's content
 * <b>Views (DASH specific)</b> | &nbsp;              | &nbsp;
 * <tt>local</tt>               | <tt>local_type</tt> | View on map elements local to calling unit
 * \}
 *
 * Usage examples:
 *
 * \code
 * // map of int (key type) to double (value type):
 * dash::Map<int, double> map;
 *
 * int myid = static_cast<int>(dash::myid());
 *
 * map.insert(std::make_pair(myid, 12.3);
 *
 * map.local.insert(std::make_pair(100 * myid, 12.3);
 * \endcode
 */

/**
 * A dynamic map container with support for workload balancing.
 *
 * \concept{DashContainerConcept}
 * \concept{DashMapConcept}
 */
template<
  class Key,
  class Mapped,
  class Compare = std::less<Key>,
  class Alloc   = dash::allocator::DynamicAllocator<
                    std::pair<const Key, Value> >
class Map
{
  template<class K_, class V_, class C_, class A_>
  friend class LocalMapRef;

private:
  typedef Map<Key, Mapped, Compare, Alloc>
    self_t;

public:
  typedef Key                                                       key_type;
  typedef Mapped                                                 mapped_type;
  typedef Compare                                                key_compare;
  typedef Alloc                                               allocator_type;
  typedef dash::default_index_t                              difference_type;
  typedef dash::default_size_t                                     size_type;
  typedef std::pair<const key_type, mapped_type>                  value_type;
  typedef GlobRef<value_type>                                      reference;
  typedef GlobRef<const value_type>                          const_reference;
  typedef GlobMapIter<value_type, Compare, Alloc>                   iterator;
  typedef GlobMapIter<const value_type, Compare, Alloc>       const_iterator;

  typedef       iterator                                             pointer;
  typedef const_iterator                                       const_pointer;

  typedef typename std::reverse_iterator<iterator>
    reverse_iterator;
  typedef typename std::reverse_iterator<const_iterator>
    const_reverse_iterator;

  typedef MapRef<Key, Mapped, Compare, Alloc>                      view_type;
  typedef LocalMapRef<Key, Mapped, Compare, Alloc>                local_type;

  typedef typename glob_mem_type::local_iterator
    local_iterator;
  typedef typename glob_mem_type::const_local_iterator
    const_local_iterator;
  typedef typename glob_mem_type::reverse_local_iterator
    reverse_local_iterator;
  typedef typename glob_mem_type::const_reverse_local_iterator
    const_reverse_local_iterator;

  typedef typename glob_mem_type::local_reference
    local_reference;
  typedef typename glob_mem_type::const_local_reference
    const_local_reference;

  typedef dash::Array<
            size_type, int, dash::CSRPattern<1, dash::ROW_MAJOR, int> >
    local_sizes_map;

  typedef struct {
    dart_unit_t unit;
    index_type  index;
  } key_local_pos;

  typedef std::function<key_local_pos(key_type)>
    key_mapping;

public:
  /// Local proxy object, allows use in range-based for loops.
  local_type local;

public:
  /**
   * Default constructor.
   *
   * Sets the associated team to DART_TEAM_NULL for global map instances
   * that are declared before \c dash::Init().
   */
  Map(
    size_type   nelem = 0,
    Team      & team  = dash::Team::Null())
  : local(this),
    _team(&team),
    _myid(team.myid()),
    _remote_size(0),
    _key_mapping(
      std::bind(&self_t::cyclic_key_mapping, this, std::placeholders::_1))
  {
    DASH_LOG_TRACE_VAR("Map(nelem,team)", nelem);
    if (_team->size() > 0) {
      _local_sizes.allocate(team.size(), dash::BLOCKED, team);
      _local_sizes.local[0] = 0;
      if (nelem > 0) {
        allocate(nelem);
        barrier();
      }
    }
    DASH_LOG_TRACE("Map(nelem,team) >");
  }

  /**
   * Constructor, creates a new constainer instance with the specified
   * initial global container capacity and associated units.
   */
  Map(
    size_type            nelem,
    /// Function mapping element keys to unit and local index.
    const key_mapping  & key_mapping_fun,
    Team               & team = dash::Team::All())
  : local(this),
    _team(&team),
    _myid(team.myid()),
    _remote_size(0),
    _key_mapping(key_mapping_fun)
  {
    DASH_LOG_TRACE_VAR("Map(nelem,kmap,team)", nelem);
    if (_team->size() > 0) {
      _local_sizes.allocate(team.size(), dash::BLOCKED, team);
      _local_sizes.local[0] = 0;
      if (nelem > 0) {
        allocate(nelem);
        barrier();
      }
    }
    DASH_LOG_TRACE("Map(nelem,kmap,team) >");
  }

  /**
   * Destructor, deallocates local and global memory acquired by the
   * container instance.
   */
  ~Map()
  {
    DASH_LOG_TRACE_VAR("Map.~Map()", this);
    deallocate();
    DASH_LOG_TRACE_VAR("Map.~Map >", this);
  }

  //////////////////////////////////////////////////////////////////////////
  // Distributed container
  //////////////////////////////////////////////////////////////////////////

  /**
   * The team containing all units accessing this map.
   *
   * \return  A reference to the Team containing the units associated with
   *          the container instance.
   */
  inline const Team & team() const noexcept
  {
    return *_team;
  }

  //////////////////////////////////////////////////////////////////////////
  // Dynamic distributed memory
  //////////////////////////////////////////////////////////////////////////

  /**
   * Synchronize changes on local and global memory space of the map since
   * initialization or the last call of its \c barrier method with global
   * memory.
   */
  void barrier()
  {
    DASH_LOG_TRACE_VAR("Map.barrier()", _team);
    // Apply changes in local memory spaces to global memory space:
    if (_globmem != nullptr) {
      _globmem->commit();
    }
    // Accumulate local sizes of remote units:
    _local_sizes.barrier();
    _remote_size = 0;
    for (int u = 0; u < _team->size(); ++u) {
      if (u != _myid) {
        size_type local_size_u  = _local_sizes[u];
        _remote_size           += local_size_u;
      }
    }
    DASH_LOG_TRACE("Map.barrier()", "passed barrier");
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
    DASH_LOG_TRACE("Map.allocate()");
    if (_team == nullptr || *_team == dash::Team::Null()) {
      DASH_LOG_TRACE("Map.allocate",
                     "initializing with Team::All()");
      _team = &team;
      DASH_LOG_TRACE_VAR("Map.allocate", team.dart_id());
    } else {
      DASH_LOG_TRACE("Map.allocate",
                     "initializing with initial team");
    }
    _remote_size = 0;
    auto lcap    = dash::math::div_ceil(nelem, _team->size());
    // Initialize members:
    _myid    = _team->myid();
    // Allocate local memory of identical size on every unit:
    DASH_LOG_TRACE_VAR("Map.allocate", lcap);

    _globmem = new glob_mem_type(lcap, *_team);
    // Global iterators:
    _begin   = iterator(_globmem, _nil_node);
    _end     = _begin;
    // Local iterators:
    _lbegin  = _globmem->lbegin(_myid);
    // More efficient than using _globmem->lend as this a second mapping
    // of the local memory segment:
    _lend    = _lbegin;
    DASH_LOG_TRACE_VAR("Map.allocate", _myid);
    // Register deallocator of this map instance at the team
    // instance that has been used to initialized it:
    _team->register_deallocator(
             this, std::bind(&Map::deallocate, this));
    // Assure all units are synchronized after allocation, otherwise
    // other units might start working on the map before allocation
    // completed at all units:
    if (dash::is_initialized()) {
      DASH_LOG_TRACE("Map.allocate",
                     "waiting for allocation of all units");
      _team->barrier();
    }
    DASH_LOG_TRACE("Map.allocate >", "finished");
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
    DASH_LOG_TRACE_VAR("Map.deallocate()", this);
    // Assure all units are synchronized before deallocation, otherwise
    // other units might still be working on the map:
    if (dash::is_initialized()) {
      barrier();
    }
    // Remove this function from team deallocator map to avoid
    // double-free:
    _team->unregister_deallocator(
      this, std::bind(&Map::deallocate, this));
    // Deallocate map elements:
    DASH_LOG_TRACE_VAR("Map.deallocate()", _globmem);
    if (_globmem != nullptr) {
      delete _globmem;
      _globmem = nullptr;
    }
    _local_sizes.local[0] = 0;
    _remote_size          = 0;
    DASH_LOG_TRACE_VAR("Map.deallocate >", this);
  }

  //////////////////////////////////////////////////////////////////////////
  // Global Iterators
  //////////////////////////////////////////////////////////////////////////

  /**
   * Global pointer to the beginning of the map.
   */
  inline iterator & begin() noexcept
  {
    return _begin;
  }

  /**
   * Global const pointer to the beginning of the map.
   */
  inline const_iterator begin() const noexcept
  {
    return const_iterator(_begin);
  }

  /**
   * Global const pointer to the beginning of the map.
   */
  inline const_iterator cbegin() const noexcept
  {
    return const_iterator(_begin);
  }

  /**
   * Global pointer to the end of the map.
   */
  inline iterator & end() noexcept
  {
    return _end;
  }

  /**
   * Global const pointer to the end of the map.
   */
  inline const_iterator end() const noexcept
  {
    return const_iterator(_end);
  }

  /**
   * Global const pointer to the end of the map.
   */
  inline const_iterator cend() const noexcept
  {
    return const_iterator(_end);
  }


  //////////////////////////////////////////////////////////////////////////
  // Local Iterators
  //////////////////////////////////////////////////////////////////////////

  inline local_iterator & lbegin() noexcept
  {
    return _lbegin;
  }

  inline const_local_iterator & lbegin() const noexcept
  {
    return const_local_iterator(_lbegin);
  }

  inline const_local_iterator & clbegin() const noexcept
  {
    return const_local_iterator(_lbegin);
  }

  inline local_iterator & lend() noexcept
  {
    return _lend;
  }

  inline const_local_iterator & lend() const noexcept
  {
    return const_local_iterator(_lend);
  }

  inline const_local_iterator & clend() const noexcept
  {
    return const_local_iterator(_lend);
  }

  //////////////////////////////////////////////////////////////////////////
  // Capacity
  //////////////////////////////////////////////////////////////////////////

  /**
   * Maximum number of elements a map container can hold, e.g. due to
   * system limitations.
   * The maximum size is not guaranteed.
   */
  constexpr max_size() const noexcept
  {
    return std::numeric_limits<key_type>::max();
  }

  /**
   * The size of the map.
   *
   * \return  The number of elements in the map.
   */
  inline size_type size() const noexcept
  {
    return _remote_size + _local_sizes.local[0];
  }

  /**
   * The number of elements that can be held in currently allocated storage
   * of the map.
   *
   * \return  The number of elements in the map.
   */
  inline size_type capacity() const noexcept
  {
    return _globmem->size();
  }

  /**
   * Whether the map is empty.
   *
   * \return  true if \c size() is 0, otherwise false
   */
  inline bool empty() const noexcept
  {
    return _size == 0;
  }

  /**
   * The number of elements in the local part of the map.
   *
   * \return  The number of elements in the map that are local to the
   *          calling unit.
   */
  inline size_type lsize() const noexcept
  {
    return _local_sizes.local[0];
  }

  /**
   * The capacity of the local part of the map.
   *
   * \return  The number of allocated elements in the map that are local
   *          to the calling unit.
   */
  inline size_type lcapacity() const noexcept
  {
    return _globmem != nullptr
           ? _globmem->local_size()
           : 0;
  }

  //////////////////////////////////////////////////////////////////////////
  // Element access
  //////////////////////////////////////////////////////////////////////////

  /**
   * If \c key matches the key of an element in the container, returns a
   * reference to its mapped value.
   *
   * If \c key does not match the key of any element in the container,
   * inserts a new element with that key and returns a reference to its
   * mapped value.
   * Notice that this always increases the container size by one, even if no
   * mapped value is assigned to the element. The element is then constructed
   * using its default constructor.
   *
   * Equivalent to:
   *
   * \code
   *   (*(
   *     (this->insert(std::make_pair(key, mapped_type())))
   *     .first)
   *   ).second;
   * \endcode
   *
   * Member function \c at() has the same behavior when an element with the
   * key exists, but throws an exception when it does not.
   *
   * \return   reference to the mapped value of the element with a key value
   *           equivalent to \c key.
   */
  reference operator[](const key_type & key)
  {
    return (*((insert(
                std::make_pair(key, mapped_type())
              )).first))
           .second;
  }

  /**
   * If \c key matches the key of an element in the container, returns a
   * reference to its mapped value.
   *
   * Throws an exception if \c key does not match the key of any element in
   * the container.
   *
   * Member function \c operator[]() has the same behavior when an element
   * with the key exists, but does not throw an exception when it does not.
   *
   * \return   reference to the mapped value of the element with a key value
   *           equivalent to \c key.
   */
  reference at(const key_type & key)
  {
    auto insertion = insert(
                       std::make_pair(key, mapped_type())
                     );
    if (!insertion.second) {
      // Equivalent key existed in map, return mapped element:
      return (*(insertion.first)).second;
    }
    // No equivalent key in map, throw:
    DASH_THROW(
      dash::exception::InvalidArgument,
      "No element in map for key " << key);
  }

  //////////////////////////////////////////////////////////////////////////
  // Modifiers
  //////////////////////////////////////////////////////////////////////////

  /**
   * Insert a new element as key-value pair, increasing the container size
   * by 1.
   * Internally, map containers keep all their elements sorted by their key
   * following the criterion specified by its comparison object.
   * The elements are always inserted in its respective position following
   * this ordering.
   *
   * \see     \c operator[]
   *
   * \return  pair, with its member pair::first set to an iterator pointing
   *          to either the newly inserted element or to the element with an
   *          equivalent key in the map.
   *          The pair::second element in the pair is set to true if a new
   *          element was inserted or false if an equivalent key already
   *          existed.
   */
  std::pair<iterator, bool> insert(
    /// The element to insert.
    const value_type & value)
  {
    auto result   = std::make_pair(_end, false);
    auto key      = value.first;
    auto mapped   = value.second;
    // Resolve insertion position of element from key mapping:
    auto l_pos    = _key_mapping(key);
    // Unit assigned to range containing the given key:
    auto unit     = l_pos.unit;
    // Offset of key in local memory:
    auto l_idx    = l_pos.index;
    if (unit == _myid) {
      // Local insertion, target unit of element is active unit:
    } else {
      // Remote insertion:
      // Local capacity and number of elements of remote unit:
      size_type remote_cap  = _globmem->local_size(unit);
      size_type remote_size = _local_sizes[unit];
      // Global iterator to element insert position:
      auto      g_it        = _globmem->at(unit, l_idx);
      if (element_found) {
        // Existing element with equivalent key found, no insertion:
        result.second = false;
      } else {
        // No element with equivalent key found, try to add new element:
        result.second = true;
        if (remote_cap > remote_size) {
          // Emplace new element at remote unit:
          // Atomic increment of local size of remote unit:
          size_type new_remote_size = dash::Atomic<size_type>(
                                        _local_sizes[unit]
                                      ).fetch_and_add(1);
          // Check local capacity of remote unit after atomic increment:
          if (remote_cap < new_remote_size) {
            DASH_THROW(
              dash::exception::RuntimeError,
              "Map.insert failed: local capacity of target unit exceeded");
          }
        }
      }
    }
    return result;
  }

  /**
   * Insert elements in iterator range of key-value pairs, increasing the
   * container size by the number of elements in the range.
   * Internally, map containers keep all their elements sorted by their key
   * following the criterion specified by its comparison object.
   * The elements are always inserted in its respective position following
   * this ordering.
   */
  template<class InputIterator>
  void insert(
    InputIterator first,
    InputIterator last)
  {
  }

  /**
   * Removes and destroys single element referenced by given iterator from
   * the container, decreasing the container size by 1.
   *
   * \return  iterator to the element that follows the last element removed,
   *          or \c end() if the last element was removed.
   */
  iterator erase(
    const_iterator position)
  {
  }

  /**
   * Removes and destroys elements referenced by the given key from the
   * container, decreasing the container size by the number of elements
   * removed.
   *
   * \return  The number of elements removed.
   */
  size_type erase(
    /// Key of the container element to remove.
    const key_type & key)
  {
  }

  /**
   * Removes and destroys elements in the given range from the container,
   * decreasing the container size by the number of elements removed.
   *
   * \return  iterator to the element that follows the last element removed,
   *          or \c end() if the last element was removed.
   */
  iterator erase(
    /// Iterator at first element to remove.
    const_iterator first,
    /// Iterator past the last element to remove.
    const_iterator last)
  {
  }

private:
  /**
   * Simplistic cyclic key mapping function.
   */
  key_local_pos cyclic_key_mapping(key_type key) const
  {
    key_local_pos lpos;
    auto unit  = static_cast<dart_unit_t>(key) % _team->size();
    auto lcap  = _globmem->local_size(unit);
    lpos.unit  = unit;
    lpos.index = static_cast<index_type>(key) % unit_lcap;
    return lpos;
  }

private:
  /// Team containing all units interacting with the map.
  dash::Team           * _team        = nullptr;
  /// DART id of the unit that created the map.
  dart_unit_t            _myid;
  /// Global memory allocation and -access.
  glob_mem_type        * _globmem     = nullptr;
  /// Iterator to initial element in the map.
  iterator               _begin;
  /// Iterator past the last element in the map.
  iterator               _end;
  /// Number of elements in the map.
  size_type              _remote_size = 0;
  /// Native pointer to first local element in the map.
  local_iterator         _lbegin;
  /// Native pointer past the last local element in the map.
  local_iterator         _lend;
  /// Mapping units to their number of local map elements.
  local_sizes_map        _local_sizes;
  /// Mapping of key to unit and local offset.
  key_mapping            _key_mapping;

}; // class Map

} // namespace dash

#endif // DASH__MAP_H__INCLUDED
