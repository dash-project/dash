#ifndef DASH__GLOB_DYNAMIC_MEM_H_
#define DASH__GLOB_DYNAMIC_MEM_H_

#include <dash/dart/if/dart.h>

#include <dash/Types.h>
#include <dash/GlobPtr.h>
#include <dash/GlobSharedRef.h>
#include <dash/Allocator.h>
#include <dash/Team.h>
#include <dash/Onesided.h>
#include <dash/Array.h>

#include <dash/algorithm/MinMax.h>
#include <dash/algorithm/Copy.h>

#include <dash/allocator/LocalBucketIter.h>
#include <dash/allocator/GlobBucketIter.h>
#include <dash/allocator/internal/GlobDynamicMemTypes.h>

#include <dash/internal/Logging.h>

#include <list>
#include <vector>
#include <iterator>
#include <sstream>
#include <iostream>


namespace dash {

/**
 * \defgroup  DashGlobalDynamicMemoryConcept  Global Dynamic Memory Concept
 * Concept of distributed dynamic global memory space shared by units in a
 * specified team.
 *
 * \ingroup DashGlobalMemoryConcept
 * \{
 * \par Description
 *
 * Extends the Global Memory concept by dynamic address spaces.
 *
 * Units can change the capacity of the global memory space by resizing
 * their own local segment of the global memory space.
 * Resizing local memory segments (methods \c resize, \c grow and \c shrink)
 * is non-collective, however the resulting changes to local and global
 * memory space are only immediately visible to the unit that executed the
 * resize operation.
 *
 * The collective operation \c commit synchronizes changes on local memory
 * spaces between all units such that new allocated memory segments are
 * attached in global memory and deallocated segments detached, respectively.
 *
 * Newly allocated memory segments are unattached and immediately accessible
 * by the local unit only.
 * Deallocated memory is immediately removed from the local unit's memory
 * space but remains accessible for remote units.
 *
 * Different from typical dynamic container semantics, neither resizing the
 * memory space nor commit operations invalidate iterators to elements in
 * allocated global memory.
 * An iterators referencing a remote element in global dynamic memory is only
 * invalidated in the \c commit operation following the deallocation of the
 * element's memory segment.
 *
 * \par Types
 *
 * Type Name            | Description                                            |
 * -------------------- | ------------------------------------------------------ |
 * \c GlobalRAI         | Random access iterator on global address space         |
 * \c LocalRAI          | Random access iterator on a single local address space |
 *
 *
 * \par Methods
 *
 * Return Type          | Method             | Parameters                  | Description                                                                                                |
 * -------------------- | ------------------ | --------------------------- | ---------------------------------------------------------------------------------------------------------- |
 * <tt>void</tt>        | <tt>resize</tt>    | <tt>size lsize_new</tt>     | Resize the local segment of the global memory space to the specified number of values.                     |
 * <tt>void</tt>        | <tt>grow</tt>      | <tt>size lsize_diff</tt>    | Extend the size of the local segment of the global memory space by the specified number of values.         |
 * <tt>void</tt>        | <tt>shrink</tt>    | <tt>size lsize_diff</tt>    | Reduce the size of the local segment of the global memory space by the specified number of values.         |
 * <tt>void</tt>        | <tt>commit</tt>    | nbsp;                       | Publish changes to local memory across all units.                                                          |
 *
 *
 * \par Methods inherited from Global Memory concept
 *
 * Return Type          | Method             | Parameters                         | Description                                                                                                |
 * -------------------- | ------------------ | ---------------------------------- | ---------------------------------------------------------------------------------------------------------- |
 * <tt>GlobalRAI</tt>   | <tt>begin</tt>     | &nbsp;                             | Global pointer to the initial address of the global memory space                                           |
 * <tt>GlobalRAI</tt>   | <tt>end</tt>       | &nbsp;                             | Global pointer past the final element in the global memory space                                           |
 * <tt>LocalRAI</tt>    | <tt>lbegin</tt>    | &nbsp;                             | Local pointer to the initial address in the local segment of the global memory space                       |
 * <tt>LocalRAI</tt>    | <tt>lbegin</tt>    | <tt>unit u</tt>                    | Local pointer to the initial address in the local segment at unit \c u of the global memory space          |
 * <tt>LocalRAI</tt>    | <tt>lend</tt>      | &nbsp;                             | Local pointer past the final element in the local segment of the global memory space                       |
 * <tt>LocalRAI</tt>    | <tt>lend</tt>      | <tt>unit u</tt>                    | Local pointer past the final element in the local segment at unit \c u of the global memory space          |
 * <tt>GlobalRAI</tt>   | <tt>at</tt>        | <tt>index gidx</tt>                | Global pointer to the element at canonical global offset \c gidx in the global memory space                |
 * <tt>void</tt>        | <tt>put_value</tt> | <tt>value & v_in, index gidx</tt>  | Stores value specified in parameter \c v_in to address in global memory at canonical global offset \c gidx |
 * <tt>void</tt>        | <tt>get_value</tt> | <tt>value * v_out, index gidx</tt> | Loads value from address in global memory at canonical global offset \c gidx into local address \c v_out   |
 * <tt>void</tt>        | <tt>barrier</tt>   | &nbsp;                             | Blocking synchronization of all units associated with the global memory instance                           |
 *
 * \}
 */


/**
 * Global memory region with dynamic size.
 *
 * Conventional global memory (see \c dash::GlobMem) allocates a single
 * contiguous range of fixed size in local memory at every unit.
 * Iterating static memory space is trivial as native pointer arithmetics
 * can be used to traverse elements in canonical storage order.
 *
 * In global dynamic memory, units allocate multiple heap-allocated buckets
 * in local memory.
 * The number of local buckets and their sizes may differ between units.
 * In effect, elements in local memory are distributed in non-contiguous
 * address ranges and a custom iterator is used to access elements in
 * logical storage order.
 *
 * Usage examples:
 *
 * \code
 *   size_t initial_local_capacity  = 1024;
 *   GlobDynamicMem<double> gdmem(initial_local_capacity);
 *
 *   size_t initial_global_capacity = dash::size() * initial_local_capacity;
 *
 *   if (dash::myid() == 0) {
 *     // Allocate another 512 elements in local memory space.
 *     // This is a local operation, the additionally allocated memory
 *     // space is only accessible by the local unit, however:
 *     gdmem.grow(512);
 *   }
 *   if (dash::myid() == 1) {
 *     // Decrease capacity of local memory space by 128 units.
 *     // This is a local operation. New size of logical memory space is
 *     // effective for the local unit immediately but memory is not
 *     // physically freed yet and is still accessible by other units.
 *     gdmem.shrink(128);
 *   }
 *
 *   // Global memory space has not been updated yet, changes are only
 *   // visible locally:
 *   if (dash::myid() == 0) {
 *     assert(gdmem.size() == initial_global_capacity + 512);
 *   } else if (dash::myid() == 1) {
 *     assert(gdmem.size() == initial_global_capacity - 128);
 *   } else {
 *     assert(gdmem.size() == initial_global_capacity);
 *   }
 *
 *   auto unit_0_local_size = gdmem.lend(0) - gdmem.lbegin(0);
 *   auto unit_1_local_size = gdmem.lend(1) - gdmem.lbegin(1);
 *
 *   if (dash::myid() == 0) {
 *     assert(unit_0_local_size == 1024 + 512);
 *     assert(unit_0_local_size == gdmem.local_size());
 *   } else {
 *     assert(unit_0_local_size == initial_local_capacity);
 *   }
 *   if (dash::myid() == 1) {
 *     assert(unit_1_local_size == 1024 - 128);
 *     assert(unit_1_local_size == gdmem.local_size());
 *   } else {
 *     assert(unit_1_local_size == initial_local_capacity);
 *   }
 *
 *   // Memory marked for deallocation is still accessible by other units:
 *   if (dash::myid() != 1) {
 *     auto unit_1_last = gdmem.at(dash::myid(), initial_local_capacity-1);
 *     double * value;
 *     gdmem.get_value(value, unit_1_last);
 *   }
 *
 *   // Collectively commit changes of local memory allocation to global
 *   // memory space:
 *   // register newly allocated local memory and remove local memory marked
 *   // for deallocation.
 *   gdmem.commit();
 *
 *   // Changes are globally visible now:
 *   assert(gdmem.size() == initial_global_capacity + 512 - 128);
 *
 *   unit_0_local_size = gdmem.lend(0) - gdmem.lbegin(0);
 *   unit_1_local_size = gdmem.lend(1) - gdmem.lbegin(1);
 *   assert(unit_0_local_size == 1024 + 512);
 *   assert(unit_1_local_size == 1024 - 128);
 * \endcode
 *
 * \concept{DashGlobalDynamicMemoryConcept}
 */
template<
  /// Type of values allocated in the global memory space
  typename ElementType,
  /// Type of allocator implementation used to allocate and deallocate
  /// global memory
  class    AllocatorType =
             dash::allocator::DynamicAllocator<ElementType> >
class GlobDynamicMem
{
private:
  typedef GlobDynamicMem<ElementType, AllocatorType>
    self_t;

public:
  typedef AllocatorType                                      allocator_type;
  typedef ElementType                                            value_type;
  typedef typename AllocatorType::size_type                       size_type;
  typedef typename AllocatorType::difference_type           difference_type;
  typedef typename AllocatorType::difference_type                index_type;
  typedef typename AllocatorType::pointer                       raw_pointer;
  typedef typename AllocatorType::void_pointer                 void_pointer;
  typedef typename AllocatorType::const_void_pointer     const_void_pointer;
  typedef GlobPtr<ElementType>                                      pointer;
  typedef GlobPtr<const ElementType>                          const_pointer;
  typedef GlobSharedRef<ElementType>                              reference;
  typedef GlobSharedRef<const ElementType>                  const_reference;

