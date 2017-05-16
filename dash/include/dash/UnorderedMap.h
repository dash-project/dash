#ifndef DASH__UNORDERED_MAP_H__INCLUDED
#define DASH__UNORDERED_MAP_H__INCLUDED

#include <dash/Types.h>
#include <dash/GlobRef.h>
#include <dash/Team.h>
#include <dash/Array.h>
#include <dash/memory/GlobHeapMem.h>
#include <dash/Allocator.h>

#include <dash/atomic/GlobAtomicRef.h>

#include <iterator>
#include <utility>
#include <limits>
#include <vector>
#include <functional>
#include <algorithm>
#include <cstddef>

#ifdef DOXYGEN

namespace dash {

/**
 * \defgroup  DashUnorderedMapConcept  Unordered Map Concept
 * Concept of a distributed unordered map container.
 *
 * \ingroup DashContainerConcept
 * \{
 * \par Description
 *
 * Different from regular maps, elements in unordered map containers are
 * not sorted in any particular order, but organized into buckets depending
 * on their hash values. Unordered maps allow for fast access to individual
 * elements as the storage location of a key in global and/or local memory
 * can be resolved directly from its hash value.
 *
 * Extends concepts:
 *
 * - \ref DashContainerConcept
 * - \ref DashSequentialContainerConcept
 * - \ref DashDynamicContainerConcept
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

/**
 * A dynamic map container with support for workload balancing.
 *
 * \concept{DashUnorderedMapConcept}
 */
template<
  typename Key,
  typename Mapped,
  typename Hash    = dash::HashLocal<Key>,
  typename Pred    = std::equal_to<Key>,
  typename Alloc   = dash::allocator::EpochSynchronizedAllocator<
                       std::pair<const Key, Mapped> > >
class UnorderedMap
{
public:
  typedef UnorderedMap<Key, Mapped, Hash, Pred, Alloc>             self_type;

  typedef Key                                                       key_type;
  typedef Mapped                                                 mapped_type;
  typedef Hash                                                        hasher;
  typedef Pred                                                     key_equal;
  typedef Alloc                                               allocator_type;

  typedef dash::default_index_t                                   index_type;
  typedef dash::default_index_t                              difference_type;
  typedef dash::default_size_t                                     size_type;
  typedef std::pair<const key_type, mapped_type>                  value_type;

  typedef typename dash::container_traits<self_type>::local_type  local_type;

  typedef dash::GlobHeapMem<value_type, allocator_type>     glob_mem_type;

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

  typedef UnorderedMapGlobIter<Key, Mapped, Hash, Pred, Alloc>
    iterator;
  typedef UnorderedMapGlobIter<Key, Mapped, Hash, Pred, Alloc>
    const_iterator;
  typedef typename std::reverse_iterator<iterator>
    reverse_iterator;
  typedef typename std::reverse_iterator<const_iterator>
    const_reverse_iterator;

  typedef UnorderedMapLocalIter<Key, Mapped, Hash, Pred, Alloc>
    local_pointer;
  typedef UnorderedMapLocalIter<Key, Mapped, Hash, Pred, Alloc>
    const_local_pointer;
  typedef UnorderedMapLocalIter<Key, Mapped, Hash, Pred, Alloc>
    local_iterator;
  typedef UnorderedMapLocalIter<Key, Mapped, Hash, Pred, Alloc>
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

  //////////////////////////////////////////////////////////////////////////
  // Distributed container
  //////////////////////////////////////////////////////////////////////////

  /**
   * The team containing all units accessing this map.
   *
   * \return  A reference to the Team containing the units associated with
   *          the container instance.
   */
  constexpr Team & team() const noexcept;

  /**
   * Reference to instance of \c DashGlobalMemoryConcept used for underlying
   * memory management of this container instance.
   */
  const glob_mem_type & globmem() const;

  //////////////////////////////////////////////////////////////////////////
  // Dynamic distributed memory
  //////////////////////////////////////////////////////////////////////////

  /**
   * Synchronize changes on local and global memory space of the map since
   * initialization or the last call of its \c barrier method with global
   * memory.
   */
  void barrier();

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
    dash::Team & team  = dash::Team::All());

  /**
   * Free global memory allocated by this container instance.
   *
   * Calls implicit barrier on the team associated with the container
   * instance.
   */
  void deallocate();

  //////////////////////////////////////////////////////////////////////////
  // Global Iterators
  //////////////////////////////////////////////////////////////////////////

  /**
   * Global iterator to the beginning of the map.
   * After inserting and removing elements, begin and end iterators may
   * differ between units until the next call of \c commit().
   */
  iterator & begin() noexcept;

  /**
   * Global const iterator to the beginning of the map.
   * After inserting and removing elements, begin and end iterators may
   * differ between units until the next call of \c commit().
   */
  const_iterator & begin() const noexcept;

  /**
   * Global const iterator to the beginning of the map.
   * After inserting and removing elements, begin and end iterators may
   * differ between units until the next call of \c commit().
   */
  const_iterator & cbegin() const noexcept;

  /**
   * Global iterator to the end of the map.
   * After inserting and removing elements, begin and end iterators may
   * differ between units until the next call of \c commit().
   */
  iterator & end() noexcept;

  /**
   * Global const iterator to the end of the map.
   * After inserting and removing elements, begin and end iterators may
   * differ between units until the next call of \c commit().
   */
  const_iterator & end() const noexcept;

