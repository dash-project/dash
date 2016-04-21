#ifndef DASH__UNORDERED_MAP_H__INCLUDED
#define DASH__UNORDERED_MAP_H__INCLUDED

#include <dash/Types.h>
#include <dash/GlobRef.h>
#include <dash/Team.h>
#include <dash/Exception.h>
#include <dash/Array.h>
#include <dash/Atomic.h>
#include <dash/GlobDynamicMem.h>
#include <dash/Allocator.h>

// #include <dash/map/UnorderedMapRef.h>
// #include <dash/map/LocalUnorderedMapRef.h>
#include <dash/map/LocalUnorderedMapIter.h>
#include <dash/map/GlobUnorderedMapIter.h>

#include <iterator>
#include <utility>
#include <limits>
#include <vector>
#include <functional>
#include <algorithm>
#include <cstddef>

namespace dash {

/**
 * \defgroup  DashUnorderedMapConcept  Unordered Map Container Concept
 * Concept of a distributed unordered map container.
 *
 * \ingroup DashConcept
 * \{
 * \par Description
 *
 * Different from regular maps, elements in unordered map containers are
 * not sorted in any particular order, but organized into buckets depending
 * on their hash values. Unordered maps allow for fast access to individual
 * elements as the storage location of a key in global and/or local memory
 * can be resolved directly from its hash value.
 *
 * Container properties:
 *
 * - Associative: Elements are referenced by their key and not by their
 *   absolute position in the container.
 * - Unordered: Elements follow no order and are organized using hash tables.
 * - Map: Each element associates a key to a mapped value: Keys identify the
 *   elements which contain the mapped value.
 * - Unique keys: No to elements can have equivalent keys.
 * - Allocator-aware: The container uses an allocator object to manage
 *   acquisition and release of storage space.
 *
 * Iterator validity:
 *
 * - All iterators in the container remain valid after the insertion unless
 *   it forces a rehash. In this case, all iterators in the container are
 *   invalidated.
 * - A rehash is forced if the new container size after the insertion
 *   operation would increase above its capacity threshold.
 * - References to elements in the map container remain valid in all cases,
 *   even after a rehash.
 *
 * \par Member types
 *
 * Type                            | Definition
 * ------------------------------- | ----------------------------------------------------------------------------------------
 * <b>STL</b>                      | &nbsp;
 * <tt>key_type</tt>               | First template parameter <tt>Key</tt>
 * <tt>mapped_type</tt>            | Second template parameter <tt>Mapped</tt>
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
 * <tt>view_type</tt>              | Proxy type for views on map elements, implements \c DashUnorderedMapConcept
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
 * dash::UnorderedMap<int, double> map;
 *
 * int myid = static_cast<int>(dash::myid());
 *
 * map.insert(std::make_pair(myid, 12.3);
 *
 * map.local.insert(std::make_pair(100 * myid, 12.3);
 * \endcode
 */

template<typename Key>
class HashLocal
{
private:
  typedef dash::default_size_t size_type;

public:
  typedef Key          argument_type;
  typedef dart_unit_t  result_type;

public:
  /**
   * Default constructor.
   */
  HashLocal()
  : _team(nullptr),
    _nunits(0),
    _myid(DART_UNDEFINED_UNIT_ID)
  { }

  /**
   * Constructor.
   */
  HashLocal(
    dash::Team & team)
  : _team(&team),
    _nunits(team.size()),
    _myid(team.myid())
  { }

  result_type operator()(
    const argument_type & key) const
  {
    return _myid;
  }

private:
  dash::Team * _team   = nullptr;
  size_type    _nunits = 0;
  dart_unit_t  _myid   = DART_UNDEFINED_UNIT_ID;

}; // class HashLocal

// Forward-declaration.
template<
  typename Key,
  typename Mapped,
  typename Hash,
  typename Pred,
  typename Alloc >
class UnorderedMap;

template<
  typename Key,
  typename Mapped,
  typename Hash,
  typename Pred,
  typename Alloc >
class LocalUnorderedMapRef
{
private:
  typedef LocalUnorderedMapRef<Key, Mapped, Hash, Pred, Alloc>
    self_t;