  typedef ElementType &                                     local_reference;
  typedef const ElementType &                         const_local_reference;

  typedef LocalBucketIter<value_type, index_type>
    local_iterator;
  typedef LocalBucketIter<value_type, index_type>
    const_local_iterator;

  typedef GlobBucketIter<
            value_type, self_t, pointer, reference>
    global_iterator;
  typedef GlobBucketIter<
            const value_type, const self_t, const_pointer, const_reference>
    const_global_iterator;

  typedef std::reverse_iterator<      global_iterator>
    reverse_global_iterator;
  typedef std::reverse_iterator<const_global_iterator>
    const_reverse_global_iterator;

  typedef std::reverse_iterator<      local_iterator>
    reverse_local_iterator;
  typedef std::reverse_iterator<const_local_iterator>
    const_reverse_local_iterator;

  typedef       local_iterator                                local_pointer;
  typedef const_local_iterator                          const_local_pointer;

  typedef typename local_iterator::bucket_type                  bucket_type;

private:
  typedef typename std::list<bucket_type>
    bucket_list;
  typedef typename bucket_list::iterator
    bucket_iterator;

  typedef dash::Array<
            size_type, int, dash::CSRPattern<1, dash::ROW_MAJOR, int> >
    local_sizes_map;

  typedef std::vector<std::vector<size_type> >
    bucket_cumul_sizes_map;

  template<typename T_, class GMem_, class Ptr_, class Ref_>
  friend class dash::GlobBucketIter;

public:
  /**
   * Constructor, collectively allocates the given number of elements in
   * local memory of every unit in a team.
   *
   * \concept{DashDynamicMemorySpaceConcept}
   * \concept{DashMemorySpaceConcept}
   */
  GlobDynamicMem(
    /// Initial number of local elements to allocate in global memory space
    size_type   n_local_elem = 0,
    /// Team containing all units operating on the global memory region
    Team      & team         = dash::Team::All())
  : _allocator(team),
    _team(&team),
    _teamid(team.dart_id()),
    _nunits(team.size()),
    _myid(team.myid()),
    _attach_buckets_first(_buckets.end()),
    _local_sizes(team.size()),
    _bucket_cumul_sizes(team.size()),
    _num_attach_buckets(team.size()),
    _num_detach_buckets(team.size()),
    _remote_size(0)
  {
    DASH_LOG_TRACE("GlobDynamicMem.(ninit,nunits)",
                   n_local_elem, team.size());

    _local_sizes.local[0]        = 0;
    _num_attach_buckets.local[0] = 0;
    _num_detach_buckets.local[0] = 0;

    DASH_LOG_TRACE("GlobDynamicMem.GlobDynamicMem",
                   "allocating initial memory space");
    grow(n_local_elem);
    commit();

    DASH_LOG_TRACE("GlobDynamicMem.GlobDynamicMem >");
  }

  /**
   * Destructor, collectively frees underlying global memory.
   */
  ~GlobDynamicMem()
  {
    DASH_LOG_TRACE("GlobDynamicMem.~GlobDynamicMem()");
    DASH_LOG_TRACE("GlobDynamicMem.~GlobDynamicMem >");
  }

  GlobDynamicMem() = delete;

  /**
   * Copy constructor.
   */
  GlobDynamicMem(const self_t & other)
    = default;

  /**
   * Assignment operator.
   */
  self_t & operator=(const self_t & rhs)
    = default;

  /**
   * Equality comparison operator.
   */
  bool operator==(const self_t & rhs) const noexcept
  {
    return (_teamid         == rhs._teamid &&
            _nunits         == rhs._nunits &&
            _lbegin         == rhs._lbegin &&
            _lend           == rhs._lend &&
            _buckets        == rhs._buckets &&
            _detach_buckets == rhs._detach_buckets);
  }