  /**
   * Global const iterator to the end of the map.
   * After inserting and removing elements, begin and end iterators may
   * differ between units until the next call of \c commit().
   */
  const_iterator & cend() const noexcept;

  //////////////////////////////////////////////////////////////////////////
  // Local Iterators
  //////////////////////////////////////////////////////////////////////////

  /**
   * Local iterator to the local beginning of the map.
   * After inserting and removing elements, begin and end iterators may
   * differ between units until the next call of \c commit().
   */
  local_iterator & lbegin() noexcept;

  /**
   * Const local iterator to the local beginning of the map.
   * After inserting and removing elements, begin and end iterators may
   * differ between units until the next call of \c commit().
   */
  const_local_iterator & lbegin() const noexcept;

  /**
   * Const local iterator to the local beginning of the map.
   * After inserting and removing elements, begin and end iterators may
   * differ between units until the next call of \c commit().
   */
  const_local_iterator & clbegin() const noexcept;

  /**
   * Local iterator to the local end of the map.
   * After inserting and removing elements, begin and end iterators may
   * differ between units until the next call of \c commit().
   */
  local_iterator & lend() noexcept;

  /**
   * Local const iterator to the local end of the map.
   * After inserting and removing elements, begin and end iterators may
   * differ between units until the next call of \c commit().
   */
  const_local_iterator & lend() const noexcept;

  /**
   * Local const iterator to the local end of the map.
   * After inserting and removing elements, begin and end iterators may
   * differ between units until the next call of \c commit().
   */
  const_local_iterator & clend() const noexcept;

  //////////////////////////////////////////////////////////////////////////
  // Capacity
  //////////////////////////////////////////////////////////////////////////

  /**
   * Maximum number of elements a map container can hold, e.g. due to
   * system limitations.
   * The maximum size is not guaranteed.
   */
  constexpr size_type max_size() const noexcept;

  /**
   * The size of the map.
   *
   * \return  The number of elements in the map.
   */
  size_type size() const noexcept;

  /**
   * The number of elements that can be held in currently allocated storage
   * of the map.
   *
   * \return  The number of elements in the map.
   */
  size_type capacity() const noexcept;

  /**
   * Whether the map is empty.
   *
   * \return  true if \c size() is 0, otherwise false
   */
  bool empty() const noexcept;

  /**
   * The number of elements in the local part of the map.
   *
   * \return  The number of elements in the map that are local to the
   *          calling unit.
   */
  size_type lsize() const noexcept;

  /**
   * The capacity of the local part of the map.
   *
   * \return  The number of allocated elements in the map that are local
   *          to the calling unit.
   */
  size_type lcapacity() const noexcept;

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
  mapped_type_reference operator[](const key_type & key);

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
  const_mapped_type_reference at(const key_type & key) const;

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
  mapped_type_reference at(const key_type & key);

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
  size_type count(const key_type & key) const;

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
  iterator find(const key_type & key);

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
  const_iterator find(const key_type & key) const;

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
    const value_type & value);

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
    InputIterator last);

  /**
   * Removes and destroys single element referenced by given iterator from
   * the container, decreasing the container size by 1.
   *
   * References and iterators to the erased elements are invalidated.
   * Other iterators and references are not invalidated.
   *
   * \return  iterator to the element that follows the last element removed,
   *          or \c end() if the last element was removed.
   */
  iterator erase(
    const_iterator position);

  /**
   * Removes and destroys elements referenced by the given key from the
   * container, decreasing the container size by the number of elements
   * removed.
   *
   * References and iterators to the erased elements are invalidated.
   * Other iterators and references are not invalidated.
   *
   * \return  The number of elements removed.
   */
  size_type erase(
    /// Key of the container element to remove.
    const key_type & key);

  /**
   * Removes and destroys elements in the given range from the container,
   * decreasing the container size by the number of elements removed.
   *
   * References and iterators to the erased elements are invalidated.
   * Other iterators and references are not invalidated.
   *
   * \return  iterator to the element that follows the last element removed,
   *          or \c end() if the last element was removed.
   */
  iterator erase(
    /// Iterator at first element to remove.
    const_iterator first,
    /// Iterator past the last element to remove.
    const_iterator last);

  //////////////////////////////////////////////////////////////////////////
  // Bucket Interface
  //////////////////////////////////////////////////////////////////////////

  /**
   * Returns the index of the bucket for a specified key.
   * Elements with keys equivalent to the specified key are always found in
   * this bucket.
   *
   * \return  Bucket index for the specified key
   */
  size_type bucket(
    /// The value of the key to examine
    const key_type & key) const;

  /**
   * The number of elements in a specified bucket.
   *
   * \return  The number of elements in the bucket with the specified index
   */
  size_type bucket_size(
    /// The index of the bucket to examine
    size_type bucket_index) const;

  //////////////////////////////////////////////////////////////////////////
  // Observers
  //////////////////////////////////////////////////////////////////////////

  /**
   * Returns the function that compares keys for equality.
   */
  key_equal key_eq() const;

  /**
   * Returns the function that hashes the keys.
   */
  hasher hash_function() const;

}; // class UnorderedMap

} // namespace dash

#endif // ifdef DOXYGEN

#include <dash/map/UnorderedMap.h>

#endif // DASH__UNORDERED_MAP_H__INCLUDED