  typedef UnorderedMap<Key, Mapped, Hash, Pred, Alloc>
    map_type;

public:
  LocalUnorderedMapRef(
    map_type * map = nullptr)
  : _map(map)
  { }

private:
  map_type * _map = nullptr;

}; // class LocalUnorderedMapRef

/**
 * A dynamic map container with support for workload balancing.
 *
 * \concept{DashContainerConcept}
 * \concept{DashUnorderedMapConcept}
 */
template<
  typename Key,
  typename Mapped,
  typename Hash    = dash::HashLocal<Key>,
  typename Pred    = std::equal_to<Key>,
  typename Alloc   = dash::allocator::DynamicAllocator<
                       std::pair<const Key, Mapped> > >
class UnorderedMap
{
  template<typename K_, typename M_, typename H_, typename P_, typename A_>
  friend class LocalUnorderedMapRef;

  template<typename K_, typename M_, typename H_, typename P_, typename A_>
  friend class GlobUnorderedMapIter;

  template<typename K_, typename M_, typename H_, typename P_, typename A_>
  friend class LocalUnorderedMapIter;

private:
  typedef UnorderedMap<Key, Mapped, Hash, Pred, Alloc>
    self_t;

public:
  typedef Key                                                       key_type;
  typedef Mapped                                                 mapped_type;
  typedef Hash                                                        hasher;
  typedef Pred                                                     key_equal;
  typedef Alloc                                               allocator_type;

  typedef dash::default_index_t                                   index_type;
  typedef dash::default_index_t                              difference_type;
  typedef dash::default_size_t                                     size_type;
  typedef std::pair<const key_type, mapped_type>                  value_type;

  typedef LocalUnorderedMapRef<Key, Mapped, Hash, Pred, Alloc>    local_type;

  typedef dash::GlobDynamicMem<value_type, allocator_type>     glob_mem_type;

  typedef typename glob_mem_type::reference                        reference;
  typedef typename glob_mem_type::const_reference            const_reference;

  typedef typename reference::template rebind<mapped_type>::other
    mapped_type_reference;
  typedef typename const_reference::template rebind<mapped_type>::other
    const_mapped_type_reference;

  typedef typename glob_mem_type::global_iterator
    node_iterator;
  typedef typename glob_mem_type::const_global_iterator
    const_node_iterator;
  typedef typename glob_mem_type::local_iterator
    local_node_iterator;
  typedef typename glob_mem_type::const_local_iterator
    const_local_node_iterator;
  typedef typename glob_mem_type::reverse_global_iterator
    reverse_node_iterator;
  typedef typename glob_mem_type::const_reverse_global_iterator
    const_reverse_node_iterator;
  typedef typename glob_mem_type::reverse_local_iterator
    reverse_local_node_iterator;
  typedef typename glob_mem_type::const_reverse_local_iterator
    const_reverse_local_node_iterator;

  typedef typename glob_mem_type::global_iterator
    local_node_pointer;
  typedef typename glob_mem_type::const_global_iterator
    const_local_node_pointer;

  typedef GlobUnorderedMapIter<Key, Mapped, Hash, Pred, Alloc>
    iterator;
  typedef GlobUnorderedMapIter<Key, Mapped, Hash, Pred, Alloc>
    const_iterator;
  typedef typename std::reverse_iterator<iterator>
    reverse_iterator;
  typedef typename std::reverse_iterator<const_iterator>
    const_reverse_iterator;

  typedef LocalUnorderedMapIter<Key, Mapped, Hash, Pred, Alloc>
    local_pointer;
  typedef LocalUnorderedMapIter<Key, Mapped, Hash, Pred, Alloc>
    const_local_pointer;
  typedef LocalUnorderedMapIter<Key, Mapped, Hash, Pred, Alloc>
    local_iterator;
  typedef LocalUnorderedMapIter<Key, Mapped, Hash, Pred, Alloc>
    const_local_iterator;
  typedef typename std::reverse_iterator<local_iterator>
    reverse_local_iterator;
  typedef typename std::reverse_iterator<const_local_iterator>
    const_reverse_local_iterator;