  /**
   * Total number of elements in attached memory space, including size of
   * local unattached memory segments.
   */
  inline size_type size() const
  {
    auto global_size = _remote_size + local_size();
    return global_size;
  }

  /**
   * Number of elements in local memory space.
   */
  inline size_type local_size() const noexcept
  {
    return _local_sizes.local[0];
  }

  /**
   * Number of elements in local memory space of given unit.
   *
   * \return  Local capacity as published by the specified unit in last
   *          commit.
   */
  inline size_type local_size(team_unit_t unit) const
  {
    DASH_LOG_TRACE("GlobDynamicMem.local_size(u)", "unit:", unit);
    DASH_ASSERT_RANGE(0, unit, _nunits-1, "unit id out of range");
    DASH_LOG_TRACE_VAR("GlobDynamicMem.local_size",
                       _bucket_cumul_sizes[unit]);
    size_type unit_local_size;
    if (unit == _myid) {
      // Value of _local_sizes[u] is the local size as visible by the unit,
      // i.e. including size of unattached buckets.
      unit_local_size = _local_sizes.local[0];
    } else {
      unit_local_size = _bucket_cumul_sizes[unit].back();
    }
    DASH_LOG_TRACE("GlobDynamicMem.local_size >", unit_local_size);
    return unit_local_size;
  }

  /**
   * Inequality comparison operator.
   */
  bool operator!=(const self_t & rhs) const noexcept
  {
    return !(*this == rhs);
  }

  /**
   * The team containing all units accessing the global memory space.
   *
   * \return  A reference to the Team containing the units associated with
   *          the global dynamic memory space.
   */
  inline dash::Team & team() const noexcept
  {
    if (_team != nullptr) {
      return *_team;
    }
    return dash::Team::Null();
  }

  /**
   * Increase capacity of local segment of global memory region by the given
   * number of elements.
   * Same as \c resize(size() + num_elements).
   *
   * Local operation.
   * Newly allocated memory is attached to global memory space by calling
   * the collective operation \c attach().
   *
   * \return  Native pointer to beginning of new allocated memory.
   *
   * \see resize
   * \see shrink
   * \see commit
   */
  local_iterator grow(size_type num_elements)
  {
    DASH_LOG_DEBUG_VAR("GlobDynamicMem.grow()", num_elements);
    size_type local_size_old = _local_sizes.local[0];
    DASH_LOG_TRACE("GlobDynamicMem.grow",
                   "current local size:", local_size_old);
    if (num_elements == 0) {
      DASH_LOG_DEBUG("GlobDynamicMem.grow >", "no grow");
      return _lend;
    }
    // Update size of local memory space:
    _local_sizes.local[0]        += num_elements;
    // Update number of local buckets marked for attach:
    _num_attach_buckets.local[0] += 1;

    // Create new unattached bucket:
    DASH_LOG_TRACE("GlobDynamicMem.grow", "creating new unattached bucket:",
                   "size:", num_elements);
    bucket_type bucket;
    bucket.size     = num_elements;
    bucket.lptr     = _allocator.allocate_local(bucket.size);
    bucket.gptr     = DART_GPTR_NULL;
    bucket.attached = false;
    // Add bucket to local memory space:
    _buckets.push_back(bucket);
    if (_attach_buckets_first == _buckets.end()) {
      // Move iterator to first unattached bucket to position of new bucket:
      _attach_buckets_first = _buckets.begin();
      std::advance(_attach_buckets_first,  _buckets.size() - 1);
    }
    _bucket_cumul_sizes[_myid].push_back(_local_sizes.local[0]);
    DASH_LOG_TRACE("GlobDynamicMem.grow", "added unattached bucket:",
                   "size:", bucket.size,
                   "lptr:", bucket.lptr);
    // Update local iteration space:
    _lbegin = lbegin(_myid);
    _lend   = lend(_myid);
    DASH_ASSERT_EQ(_local_sizes.local[0], _lend - _lbegin,
                   "local size differs from local iteration space size");
    DASH_LOG_TRACE("GlobDynamicMem.grow",
                   "new local size:",     _local_sizes.local[0]);
    DASH_LOG_TRACE("GlobDynamicMem.grow",
                   "local buckets:",      _buckets.size(),
                   "unattached buckets:", _num_attach_buckets.local[0]);
    DASH_LOG_TRACE("GlobDynamicMem.grow >");
    // Return local iterator to start of allocated memory:
    return _lbegin + local_size_old;
  }

  /**
   * Decrease capacity of local segment of global memory region by the given
   * number of elements.
   * Buckets are not deallocated until next commit as other units might
   * still reference them.
   * Same as \c resize(size() - num_elements).
   *
   * Local operation.
   *
   * Resizes logical local memory space but does not deallocate memory.
   * Local memory is accessible by other units until deallocated and
   * detached from global memory space by calling the collective operation
   * \c commit().
   *
   * \see resize
   * \see grow
   * \see commit
   */
  void shrink(size_type num_elements)
  {
    // This function updates the size of the local memory space of the
    // calling unit u.
    // The following members are updated:
    //
    // _local_sizes[u]:
    //   Size of local memory space as visible to unit u.
    //
    // _bucket_cumul_sizes:
    //    An array mapping units to a list of their cumulative bucket sizes
    //    (i.e. postfix sum) which is required to iterate over the
    //    non-contigous global dynamic memory space.
    //    For example, if unit 2 allocated buckets with sizes 1, 3 and 5,
    //    _bucket_cumul_sizes[2] is a list { 1, 4, 9 }.
    //
    // _buckets:
    //    List of local buckets that provide the underlying storage of the
    //    active unit's local memory space.
    //
    // Notes:
    //
    // It must be ensured that the updated cumulative bucket sizes of a
    // remote unit can be resolved in \c update_local_size() after any
    // possible combination of grow- and shrink-operations at the remote unit
    // from the following information:
    //
    // - the cumulative bucket sizes of the remote unit at the time of the
    //   last commit
    // - the remote unit's local size at the time of the last commit
    // - the remote unit's current local size (including unattached buckets)
    // - the number of the remote unit's unattached buckets and their size

    DASH_LOG_DEBUG_VAR("GlobDynamicMem.shrink()", num_elements);
    DASH_ASSERT_LT(num_elements, local_size() + 1,
                   "cannot shrink size " << local_size() << " "
                   "by " << num_elements << " elements");
    if (num_elements == 0) {
      DASH_LOG_DEBUG("GlobDynamicMem.shrink >", "no shrink");
      return;
    }
    DASH_LOG_TRACE("GlobDynamicMem.shrink",
                   "current local size:", _local_sizes.local[0]);
    DASH_LOG_TRACE("GlobDynamicMem.shrink",
                   "current local buckets:", _buckets.size());
    // Position of iterator to first unattached bucket:
    auto attach_buckets_first_pos = std::distance(_buckets.begin(),
                                                  _attach_buckets_first);
    DASH_LOG_TRACE_VAR("GlobDynamicMem.shrink", attach_buckets_first_pos);
    // Number of elements left to deallocate:
    auto num_dealloc = num_elements;
    // Try to reduce local capacity by deallocating un-attached local buckets
    // as they do not have to be detached collectively.
    // Unattached buckets can be removed from memory space immediately as
    // remote units cannot have a pending reference on them.
    while (!_buckets.back().attached && num_dealloc > 0) {
      bucket_type & bucket_last = _buckets.back();
      // Shrink / remove unattached buckets starting at newest bucket:
      if (bucket_last.size <= num_dealloc) {
        DASH_LOG_TRACE("GlobDynamicMem.shrink", "remove unattached bucket:",
                       "size:", bucket_last.size);
        // Mark entire bucket for deallocation below:
        num_dealloc           -= bucket_last.size;
        _local_sizes.local[0] -= bucket_last.size;
        _bucket_cumul_sizes[_myid].pop_back();
        // End iterator of _buckets about to change, update iterator to first
        // unattached bucket if it references the removed bucket:
        auto attach_buckets_first_it = _attach_buckets_first;
        if (attach_buckets_first_it   != _buckets.end() &&
            ++attach_buckets_first_it == _buckets.end()) {
          // Iterator to first unattached bucket references last bucket:
          DASH_LOG_TRACE("GlobDynamicMem.shrink",
                         "updating iterator to first unattached bucket");
          _attach_buckets_first--;
        }
        _buckets.pop_back();
        if (_attach_buckets_first->attached) {
          // Updated iterator to first unattached bucket references attached
          // bucket:
          _attach_buckets_first = _buckets.end();
        }
        // Update number of local buckets marked for attach:
        DASH_ASSERT_GT(_num_attach_buckets.local[0], 0,
                       "Last bucket unattached but number of buckets marked "
                       "for attach is 0");
        _num_attach_buckets.local[0] -= 1;
      } else if (bucket_last.size > num_dealloc) {
        // TODO: Clarify if shrinking unattached buckets is allowed
        DASH_LOG_TRACE("GlobDynamicMem.shrink", "shrink unattached bucket:",
                       "old size:", bucket_last.size,
                       "new size:", bucket_last.size - num_dealloc);
        bucket_last.size                  -= num_dealloc;
        _local_sizes.local[0]             -= num_dealloc;
        _bucket_cumul_sizes[_myid].back() -= num_dealloc;
        num_dealloc = 0;
      }
    }
    // Number of elements to deallocate exceeds capacity of un-attached
    // buckets, deallocate attached buckets:
    auto num_dealloc_gbuckets = 0;
    // Shrink attached buckets starting at newest bucket:
    for (auto bucket_it = _buckets.rbegin();
         bucket_it != _buckets.rend();
         ++bucket_it) {
      if (!bucket_it->attached) {
        continue;
      }
      if (num_dealloc == 0) {
        break;
      }
      if (bucket_it->size <= num_dealloc) {
        // mark entire bucket for deallocation:
        num_dealloc_gbuckets++;
        _num_detach_buckets.local[0]      += 1;
        _local_sizes.local[0]             -= bucket_it->size;
        _bucket_cumul_sizes[_myid].back() -= bucket_it->size;
        num_dealloc                       -= bucket_it->size;
      } else if (bucket_it->size > num_dealloc) {
        DASH_LOG_TRACE("GlobDynamicMem.shrink", "shrink attached bucket:",
                       "old size:", bucket_it->size,
                       "new size:", bucket_it->size - num_dealloc);
        bucket_it->size                   -= num_dealloc;
        _local_sizes.local[0]             -= num_dealloc;
        _bucket_cumul_sizes[_myid].back() -= num_dealloc;
        num_dealloc = 0;
      }
    }
    // Mark attached buckets for deallocation.
    // Requires separate loop as iterators on _buckets could be invalidated.
    DASH_LOG_DEBUG_VAR("GlobDynamicMem.shrink", num_dealloc_gbuckets);
    while (num_dealloc_gbuckets-- > 0) {
      auto dealloc_bucket = _buckets.back();
      DASH_LOG_TRACE("GlobDynamicMem.shrink", "deallocate attached bucket:"
                     "size:", dealloc_bucket.size,
                     "lptr:", dealloc_bucket.lptr);
      // Mark bucket to be detached in next call of commit():
      _detach_buckets.push_back(dealloc_bucket);
      // Unregister bucket:
      _buckets.pop_back();
    }
    // Update local iterators as bucket iterators might have changed:
    _lbegin = lbegin(_myid);
    _lend   = lend(_myid);

    DASH_LOG_TRACE("GlobDynamicMem.shrink",
                   "cumulative bucket sizes:",  _bucket_cumul_sizes[_myid]);
    DASH_LOG_TRACE("GlobDynamicMem.shrink",
                   "new local size:",           _local_sizes.local[0],
                   "new iteration space size:", std::distance(
                                                  _lbegin, _lend));
    DASH_LOG_TRACE("GlobDynamicMem.shrink",
                   "total number of buckets:",  _buckets.size(),
                   "unattached buckets:",       std::distance(
                                                  _attach_buckets_first,
                                                  _buckets.end()));
    DASH_LOG_DEBUG("GlobDynamicMem.shrink >");
  }

  /**
   * Commit changes of local memory region to global memory space.
   * Applies calls of \c grow(), \c shrink() and \c resize() to global
   * memory.
   *
   * Collective operation.
   *
   * Attaches local memory allocated since the last call of \c commit() to
   * global memory space and thus makes it accessible to other units.
   * Frees local memory marked for deallocation and detaches it from global
   * memory.
   *
   * \see resize
   * \see grow
   * \see shrink
   */
  void commit()
  {
    DASH_LOG_DEBUG("GlobDynamicMem.commit()");
    DASH_LOG_TRACE_VAR("GlobDynamicMem.commit", _buckets.size());

    // First detach, then attach to minimize number of elements allocated
    // at the same time:
    size_type num_detached_elem = commit_detach();
    size_type num_attached_elem = commit_attach();

    if (num_detached_elem > 0 || num_attached_elem > 0) {
      // Update _begin iterator:
      DASH_LOG_TRACE("GlobDynamicMem.commit", "updating _begin");
      _begin = global_iterator(this, 0);
      DASH_LOG_TRACE("GlobDynamicMem.commit", "updating _end");
      _end   = _begin + size();
    }
    // Update local iterators as bucket iterators might have changed:
    DASH_LOG_TRACE("GlobDynamicMem.commit", "updating _lbegin");
    _lbegin = lbegin(_myid);
    DASH_LOG_TRACE("GlobDynamicMem.commit", "updating _lend");
    _lend   = lend(_myid);
    DASH_LOG_DEBUG("GlobDynamicMem.commit >", "finished");
  }