  typedef typename glob_mem_type::local_reference
    local_reference;
  typedef typename glob_mem_type::const_local_reference
    const_local_reference;

  typedef dash::Array<
            size_type, int, dash::CSRPattern<1, dash::ROW_MAJOR, int> >
    local_sizes_map;

public:
  /// Local proxy object, allows use in range-based for loops.
  local_type local;

public:
  /**
   * Constructor.
   *
   * Sets the associated team to DART_TEAM_NULL for global map instances
   * that are declared before \c dash::Init().
   */
  UnorderedMap(
    size_type   nelem = 0,
    Team      & team  = dash::Team::All())
  : local(this),
    _team(&team),
    _myid(team.myid()),
    _remote_size(0),
    _key_hash(team)
  {
    DASH_LOG_TRACE_VAR("UnorderedMap(nelem,team)", nelem);
    if (_team->size() > 0) {
      allocate(nelem, team);
    }
    DASH_LOG_TRACE("UnorderedMap(nelem,team) >");
  }

  /**
   * Constructor.
   *
   * Sets the associated team to DART_TEAM_NULL for global map instances
   * that are declared before \c dash::Init().
   */
  UnorderedMap(
    size_type   nelem,
    size_type   nlbuf,
    Team      & team  = dash::Team::All())
  : local(this),
    _team(&team),
    _myid(team.myid()),
    _remote_size(0),
    _key_hash(team),
    _local_buffer_size(nlbuf)
  {
    DASH_LOG_TRACE("UnorderedMap(nelem,nlbuf,team)",
                   "nelem:", nelem, "nlbuf:", nlbuf);
    if (_team->size() > 0) {
      allocate(nelem, team);
    }
    DASH_LOG_TRACE("UnorderedMap(nelem,nlbuf,team) >");
  }

  /**
   * Destructor, deallocates local and global memory acquired by the
   * container instance.
   */
  ~UnorderedMap()
  {
    DASH_LOG_TRACE_VAR("UnorderedMap.~UnorderedMap()", this);
    deallocate();
    DASH_LOG_TRACE_VAR("UnorderedMap.~UnorderedMap >", this);
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
    if (_team != nullptr) {
      return *_team;
    }
    return dash::Team::Null();
  }

  //////////////////////////////////////////////////////////////////////////
  // Dynamic distributed memory
  //////////////////////////////////////////////////////////////////////////