  /**
   * Resize capacity of local segment of global memory region to the given
   * number of elements.
   *
   * Local operation.
   *
   * If capacity is increased, newly allocated memory is only attached to
   * global memory space and thus made accessible to other units by calling
   * the collective operation \c commit().
   * If capacity is decreased, resizes logical local memory space but does
   * not deallocate memory.
   * Local memory is accessible by other units until deallocated and
   * detached from global memory space by calling the collective operation
   * \c commit().
   *
   * \see grow
   * \see shrink
   * \see commit
   */
  void resize(size_type num_elements)
  {
    DASH_LOG_DEBUG("GlobDynamicMem.resize()", "new size:", num_elements);
    index_type diff_capacity = num_elements - size();
    if (diff_capacity > 0) {
      grow(diff_capacity);
    } else if (diff_capacity < 0) {
      shrink(-diff_capacity);
    }
    DASH_LOG_DEBUG("GlobDynamicMem.resize >");
  }

  /**
   * Global pointer of the initial address of the global memory.
   */
  global_iterator & begin()
  {
    return _begin;
  }

  /**
   * Global pointer of the initial address of the global memory.
   */
  const_global_iterator & begin() const
  {
    return _begin;
  }

  /**
   * Global pointer of the initial address of the global memory.
   */
  reverse_global_iterator rbegin()
  {
    return reverse_global_iterator(_end);
  }

  /**
   * Global pointer of the initial address of the global memory.
   */
  const_reverse_global_iterator rbegin() const
  {
    return reverse_global_iterator(_end);
  }

  /**
   * Global pointer of the initial address of the global memory.
   */
  global_iterator & end()
  {
    return _end;
  }

  /**
   * Global pointer of the initial address of the global memory.
   */
  const_global_iterator & end() const
  {
    return _end;
  }

  /**
   * Global pointer of the initial address of the global memory.
   */
  reverse_global_iterator rend()
  {
    return reverse_global_iterator(_begin);
  }

  /**
   * Global pointer of the initial address of the global memory.
   */
  const_reverse_global_iterator rend() const
  {
    return reverse_global_iterator(_begin);
  }

  /**
   * Native pointer of the initial address of the local memory of
   * a unit.
   *
   * TODO: Should be private and renamed to update_lbegin() as returning a
   *       local iterator from lbegin(u) is not possible if u is remote.
   */
  local_iterator lbegin(
    team_unit_t unit_id)
  {
    DASH_LOG_TRACE_VAR("GlobDynamicMem.lbegin()", unit_id);
    if (unit_id == _myid) {
      local_iterator unit_lbegin(
               // iteration space
               _buckets.begin(), _buckets.end(),
               // position in iteration space
               0,
               // bucket at position in iteration space,
               // offset in bucket
               _buckets.begin(), 0);
      DASH_LOG_TRACE("GlobDynamicMem.lbegin >", unit_lbegin);
      return unit_lbegin;
    } else {
      DASH_THROW(dash::exception::NotImplemented,
                 "dash::GlobDynamicMem.lbegin(unit) is not implemented "
                 "for unit != dash::myid()");
    }
  }

  /**
   * Native pointer of the initial address of the local memory of
   * a unit.
   *
   * TODO: Should be removed once non-const lbegin(u) is refactored.
   */
  const_local_iterator lbegin(
    team_unit_t unit_id) const
  {
    DASH_LOG_TRACE_VAR("GlobDynamicMem.lbegin const()", unit_id);
    if (unit_id == _myid) {
      local_iterator unit_clbegin(
               // iteration space
               _buckets.begin(), _buckets.end(),
               // position in iteration space
               0,
               // bucket at position in iteration space,
               // offset in bucket
               _buckets.begin(), 0);
      DASH_LOG_TRACE("GlobDynamicMem.lbegin const >", unit_clbegin);
      return unit_clbegin;
    } else {
      DASH_THROW(dash::exception::NotImplemented,
                 "dash::GlobDynamicMem.lbegin(unit) const " <<
                 "is not implemented for unit != dash::myid()");
    }
  }

  /**
   * Native pointer of the initial address of the local memory of
   * the unit that initialized this GlobDynamicMem instance.
   */
  inline local_iterator & lbegin()
  {
    return _lbegin;
  }

  /**
   * Native pointer of the initial address of the local memory of
   * the unit that initialized this GlobDynamicMem instance.
   */
  inline const_local_iterator lbegin() const
  {
    return _lbegin;
  }

  /**
   * Native pointer of the final address of the local memory of
   * a unit.
   */
  local_iterator lend(
    team_unit_t unit_id)
  {
    DASH_LOG_TRACE_VAR("GlobDynamicMem.lend()", unit_id);
    if (unit_id == _myid) {
      local_iterator unit_lend(
               // iteration space
               _buckets.begin(), _buckets.end(),
               // position in iteration space
               local_size(),
               // bucket at position in iteration space,
               // offset in bucket
               _buckets.end(), 0);
      DASH_LOG_TRACE("GlobDynamicMem.lend >", unit_lend);
      return unit_lend;
    } else {
      DASH_THROW(dash::exception::NotImplemented,
                 "dash::GlobDynamicMem.lend(unit) is not implemented "
                 "for unit != dash::myid()");
    }
  }

  /**
   * Native pointer of the final address of the local memory of
   * a unit.
   */
  const_local_iterator lend(
    team_unit_t unit_id) const
  {
    DASH_LOG_TRACE_VAR("GlobDynamicMem.lend() const", unit_id);
    if (unit_id == _myid) {
      local_iterator unit_clend(
               // iteration space
               _buckets.cbegin(), _buckets.cend(),
               // position in iteration space
               local_size(),
               // bucket at position in iteration space,
               // offset in bucket
               _buckets.cend(), 0);
      DASH_LOG_TRACE("GlobDynamicMem.lend const >", unit_clend);
      return unit_clend;
    } else {
      DASH_THROW(dash::exception::NotImplemented,
                 "dash::GlobDynamicMem.lend(unit) const is not implemented "
                 "for unit != dash::myid()");
    }
  }

  /**
   * Native pointer of the initial address of the local memory of
   * the unit that initialized this GlobDynamicMem instance.
   */
  inline local_iterator & lend()
  {
    return _lend;
  }

  /**
   * Native pointer of the initial address of the local memory of
   * the unit that initialized this GlobDynamicMem instance.
   */
  inline const_local_iterator & lend() const
  {
    return _lend;
  }

  /**
   * Write value to global memory at given offset.
   *
   * \see  dash::put_value
   */
  template<typename ValueType = ElementType>
  void put_value(
    const ValueType & newval,
    index_type        global_index)
  {
    DASH_LOG_TRACE("GlobDynamicMem.put_value(newval, gidx = %d)",
                   global_index);
    auto git = const_global_iterator(this, global_index);
    dash::put_value(newval, git);
  }