  /**
   * Synchronize changes on local and global memory space of the map since
   * initialization or the last call of its \c barrier method with global
   * memory.
   *
   * TODO: Implement moving elements, integrate rebalance policy type.
   */
  void barrier()
  {
    DASH_LOG_TRACE_VAR("UnorderedMap.barrier()", _team->dart_id());
    // Apply changes in local memory spaces to global memory space:
    if (_globmem != nullptr) {
      _globmem->commit();
    }
    // Accumulate local sizes of remote units:
    _local_sizes.barrier();
    _remote_size = 0;
    for (int u = 0; u < _team->size(); ++u) {
      size_type local_size_u;
      if (u != _myid) {
        local_size_u = _local_sizes[u];
        _remote_size += local_size_u;
      } else {
        local_size_u = _local_sizes.local[0];
      }
      _local_cumul_sizes[u] = local_size_u;
      if (u > 0) {
        _local_cumul_sizes[u] += _local_cumul_sizes[u-1];
      }
      DASH_LOG_TRACE("UnorderedMap.barrier",
                     "local size at unit", u, ":", local_size_u,
                     "cumulative size:", _local_cumul_sizes[u]);
    }
    auto new_size = size();
    DASH_LOG_TRACE("UnorderedMap.barrier", "new size:", new_size);
    DASH_ASSERT_EQ(_remote_size, new_size - _local_sizes.local[0],
                   "invalid size after global commit");
    _begin = iterator(this, 0);
    _end   = iterator(this, new_size);
    DASH_LOG_TRACE("UnorderedMap.barrier >", "passed barrier");
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
    DASH_LOG_TRACE("UnorderedMap.allocate()");
    DASH_LOG_TRACE_VAR("UnorderedMap.allocate", nelem);
    DASH_LOG_TRACE_VAR("UnorderedMap.allocate", _local_buffer_size);
    if (_team == nullptr || *_team == dash::Team::Null()) {
      DASH_LOG_TRACE("UnorderedMap.allocate",
                     "initializing with specified team -",
                     "team size:", team.size());
      _team = &team;
      DASH_LOG_TRACE_VAR("UnorderedMap.allocate", team.dart_id());
    } else {
      DASH_LOG_TRACE("UnorderedMap.allocate",
                     "initializing with initial team");
    }
    _local_cumul_sizes = std::vector<size_type>(_team->size(), 0);
    DASH_ASSERT_GT(_local_buffer_size, 0, "local buffer size must not be 0");
    if (nelem < _team->size() * _local_buffer_size) {
      nelem = _team->size() * _local_buffer_size;
      DASH_LOG_TRACE("UnorderedMap.allocate", "nelem increased to", nelem);
    }
    _key_hash    = hasher(*_team);
    _remote_size = 0;
    auto lcap    = dash::math::div_ceil(nelem, _team->size());
    // Initialize members:
    _myid        = _team->myid();

    DASH_LOG_TRACE("UnorderedMap.allocate", "initialize global memory,",
                   "local capacity:", lcap);
    _globmem     = new glob_mem_type(lcap, *_team);
    DASH_LOG_TRACE("UnorderedMap.allocate", "global memory initialized");

    // Initialize local sizes with 0:
    _local_sizes.allocate(_team->size(), dash::BLOCKED, *_team);
    _local_sizes.local[0] = 0;
    _local_size_gptr      = _local_sizes[_myid].dart_gptr();

    // Global iterators:
    _begin       = iterator(this, 0);
    _end         = _begin;
    DASH_LOG_TRACE_VAR("UnorderedMap.allocate", _begin);
    DASH_LOG_TRACE_VAR("UnorderedMap.allocate", _end);
    // Local iterators:
    _lbegin      = local_iterator(this, 0);
    _lend        = _lbegin;
    DASH_LOG_TRACE_VAR("UnorderedMap.allocate", _lbegin);
    DASH_LOG_TRACE_VAR("UnorderedMap.allocate", _lend);
    // Register deallocator of this map instance at the team
    // instance that has been used to initialized it:
    _team->register_deallocator(
             this, std::bind(&UnorderedMap::deallocate, this));
    // Assure all units are synchronized after allocation, otherwise
    // other units might start working on the map before allocation
    // completed at all units:
    if (dash::is_initialized()) {
      DASH_LOG_TRACE("UnorderedMap.allocate",
                     "waiting for allocation of all units");
      _team->barrier();
    }
    DASH_LOG_TRACE("UnorderedMap.allocate >", "finished");
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
    DASH_LOG_TRACE_VAR("UnorderedMap.deallocate()", this);
    // Assure all units are synchronized before deallocation, otherwise
    // other units might still be working on the map:
    if (dash::is_initialized()) {
      barrier();
    }
    // Remove this function from team deallocator map to avoid
    // double-free:
    _team->unregister_deallocator(
      this, std::bind(&UnorderedMap::deallocate, this));
    // Deallocate map elements:
    DASH_LOG_TRACE_VAR("UnorderedMap.deallocate()", _globmem);
    if (_globmem != nullptr) {
      delete _globmem;
      _globmem = nullptr;
    }
    _local_cumul_sizes    = std::vector<size_type>(_team->size(), 0);
    _local_sizes.local[0] = 0;
    _remote_size          = 0;
    _begin                = iterator();
    _end                  = _begin;
    DASH_LOG_TRACE_VAR("UnorderedMap.deallocate >", this);
  }

  //////////////////////////////////////////////////////////////////////////
  // Global Iterators
  //////////////////////////////////////////////////////////////////////////

  /**
   * Global pointer to the beginning of the map.
   * After inserting and removing elements, iterators returned by \c begin()
   * and \c end() differ between units until the next call of \c commit().
   */
  inline iterator & begin() noexcept
  {
    return _begin;
  }

  /**
   * Global const pointer to the beginning of the map.
   * After inserting and removing elements, iterators returned by \c begin()
   * and \c end() differ between units until the next call of \c commit().
   */
  inline const_iterator & begin() const noexcept
  {
    return const_cast<const self_t *>(this)->begin();
  }

  /**
   * Global const pointer to the beginning of the map.
   * After inserting and removing elements, iterators returned by \c begin()
   * and \c end() differ between units until the next call of \c commit().
   */
  inline const_iterator & cbegin() const noexcept
  {
    return const_cast<const self_t *>(this)->begin();
  }

  /**
   * Global pointer to the end of the map.
   * After inserting and removing elements, iterators returned by \c begin()
   * and \c end() differ between units until the next call of \c commit().
   */
  inline iterator & end() noexcept
  {
    return _end;
  }

  /**
   * Global const pointer to the end of the map.
   * After inserting and removing elements, iterators returned by \c begin()
   * and \c end() differ between units until the next call of \c commit().
   */
  inline const_iterator & end() const noexcept
  {
    return const_cast<const self_t *>(this)->end();
  }

  /**
   * Global const pointer to the end of the map.
   * After inserting and removing elements, iterators returned by \c begin()
   * and \c end() differ between units until the next call of \c commit().
   */
  inline const_iterator & cend() const noexcept
  {
    return const_cast<const self_t *>(this)->end();
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
    return const_cast<const self_t *>(this)->lbegin();
  }

  inline const_local_iterator & clbegin() const noexcept
  {
    return const_cast<const self_t *>(this)->lbegin();
  }

  inline local_iterator & lend() noexcept
  {
    return _lend;
  }

  inline const_local_iterator & lend() const noexcept
  {
    return const_cast<const self_t *>(this)->lend();
  }

  inline const_local_iterator & clend() const noexcept
  {
    return const_cast<const self_t *>(this)->lend();
  }

  //////////////////////////////////////////////////////////////////////////
  // Capacity
  //////////////////////////////////////////////////////////////////////////

  /**
   * Maximum number of elements a map container can hold, e.g. due to
   * system limitations.
   * The maximum size is not guaranteed.
   */
  constexpr size_type max_size() const noexcept
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
    return size() == 0;
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
  // Element Access
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
  mapped_type_reference operator[](const key_type & key)
  {
    DASH_LOG_TRACE("UnorderedMap.[]()", "key:", key);
    iterator      git_value   = insert(
                                   std::make_pair(key, mapped_type()))
                                .first;
    DASH_LOG_TRACE_VAR("UnorderedMap.[]", git_value);
    dart_gptr_t   gptr_mapped = git_value.dart_gptr();
    value_type  * lptr_value  = git_value.local();
    mapped_type * lptr_mapped = nullptr;
    DASH_LOG_TRACE("UnorderedMap.[]", "gptr to element:", gptr_mapped);
    DASH_LOG_TRACE("UnorderedMap.[]", "lptr to element:", lptr_value);
    // Byte offset of mapped value in element type:
    auto          mapped_offs = offsetof(value_type, second);
    DASH_LOG_TRACE("UnorderedMap.[]", "byte offset of mapped member:",
                   mapped_offs);
    // Increment pointers to element by byte offset of mapped value member:
    if (lptr_value != nullptr) {
      // Convert to char pointer for byte-wise increment:
      char * b_lptr_mapped  = reinterpret_cast<char *>(lptr_value);
      b_lptr_mapped        += mapped_offs;
      // Convert to mapped type pointer:
      lptr_mapped           = reinterpret_cast<mapped_type *>(b_lptr_mapped);
    }
    if (!DART_GPTR_EQUAL(DART_GPTR_NULL, gptr_mapped)) {
      DASH_ASSERT_RETURNS(
        dart_gptr_incaddr(&gptr_mapped, mapped_offs),
        DART_OK);
    }
    DASH_LOG_TRACE("UnorderedMap.[]", "gptr to mapped member:", gptr_mapped);
    DASH_LOG_TRACE("UnorderedMap.[]", "lptr to mapped member:", lptr_mapped);
    // Create global reference to mapped value member in element:
    mapped_type_reference mapped(gptr_mapped,
                                 lptr_mapped);
    DASH_LOG_TRACE("UnorderedMap.[] >", mapped);
    return mapped;
  }