  /**
   * Read value from global memory at given offset.
   *
   * \see  dash::get_value
   */
  template<typename ValueType = ElementType>
  void get_value(
    ValueType  * ptr,
    index_type   global_index) const
  {
    DASH_LOG_TRACE("GlobDynamicMem.get_value(newval, gidx = %d)",
                   global_index);
    auto git = const_global_iterator(this, global_index);
    dash::get_value(ptr, git);
  }

  /**
   * Synchronize all units associated with this global memory instance.
   * Does not commit changes of local memory space.
   *
   * \see  commit
   */
  void barrier() const
  {
    DASH_ASSERT_RETURNS(
      dart_barrier(_teamid),
      DART_OK);
  }

  /**
   * Resolve the global iterator referencing an element position in a unit's
   * local memory.
   */
  template<typename IndexT>
  global_iterator at(
    /// The unit id
    team_unit_t unit,
    /// The unit's local address offset
    IndexT      local_index)
  {
    DASH_LOG_DEBUG("GlobDynamicMem.at()",
                   "unit:", unit, "lidx:", local_index);
    if (_nunits == 0) {
      DASH_THROW(dash::exception::RuntimeError, "No units in team");
    }
    global_iterator git(this, unit, local_index);
    DASH_LOG_DEBUG_VAR("GlobDynamicMem.at >", git);
    return git;
  }

  /**
   * Resolve the global iterator referencing an element position in a unit's
   * local memory.
   */
  template<typename IndexT>
  const_global_iterator at(
    /// The unit id
    team_unit_t unit,
    /// The unit's local address offset
    IndexT      local_index) const
  {
    DASH_LOG_DEBUG("GlobDynamicMem.at() const",
                   "unit:", unit, "lidx:", local_index);
    if (_nunits == 0) {
      DASH_THROW(dash::exception::RuntimeError, "No units in team");
    }
    // TODO
    const_global_iterator git(this, unit, local_index);
    DASH_LOG_DEBUG_VAR("GlobDynamicMem.at const >", git);
    return git;
  }

  inline const bucket_list & local_buckets() const
  {
    return _buckets;
  }

private:
  /**
   * Commit global deallocation of buffers marked for detach.
   */
  size_type commit_detach()
  {
    DASH_LOG_TRACE("GlobDynamicMem.commit_detach()");
    DASH_LOG_TRACE("GlobDynamicMem.commit_detach",
                   "local buckets to detach:", _num_detach_buckets.local[0]);
    // Number of elements successfully deallocated from global memory in
    // this commit:
    size_type num_detached_elem = 0;
    for (auto bucket_it = _detach_buckets.begin();
         bucket_it != _detach_buckets.cend(); ++bucket_it) {
      DASH_LOG_TRACE("GlobDynamicMem.commit_detach", "detaching bucket:",
                     "size:", bucket_it->size,
                     "lptr:", bucket_it->lptr,
                     "gptr:", bucket_it->gptr);
      // Detach bucket from global memory region and deallocate its local
      // memory segment:
      if (bucket_it->attached) {
        _allocator.deallocate(bucket_it->gptr);
        num_detached_elem   += bucket_it->size;
        bucket_it->attached  = false;
      }
    }
    _detach_buckets.clear();
    DASH_LOG_TRACE("GlobDynamicMem.commit_detach >",
                   "globally deallocated elements:", num_detached_elem);
    return num_detached_elem;
  }

  /**
   * Commit global allocation of buffers marked for attach.
   */
  size_type commit_attach()
  {
    DASH_LOG_TRACE("GlobDynamicMem.commit_attach()");
    DASH_LOG_TRACE("GlobDynamicMem.commit_attach",
                   "local buckets to attach:", _num_attach_buckets.local[0]);
    // Unregister buckets marked for detach in global memory:
    _num_attach_buckets.barrier();
    // Minumum and maximum number of buckets to be attached by any unit:
    auto min_max_attach     = gather_min_max(_num_attach_buckets.begin(),
                                             _num_attach_buckets.end());
    auto min_attach_buckets = min_max_attach.first;
    auto max_attach_buckets = min_max_attach.second;
    DASH_LOG_TRACE("GlobDynamicMem.commit_attach",
                   "min. attach buckets:",  min_attach_buckets);
    DASH_LOG_TRACE("GlobDynamicMem.commit_attach",
                   "max. attach buckets:",  max_attach_buckets);
    // Number of buckets successfully attached in this commit:
    size_type num_attached_buckets = 0;
    // Number of elements allocated in global memory in this commit:
    size_type num_attached_elem    = 0;
    // Number of elements at remote units before the commit:
    size_type old_remote_size      = _remote_size;
    _remote_size                   = update_remote_size();
    // Whether at least one remote unit needs to attach additional global
    // memory:
    bool has_remote_attach         = _remote_size > old_remote_size;
    DASH_LOG_TRACE_VAR("GlobDynamicMem.commit_attach", old_remote_size);
    DASH_LOG_TRACE_VAR("GlobDynamicMem.commit_attach", _remote_size);
    DASH_LOG_TRACE_VAR("GlobDynamicMem.commit_attach", size());
    DASH_LOG_TRACE_VAR("GlobDynamicMem.commit_attach", has_remote_attach);
    // Plausibility check:
    DASH_ASSERT(!has_remote_attach || max_attach_buckets > 0);

    // Attach local unattached buckets in global memory space.
    // As bucket sizes differ between units, units must collect gptr's
    // (dart_gptr_t) and size of buckets attached by other units and store
    // them locally so a remote unit's local index can be mapped to the
    // remote unit's bucket.
    if (min_attach_buckets == 0 && max_attach_buckets == 0) {
      DASH_LOG_TRACE("GlobDynamicMem.commit_attach", "no attach");
      DASH_ASSERT(_attach_buckets_first == _buckets.end());
      DASH_ASSERT(_buckets.empty() || _buckets.back().attached);
    }
    DASH_LOG_TRACE("GlobDynamicMem.commit_attach", "attaching",
                   std::distance(_attach_buckets_first, _buckets.end()),
                   "buckets");
    for (; _attach_buckets_first != _buckets.end(); ++_attach_buckets_first) {
      bucket_type & bucket = *_attach_buckets_first;
      DASH_ASSERT(!bucket.attached);
      DASH_LOG_TRACE("GlobDynamicMem.commit_attach", "attaching bucket");
      DASH_LOG_TRACE_VAR("GlobDynamicMem.commit_attach", bucket.size);
      DASH_LOG_TRACE_VAR("GlobDynamicMem.commit_attach", bucket.lptr);
      // Attach bucket's local memory segment in global memory:
      bucket.gptr     = _allocator.attach(bucket.lptr, bucket.size);
      bucket.attached = true;
      DASH_LOG_TRACE("GlobDynamicMem.commit_attach", "attached bucket:",
                     "gptr:", bucket.gptr);
      num_attached_elem            += bucket.size;
      _num_attach_buckets.local[0] -= 1;
      num_attached_buckets++;
    }
    DASH_ASSERT(_attach_buckets_first == _buckets.end());
    // All units must attach the same number of buckets collectively.
    // Attach empty buckets if this unit attached less than the maximum
    // number of buckets attached by any other unit in this commit:
    DASH_LOG_TRACE("GlobDynamicMem.commit_attach",
                   "local buckets attached:", num_attached_buckets);
    DASH_LOG_TRACE("GlobDynamicMem.commit_attach",
                   "buckets required to attach:", max_attach_buckets);
    while (num_attached_buckets < max_attach_buckets) {
      DASH_LOG_TRACE("GlobDynamicMem.commit_attach", "attaching null bucket");
      bucket_type bucket;
      bucket.size     = 0;
      bucket.lptr     = nullptr;
      bucket.attached = true;
      bucket.gptr     = _allocator.attach(bucket.lptr, bucket.size);
      DASH_ASSERT(!DART_GPTR_ISNULL(bucket.gptr));
      _buckets.push_back(bucket);
      num_attached_buckets++;
      DASH_LOG_TRACE("GlobDynamicMem.commit_attach", "attached null bucket:",
                     "gptr:", bucket.gptr,
                     "left:", max_attach_buckets - num_attached_buckets);
    }
    DASH_LOG_TRACE("GlobDynamicMem.commit_attach >",
                   "globally allocated elements:", num_attached_elem);
    return num_attached_elem;
  }

  template<
    typename GlobalIt,
    typename ValueType = typename GlobalIt::value_type >
  std::pair<ValueType, ValueType> gather_min_max(
    const GlobalIt & first,
    const GlobalIt & last)
  {
    // TODO: Unoptimized, use dash::min_max_element once it is available
    //
    DASH_LOG_TRACE("GlobDynamicMem.gather_min_max()");
    size_type * lcopy     = new ValueType[dash::distance(first, last)];
    size_type * lcopy_end = dash::copy(first, last, lcopy);
    auto min_lptr  = std::min_element(lcopy, lcopy_end);
    auto max_lptr  = std::max_element(lcopy, lcopy_end);
    std::pair<ValueType, ValueType> min_max;
    min_max.first  = *min_lptr;
    min_max.second = *max_lptr;
    delete[] lcopy;
    DASH_LOG_TRACE("GlobDynamicMem.gather_min_max >",
                   "min:", min_max.first,
                   "max:", min_max.second);
    return min_max;
  }

  /**
   * Request the size of all units' local memory, including unattached memory
   * regions, and update the capacity of global memory space.
   */
  size_type update_remote_size()
  {
    // This function updates local snapshots of the remote unit's local
    // sizes.
    // The following members are updated:
    //
    // _remote_size:
    //    The sum of all remote units' local size, including the sizes of
    //    unattached buckets.
    //
    // _bucket_cumul_sizes:
    //    An array mapping units to a list of their cumulative bucket sizes
    //    (i.e. postfix sum) which is required to iterate over the
    //    non-contigous global dynamic memory space.
    //    For example, if unit 2 allocated buckets with sizes 1, 3 and 5,
    //    _bucket_cumul_sizes[2] is a list { 1, 4, 9 }.
    //
    // Outline:
    //
    // 1. Create local copy of the distributed array _num_attach_buckets that
    //    contains the number of unattached buckets of every unit.
    // 2. Temporarily attach an array attach_bucket_sizes in global memory
    //    that contains the sizes of this unit's unattached buckets.
    // 3. At this point, every unit published the number of buckets it will
    //    attach in the next commit, and their sizes.
    //    The current local size Lu of every unit, including its unattached
    //    buckets, is stored in _local_sizes.
    // 4. For every remote unit u:
    //    - If unit u has one unattached bucket, append the unit's current
    //      local size Lu to the unit's list of cumulative bucket sizes.
    //    - If unit u has more than one unattached bucket, the sizes of the
    //      single buckets must be retrieved from the vector
    //      attach_bucket_sizes temporarily attached by u in step 1.
    // 5. Detach vector attach_bucket_sizes.

    DASH_LOG_TRACE("GlobDynamicMem.update_remote_size()");
    size_type new_remote_size = 0;
    // Number of unattached buckets of every unit:
    std::vector<size_type> num_unattached_buckets(_nunits, 0);
    _num_attach_buckets.barrier();
    dash::copy(_num_attach_buckets.begin(),
               _num_attach_buckets.end(),
               num_unattached_buckets.data());
    // Attach array of local unattached bucket sizes to allow remote units to
    // query the sizes of this unit's unattached buckets.
    std::vector<size_type> attach_buckets_sizes;
    for (auto bit = _attach_buckets_first; bit != _buckets.end(); ++bit) {
      attach_buckets_sizes.push_back((*bit).size);
    }
    DASH_LOG_TRACE_VAR("GlobDynamicMem.update_remote_size",
                       attach_buckets_sizes);
    // Use same allocator type as used for values in global memory:
    typedef typename allocator_type::template rebind<size_type>::other
      size_type_allocator_t;
    size_type_allocator_t attach_buckets_sizes_allocator(_allocator.team());
    auto attach_buckets_sizes_gptr = attach_buckets_sizes_allocator.attach(
                                       &attach_buckets_sizes[0],
                                       attach_buckets_sizes.size());
    _team->barrier();
    // Implicit barrier in allocator.attach
    DASH_LOG_TRACE_VAR("GlobDynamicMem.update_remote_size",
                       attach_buckets_sizes_gptr);
    for (int u = 0; u < _nunits; ++u) {
      if (u == _myid) {
        continue;
      }
      DASH_LOG_TRACE("GlobDynamicMem.update_remote_size",
                     "collecting local bucket sizes of unit", u);
      // Last known local attached capacity of remote unit:
      auto & u_bucket_cumul_sizes = _bucket_cumul_sizes[u];
      // Request current locally allocated capacity of remote unit:
      size_type u_local_size_old  = u_bucket_cumul_sizes.size() == 0
                                    ? 0
                                    : u_bucket_cumul_sizes.back();
      size_type u_local_size_new  = _local_sizes[u];
      DASH_LOG_TRACE_VAR("GlobDynamicMem.update_remote_size",
                         u_local_size_old);
      DASH_LOG_TRACE_VAR("GlobDynamicMem.update_remote_size",
                         u_local_size_old);
      int u_local_size_diff  = u_local_size_new - u_local_size_old;
      new_remote_size       += u_local_size_new;
      // Number of unattached buckets of unit u:
      size_type u_num_attach_buckets = num_unattached_buckets[u];
      DASH_LOG_TRACE_VAR("GlobDynamicMem.update_remote_size",
                         u_num_attach_buckets);
      if (u_num_attach_buckets == 0) {
        // No unattached buckets at unit u.
      } else if (u_num_attach_buckets == 1) {
        // One unattached bucket at unit u, no need to request single bucket
        // sizes:
        u_bucket_cumul_sizes.push_back(u_local_size_new);
      } else {
        // Unit u has multiple unattached buckets.
        // Request sizes of single unattached buckets of unit u:
        std::vector<size_type> u_attach_buckets_sizes(
                                 u_num_attach_buckets, 0);
        dart_gptr_t u_attach_buckets_sizes_gptr = attach_buckets_sizes_gptr;
        dart_gptr_setunit(&u_attach_buckets_sizes_gptr, u);
        dart_storage_t ds = dash::dart_storage<size_type>(u_num_attach_buckets);
        DASH_ASSERT_RETURNS(
          dart_get_blocking(
            // local dest:
            u_attach_buckets_sizes.data(),
            // global source:
            u_attach_buckets_sizes_gptr,
            // request bytes (~= number of sizes) from unit u:
            ds.nelem, ds.dtype),
          DART_OK);
        // Update local snapshot of cumulative bucket sizes at unit u:
        for (int bi = 0; bi < u_num_attach_buckets; ++bi) {
          size_type single_bkt_size = u_attach_buckets_sizes[bi];
          size_type cumul_bkt_size  = single_bkt_size;
          DASH_LOG_TRACE_VAR("GlobDynamicMem.update_remote_size",
                             single_bkt_size);
          if (u_bucket_cumul_sizes.size() > 0) {
            cumul_bkt_size += u_bucket_cumul_sizes.back();
          }
          u_bucket_cumul_sizes.push_back(cumul_bkt_size);
        }
      }
      // Local memory space of unit shrunk:
      if (u_local_size_diff < 0 && u_bucket_cumul_sizes.size() > 0) {
        u_bucket_cumul_sizes.back() += u_local_size_diff;
      }
    }
    // Detach array of local unattached bucket sizes, implicit barrier:
    attach_buckets_sizes_allocator.detach(attach_buckets_sizes_gptr);
    // Implicit barrier in allocator.detach
    _team->barrier();
#if DASH_ENABLE_TRACE_LOGGING
    for (int u = 0; u < _nunits; ++u) {
      DASH_LOG_TRACE("GlobDynamicMem.update_remote_size",
                     "unit", u,
                     "cumulative bucket sizes:", _bucket_cumul_sizes[u]);
    }
#endif
    DASH_LOG_TRACE("GlobDynamicMem.update_remote_size >", new_remote_size);
    _remote_size = new_remote_size;
    return _remote_size;
  }