  /**
   * If \c key matches the key of an element in the container, returns a
   * const reference to its mapped value.
   *
   * Throws an exception if \c key does not match the key of any element in
   * the container.
   *
   * Member function \c operator[]() has the same behavior when an element
   * with the key exists, but does not throw an exception when it does not.
   *
   * \return   const reference to the mapped value of the element with a key
   *           value equivalent to \c key.
   */
  const_mapped_type_reference at(const key_type & key) const
  {
    DASH_LOG_TRACE("UnorderedMap.at() const", "key:", key);
    // TODO: Unoptimized, currently calls find(key) twice as operator[](key)
    //       calls insert(key).
    const_iterator git_value = find(key);
    if (git_value == _end) {
      // No equivalent key in map, throw:
      DASH_THROW(
        dash::exception::InvalidArgument,
        "No element in map for key " << key);
    }
    auto mapped = this->operator[](key);
    DASH_LOG_TRACE("UnorderedMap.at > const", mapped);
    return mapped;
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
  mapped_type_reference at(const key_type & key)
  {
    DASH_LOG_TRACE("UnorderedMap.at()", "key:", key);
    // TODO: Unoptimized, currently calls find(key) twice as operator[](key)
    //       calls insert(key).
    const_iterator git_value = find(key);
    if (git_value == _end) {
      // No equivalent key in map, throw:
      DASH_THROW(
        dash::exception::InvalidArgument,
        "No element in map for key " << key);
    }
    auto mapped = this->operator[](key);
    DASH_LOG_TRACE("UnorderedMap.at >", mapped);
    return mapped;
  }

  //////////////////////////////////////////////////////////////////////////
  // Element Lookup
  //////////////////////////////////////////////////////////////////////////

  /**
   * Count elements with a specific key.
   * Searches the container for elements with specified key and returns the
   * number of elements found.
   * As maps do not allow for duplicate keys, either 1 or 0 elements are
   * matched.
   *
   * \return  1 if an element with the specified key exists in the container,
   *          otherwise 0.
   */
  size_type count(const key_type & key) const
  {
    DASH_LOG_TRACE_VAR("UnorderedMap.count()", key);
    size_type nelem = 0;
    if (find(key) != _end) {
      nelem = 1;
    }
    DASH_LOG_TRACE("UnorderedMap.count >", nelem);
    return nelem;
  }

  /**
   * Get iterator to element with specified key.
   * The mapped value can also be accessed directly by using member functions
   * \c at or \c operator[].
   *
   * \return  iterator to element with specified key if found, otherwise
   *          iterator to the element past the end of the container.
   *
   * \see  count
   * \see  at
   * \see  operator[]
   */
  iterator find(const key_type & key)
  {
    DASH_LOG_TRACE_VAR("UnorderedMap.find()", key);
    iterator found = std::find_if(
                       _begin, _end,
                       [&](const value_type & v) {
                         return _key_equal(v.first, key);
                       });
    DASH_LOG_TRACE("UnorderedMap.find >", found);
    return found;
  }

  /**
   * Get const_iterator to element with specified key.
   * The mapped value can also be accessed directly by using member functions
   * \c at or \c operator[].
   *
   * \return  const_iterator to element with specified key if found,
   *          otherwise const_iterator to the element past the end of the
   *          container.
   *
   * \see  count
   * \see  at
   * \see  operator[]
   */
  const_iterator find(const key_type & key) const
  {
    DASH_LOG_TRACE_VAR("UnorderedMap.find() const", key);
    const_iterator found = std::find_if(
                             _begin, _end,
                             [&](const value_type & v) {
                               return _key_equal(v.first, key);
                             });
    DASH_LOG_TRACE("UnorderedMap.find const >", found);
    return found;
  }

  //////////////////////////////////////////////////////////////////////////
  // Modifiers
  //////////////////////////////////////////////////////////////////////////

  /**
   * Insert a new element as key-value pair, increasing the container size
   * by 1.
   *
   * Iterator validity:
   *
   * - All iterators in the container remain valid after the insertion unless
   *   it forces a rehash. In this case, all iterators in the container are
   *   invalidated.
   * - A rehash is forced if the new container size after the insertion
   *   operation would increase above its capacity threshold.
   * - References to elements in the map container remain valid in all cases,
   *   even after a rehash.
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
    auto key    = value.first;
    auto mapped = value.second;
    DASH_LOG_DEBUG("UnorderedMap.insert()", "key:", key, "mapped:", mapped);
    auto result = std::make_pair(_end, false);

    DASH_ASSERT(_globmem != nullptr);
    // Local insertion, target unit of element is active unit.
    // Look up existing element at given key:
    DASH_LOG_TRACE("UnorderedMap.insert", "element key lookup");
    DASH_LOG_TRACE_VAR("UnorderedMap.insert", _begin);
    DASH_LOG_TRACE_VAR("UnorderedMap.insert", _end);
    const_iterator found = find(key);
    DASH_LOG_TRACE_VAR("UnorderedMap.insert", found);
    if (found != _end) {
      DASH_LOG_TRACE("UnorderedMap.insert", "key found");
      // Existing element found, no insertion:
      result.first  = iterator(this, found.pos());
      result.second = false;
    } else {
      DASH_LOG_TRACE("UnorderedMap.insert", "key not found");
      // Unit mapped to the new element's key by the hash function:
      auto unit = _key_hash(key);
      DASH_LOG_TRACE("UnorderedMap.insert", "target unit:", unit);
      // No element with specified key exists, insert new value.
      // Increase local size first to reserve storage for the new element.
      // Use atomic increment to prevent hazard when other units perform
      // remote insertion at the local unit:
      size_type old_local_size   = dash::Atomic<size_type>(
                                      _local_size_gptr
                                   ).fetch_and_add(1);
      size_type new_local_size   = old_local_size + 1;
      size_type local_capacity   = _globmem->local_size();
      _local_cumul_sizes[_myid] += 1;
      DASH_LOG_TRACE_VAR("UnorderedMap.insert", local_capacity);
      DASH_LOG_TRACE_VAR("UnorderedMap.insert", _local_buffer_size);
      DASH_LOG_TRACE_VAR("UnorderedMap.insert", old_local_size);
      DASH_LOG_TRACE_VAR("UnorderedMap.insert", new_local_size);
      DASH_LOG_TRACE_VAR("UnorderedMap.insert", _local_cumul_sizes[_myid]);
      DASH_ASSERT_GT(new_local_size, 0, "new local size is 0");
      // Pointer to new element:
      value_type * lptr_insert = nullptr;
      // Acquire target pointer of new element:
      if (new_local_size > local_capacity) {
        DASH_LOG_TRACE("UnorderedMap.insert",
                       "globmem.grow(", _local_buffer_size, ")");
        lptr_insert = static_cast<value_type *>(
                        _globmem->grow(_local_buffer_size));
      } else {
        lptr_insert = static_cast<value_type *>(
                        _globmem->lbegin() + old_local_size);
      }
      // Assign new value to insert position.
      DASH_LOG_TRACE("UnorderedMap.insert", "value target address:",
                     lptr_insert);
      DASH_ASSERT(lptr_insert != nullptr);
      // Using placement new to avoid assignment/copy as value_type is
      // const:
      new (lptr_insert) value_type(value);
      // Convert local iterator to global iterator:
      DASH_LOG_TRACE("UnorderedMap.insert", "converting to global iterator",
                     "unit:", unit, "lidx:", old_local_size);
      result.first  = iterator(this, unit, old_local_size);
      result.second = true;

      if (unit != _myid) {
        DASH_LOG_TRACE("UnorderedMap.insert", "remote insertion");
        // Mark inserted element for move to remote unit in next commit:
        _move_elements.push_back(result.first);
      }

      // Update iterators as global memory space has been changed for the
      // active unit:
      auto new_size = size();
      DASH_LOG_TRACE("UnorderedMap.insert", "new size:", new_size);
      DASH_LOG_TRACE("UnorderedMap.insert", "updating _begin");
      _begin        = iterator(this, 0);
      DASH_LOG_TRACE("UnorderedMap.insert", "updating _end");
      _end          = iterator(this, new_size);
      DASH_LOG_TRACE_VAR("UnorderedMap.insert", _begin);
      DASH_LOG_TRACE_VAR("UnorderedMap.insert", _end);
    }
    DASH_LOG_DEBUG("UnorderedMap.insert >",
                   (result.second ? "inserted" : "existing"), ":",
                   result.first);
    return result;
  }

  /**
   * Insert elements in iterator range of key-value pairs, increasing the
   * container size by the number of elements in the range.
   *
   * Iterator validity:
   *
   * - All iterators in the container remain valid after the insertion unless
   *   it forces a rehash. In this case, all iterators in the container are
   *   invalidated.
   * - A rehash is forced if the new container size after the insertion
   *   operation would increase above its capacity threshold.
   * - References to elements in the map container remain valid in all cases,
   *   even after a rehash.
   */
  template<class InputIterator>
  void insert(
    // Iterator at first value in the range to insert.
    InputIterator first,
    // Iterator past the last value in the range to insert.
    InputIterator last)
  {
    // TODO: Calling insert() on every single element in the range could cause
    //       multiple calls of globmem.grow(_local_buffer_size).
    //       Could be optimized to allocate additional memory in a single call
    //       of globmem.grow(std::distance(first,last)).
    for (auto it = first; it != last; ++it) {
      insert(*it);
    }
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
    return _end;
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
    return 0;
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
    return _end;
  }

  //////////////////////////////////////////////////////////////////////////
  // Observers
  //////////////////////////////////////////////////////////////////////////

  /**
   * Reference to instance of \c DashGlobalMemoryConcept used for underlying
   * memory management of this container instance.
   */
  inline const glob_mem_type & globmem() const
  {
    return *_globmem;
  }

private:
  /// Team containing all units interacting with the map.
  dash::Team           * _team            = nullptr;
  /// DART id of the unit that created the map.
  dart_unit_t            _myid;
  /// Global memory allocation and -access.
  glob_mem_type        * _globmem         = nullptr;
  /// Iterator to initial element in the map.
  iterator               _begin           = nullptr;
  /// Iterator past the last element in the map.
  iterator               _end             = nullptr;
  /// Number of elements in the map.
  size_type              _remote_size     = 0;
  /// Native pointer to first local element in the map.
  local_iterator         _lbegin          = nullptr;
  /// Native pointer past the last local element in the map.
  local_iterator         _lend            = nullptr;
  /// Mapping units to their number of local map elements.
  local_sizes_map        _local_sizes;
  /// Cumulative (postfix sum) local sizes of all units.
  std::vector<size_type> _local_cumul_sizes;
  /// Iterators to elements in local memory space that are marked for move
  /// to remote unit in next commit.
  std::vector<iterator>  _move_elements;
  /// Global pointer to local element in _local_sizes.
  dart_gptr_t            _local_size_gptr = DART_GPTR_NULL;
  /// Hash type for mapping of key to unit and local offset.
  hasher                 _key_hash;
  /// Predicate for key comparison.
  key_equal              _key_equal;
  /// Capacity of local buffer containing locally added node elements that
  /// have not been committed to global memory yet.
  /// Default is 4 KB.
  size_type              _local_buffer_size
                           = 4096 / sizeof(value_type);

}; // class UnorderedMap

} // namespace dash

#endif // DASH__UNORDERED_MAP_H__INCLUDED