  /**
   * Global pointer referencing an element position in a unit's bucket.
   */
  dart_gptr_t dart_gptr_at(
    /// Unit id mapped to address in global memory space.
    team_unit_t unit,
    /// Index of bucket containing the referenced address.
    index_type  bucket_index,
    /// Offset of the referenced address in the bucket's memory space.
    index_type  bucket_phase) const
  {
    DASH_LOG_DEBUG("GlobDynamicMem.dart_gptr_at(u,bi,bp)",
                   unit, bucket_index, bucket_phase);
    if (_nunits == 0) {
      DASH_THROW(dash::exception::RuntimeError, "No units in team");
    }
    // Get the referenced bucket's dart_gptr:
    auto bucket_it = _buckets.begin();
    std::advance(bucket_it, bucket_index);
    auto dart_gptr = bucket_it->gptr;
    DASH_LOG_TRACE_VAR("GlobDynamicMem.dart_gptr_at", bucket_it->attached);
    DASH_LOG_TRACE_VAR("GlobDynamicMem.dart_gptr_at", bucket_it->gptr);
    if (unit == _myid) {
      DASH_LOG_TRACE_VAR("GlobDynamicMem.dart_gptr_at", bucket_it->lptr);
      DASH_LOG_TRACE_VAR("GlobDynamicMem.dart_gptr_at", bucket_it->size);
      DASH_ASSERT_LT(bucket_phase, bucket_it->size,
                     "bucket phase out of bounds");
    }
    if (DART_GPTR_ISNULL(dart_gptr)) {
      DASH_LOG_TRACE("GlobDynamicMem.dart_gptr_at",
                     "bucket.gptr is DART_GPTR_NULL");
      dart_gptr = DART_GPTR_NULL;
    } else {
      // Move dart_gptr to unit and local offset:
      DASH_ASSERT_RETURNS(
        dart_gptr_setunit(&dart_gptr,
            _team->global_id(unit)),
        DART_OK);
      DASH_ASSERT_RETURNS(
        dart_gptr_incaddr(
          &dart_gptr,
          bucket_phase * sizeof(value_type)),
        DART_OK);
    }
    DASH_LOG_DEBUG("GlobDynamicMem.dart_gptr_at >", dart_gptr);
    return dart_gptr;
  }

private:
  allocator_type             _allocator;
  dash::Team               * _team;
  dart_team_t                _teamid;
  size_type                  _nunits = 0;
  local_iterator             _lbegin = nullptr;
  local_iterator             _lend   = nullptr;
  team_unit_t                _myid{DART_UNDEFINED_UNIT_ID};
  /// Buckets in local memory space, partitioned by allocated state:
  ///   [ attached buckets, ... , unattached buckets, ... ]
  /// Buckets in this list represent the local iteration- and memory space.
  bucket_list                _buckets;
  /// List of buckets marked for detach.
  bucket_list                _detach_buckets;
  /// Iterator to first unattached bucket.
  bucket_iterator            _attach_buckets_first;
  /// Mapping unit id to number of elements in the unit's attached local
  /// memory space.
  local_sizes_map            _local_sizes;
  /// An array mapping units to a list of their cumulative bucket sizes
  /// (i.e. postfix sum) which is required to iterate over the
  /// non-contigous global dynamic memory space.
  /// For example, if unit 2 allocated buckets with sizes 1,3,5, the
  /// list at _bucket_cumul_sizes[2] has values 1,4,9.
  bucket_cumul_sizes_map     _bucket_cumul_sizes;
  /// Mapping unit id to number of buckets marked for attach in the unit's
  /// memory space.
  local_sizes_map            _num_attach_buckets;
  /// Mapping unit id to number of buckets marked for detach in the unit's
  /// memory space.
  local_sizes_map            _num_detach_buckets;
  /// Total number of elements in attached memory space of remote units.
  size_type                  _remote_size = 0;
  /// Global iterator referencing start of global memory space.
  global_iterator            _begin;
  /// Global iterator referencing the final position in global memory space.
  global_iterator            _end;

}; // class GlobDynamicMem

} // namespace dash

#endif // DASH__GLOB_DYNAMIC_MEM_H_
