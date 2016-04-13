#ifndef DASH__GLOB_DYNAMIC_MEM_H_
#define DASH__GLOB_DYNAMIC_MEM_H_

#include <dash/dart/if/dart.h>

#include <dash/Types.h>
#include <dash/GlobPtr.h>
#include <dash/Allocator.h>
#include <dash/Team.h>
#include <dash/Onesided.h>
#include <dash/Array.h>

#include <dash/algorithm/MinMax.h>
#include <dash/algorithm/Copy.h>

#include <dash/allocator/LocalBucketIter.h>
#include <dash/allocator/GlobBucketIter.h>

#include <dash/internal/Logging.h>
#include <dash/internal/allocator/GlobDynamicMemTypes.h>

#include <list>
#include <vector>
#include <iterator>
#include <sstream>
#include <iostream>


namespace dash {

/**
 * Global memory region with dynamic size.
 *
 * Conventional global memory (see \c dash::GlobMem) allocates a single
 * contiguous range of fixed size in local memory at every unit.
 * Iterating local memory space is therefore easy to implement as native
 * pointer arithmetics can be used to traverse elements in canonical storage
 * order.
 * In global dynamic memory, units allocate multiple heap-allocated buckets
 * of identical size in local memory.
 * The number of local buckets may differ between units.
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
  typedef GlobRef<ElementType>                                    reference;
  typedef GlobRef<const ElementType>                        const_reference;

  typedef ElementType &                                     local_reference;
  typedef const ElementType &                         const_local_reference;

  typedef LocalBucketIter<value_type, index_type>
    local_iterator;
  typedef LocalBucketIter<const value_type, index_type>
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

    _lbegin = lbegin(_myid);
    _lend   = lend(_myid);

    DASH_LOG_TRACE("GlobDynamicMem.GlobDynamicMem",
                   "allocating initial memory space");
    grow(n_local_elem);
    commit();

    DASH_LOG_TRACE("GlobDynamicMem.GlobDynamicMem",
                   "initialize global begin() iterator");
    _begin = at(0, 0);

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
  bool operator==(const self_t & rhs) const
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
  inline size_type local_size() const
  {
    return _local_sizes.local[0];
  }

  /**
   * Number of elements in local memory space of given unit.
   */
  inline size_type local_size(dart_unit_t unit) const
  {
    DASH_ASSERT_RANGE(0, unit, _nunits-1, "unit id out of range");
    return _bucket_cumul_sizes[unit].back();
  }

  /**
   * Inequality comparison operator.
   */
  bool operator!=(const self_t & rhs) const
  {
    return !(*this == rhs);
  }

  /**
   * Increase capacity of local segment of global memory region by the given
   * number of elements.
   * Same as \c resize(local_size() + num_elements).
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
   * Same as \c resize(local_size() - num_elements).
   *
   * Local operation.
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
    num_elements = std::min(local_size(), num_elements);
    DASH_LOG_DEBUG_VAR("GlobDynamicMem.shrink()", num_elements);
    if (num_elements == 0) {
      DASH_LOG_DEBUG("GlobDynamicMem.shrink >", "no shrink");
      return;
    }
    DASH_LOG_TRACE("GlobDynamicMem.shrink",
                   "current local size:", _local_sizes.local[0]);
    auto num_dealloc = num_elements;
    // Try to reduce local capacity by deallocating un-attached local buckets
    // as they do not have to be detached collectively:
    bucket_type & bucket_last = _buckets.back();
    if (!bucket_last.attached) {
      if (bucket_last.size <= num_dealloc) {
        DASH_LOG_TRACE("GlobDynamicMem.shrink",
                       "removing unattached bucket:",
                       "size:", bucket_last.size);
        // mark entire bucket for deallocation:
        num_dealloc           -= bucket_last.size;
        _local_sizes.local[0] -= bucket_last.size;
        _bucket_cumul_sizes[_myid].pop_back();
        _buckets.pop_back();
      } else if (bucket_last.size > num_dealloc) {
        DASH_LOG_TRACE("GlobDynamicMem.shrink",
                       "shrinking unattached bucket:",
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
        DASH_LOG_TRACE("GlobDynamicMem.shrink",
                       "shrinking attached bucket:",
                       "old size:", bucket_it->size,
                       "new size:", bucket_it->size - num_dealloc);
        bucket_it->size                   -= num_dealloc;
        _local_sizes.local[0]             -= num_dealloc;
        _bucket_cumul_sizes[_myid].back() -= num_dealloc;
        num_dealloc = 0;
      }
    }
    // Mark attached buckets for deallocation:
    DASH_LOG_DEBUG_VAR("GlobDynamicMem.shrink", num_dealloc_gbuckets);
    while (num_dealloc_gbuckets-- > 0) {
      auto dealloc_bucket = _buckets.back();
      DASH_LOG_TRACE("GlobDynamicMem.shrink",
                     "deallocating attached bucket:"
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

    DASH_LOG_DEBUG("GlobDynamicMem.shrink >", "finished - ",
                   "new local size:",           _local_sizes.local[0],
                   "new iteration space size:", std::distance(
                                                  _lbegin, _lend),
                   "total number of buckets:",  _buckets.size(),
                   "unattached bucket:",        !_buckets.back().attached);
  }

  /**
   * Commit changes of local memory region to global memory space.
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
      _begin             = global_iterator(this, 0, 0);
      value_type v_begin = *_begin;
      dash__unused(v_begin);
    }
    // Update local iterators as bucket iterators might have changed:
    _lbegin = lbegin(_myid);
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
    index_type diff_capacity = num_elements - local_size();
    if (diff_capacity > 0) {
      grow(diff_capacity);
    } else if (diff_capacity < 0) {
      shrink(-diff_capacity);
    }
  }

  /**
   * Global pointer of the initial address of the global memory.
   */
  global_iterator begin()
  {
    return _begin;
  }

  /**
   * Global pointer of the initial address of the global memory.
   */
  const_global_iterator begin() const
  {
    return const_global_iterator(_begin);
  }

  /**
   * Global pointer of the initial address of the global memory.
   */
  reverse_global_iterator rbegin()
  {
    return reverse_global_iterator(_begin + size());
  }

  /**
   * Global pointer of the initial address of the global memory.
   */
  const_reverse_global_iterator rbegin() const
  {
    return const_reverse_global_iterator(_begin + size());
  }

  /**
   * Global pointer of the initial address of the global memory.
   */
  global_iterator end()
  {
    return _begin + size();
  }

  /**
   * Global pointer of the initial address of the global memory.
   */
  const_global_iterator end() const
  {
    return const_global_iterator(_begin + size());
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
    return const_reverse_global_iterator(_begin);
  }

  /**
   * Native pointer of the initial address of the local memory of
   * a unit.
   */
  local_iterator lbegin(
    dart_unit_t unit_id)
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
   */
  const_local_iterator lbegin(
    dart_unit_t unit_id) const
  {
    DASH_LOG_TRACE_VAR("GlobDynamicMem.lbegin const()", unit_id);
    if (unit_id == _myid) {
      const_local_iterator unit_clbegin(
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
  inline local_iterator lbegin()
  {
    return _lbegin;
  }

  /**
   * Native pointer of the initial address of the local memory of
   * the unit that initialized this GlobDynamicMem instance.
   */
  inline const_local_iterator lbegin() const
  {
    return const_local_iterator(_lbegin);
  }

  /**
   * Native pointer of the final address of the local memory of
   * a unit.
   */
  local_iterator lend(
    dart_unit_t unit_id)
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
    dart_unit_t unit_id) const
  {
    DASH_LOG_TRACE_VAR("GlobDynamicMem.lend() const", unit_id);
    if (unit_id == _myid) {
      const_local_iterator unit_clend(
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
  inline local_iterator lend()
  {
    return _lend;
  }

  /**
   * Native pointer of the initial address of the local memory of
   * the unit that initialized this GlobDynamicMem instance.
   */
  inline const_local_iterator lend() const
  {
    return const_local_iterator(_lend);
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
    dart_unit_t unit,
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
    dart_unit_t unit,
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
    // Unregister buckets marked for detach in global memory:
    _num_detach_buckets.barrier();
    // Minumum and maximum number of buckets to be attached by any unit:
    auto min_max_detach     = gather_min_max(_num_detach_buckets.begin(),
                                             _num_detach_buckets.end());
    auto min_detach_buckets = min_max_detach.first;
    auto max_detach_buckets = min_max_detach.second;
    DASH_LOG_TRACE("GlobDynamicMem.commit_detach",
                   "min. detach buckets:",  min_detach_buckets);
    DASH_LOG_TRACE("GlobDynamicMem.commit_detach",
                   "max. detach buckets:",  min_detach_buckets);
    // Number of elements successfully deallocated from global memory in
    // this commit:
    size_type num_detached_elem = 0;
    if (min_detach_buckets == 0 && max_detach_buckets == 0) {
      DASH_LOG_TRACE("GlobDynamicMem.commit_detach", "no detach");
    } else if (min_detach_buckets == max_detach_buckets) {
      DASH_LOG_TRACE("GlobDynamicMem.commit_detach",
                     "balanced number of buckets to detach");
      for (auto bucket_it = _detach_buckets.begin();
           bucket_it != _detach_buckets.cend(); ++bucket_it) {
        DASH_LOG_TRACE("GlobDynamicMem.commit_detach", "detaching bucket:",
                       "size:", bucket_it->size,
                       "lptr:", bucket_it->lptr,
                       "gptr:", bucket_it->gptr);
        // Detach bucket from global memory region and deallocate its local
        // memory segment:
        if (bucket_it->attached) {
          _allocator.detach(bucket_it->gptr);
          num_detached_elem   += bucket_it->size;
          bucket_it->attached  = false;
        }
      }
      _detach_buckets.clear();
    } else {
      // Cannot detach
      DASH_LOG_TRACE("GlobDynamicMem.commit_detach",
                     "inbalanced number of buckets to detach");
    }
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
    bool remote_attach             = _remote_size != old_remote_size;
    DASH_LOG_TRACE_VAR("GlobDynamicMem.commit_attach", old_remote_size);
    DASH_LOG_TRACE_VAR("GlobDynamicMem.commit_attach", _remote_size);
    DASH_LOG_TRACE_VAR("GlobDynamicMem.commit_attach", size());
    DASH_LOG_TRACE_VAR("GlobDynamicMem.commit_attach", remote_attach);
    // Plausibility check:
    DASH_ASSERT(!remote_attach || max_attach_buckets > 0);

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
    for (; _attach_buckets_first != _buckets.end(); ++_attach_buckets_first) {
      bucket_type & bucket = *_attach_buckets_first;
      DASH_ASSERT(!bucket.attached);
      DASH_LOG_TRACE("GlobDynamicMem.commit_attach", "attaching bucket");
      DASH_LOG_TRACE_VAR("GlobDynamicMem.commit_attach", bucket.attached);
      DASH_LOG_TRACE_VAR("GlobDynamicMem.commit_attach", bucket.size);
      DASH_LOG_TRACE_VAR("GlobDynamicMem.commit_attach", bucket.lptr);
      DASH_LOG_TRACE_VAR("GlobDynamicMem.commit_attach", bucket.gptr);
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
      DASH_ASSERT(bucket.gptr != DART_GPTR_NULL);
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
    DASH_LOG_TRACE("GlobDynamicMem.gather_min_max >"
                   "min. detach buckets:", min_max.first,
                   "max. detach buckets:", min_max.second);
    return min_max;
  }

  /**
   * Collect and update capacity of global attached memory space.
   */
  size_type update_remote_size()
  {
    DASH_LOG_TRACE("GlobDynamicMem.update_remote_size");
    size_type new_remote_size = 0;
    for (int u = 0; u < _nunits; ++u) {
      if (u != _myid) {
        // Last known locally allocated capacity of remote unit:
        auto    & u_bucket_cumul_sizes = _bucket_cumul_sizes[u];
        size_type u_local_size_old     = u_bucket_cumul_sizes.size() > 0
                                         ? u_bucket_cumul_sizes.back()
                                         : 0;
        // Request current locally allocated capacity of remote unit:
        size_type u_local_size_new     = _local_sizes[u];
        DASH_LOG_TRACE("GlobDynamicMem.update_remote_size",
                       "unit", u,
                       "old local size:", u_local_size_old,
                       "new local size:", u_local_size_new);
        u_bucket_cumul_sizes.push_back(u_local_size_new);
        new_remote_size += u_local_size_new;
      }
    }
    if (new_remote_size == _remote_size) {
      for (int u = 0; u < _nunits; ++u) {
        if (u != _myid) {
          // No new buckets allocated at any unit, undo updates of
          // accumulated bucket sizes:
          _bucket_cumul_sizes[u].pop_back();
        }
      }
    }
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
    dart_unit_t unit,
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
    DASH_LOG_TRACE_VAR("GlobDynamicMem.dart_gptr_at", bucket_it->lptr);
    DASH_LOG_TRACE_VAR("GlobDynamicMem.dart_gptr_at", bucket_it->gptr);
    DASH_LOG_TRACE_VAR("GlobDynamicMem.dart_gptr_at", bucket_it->size);
    DASH_ASSERT_LT(bucket_phase, bucket_it->size,
                   "bucket phase out of bounds");
    if (DART_GPTR_EQUAL(dart_gptr, DART_GPTR_NULL)) {
      DASH_LOG_TRACE("GlobDynamicMem.dart_gptr_at",
                     "bucket.gptr is DART_GPTR_NULL");
    } else {
      // Move dart_gptr to unit and local offset:
      DASH_ASSERT_RETURNS(
        dart_gptr_setunit(&dart_gptr, unit),
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
  dart_team_t                _teamid;
  size_type                  _nunits = 0;
  local_iterator             _lbegin = nullptr;
  local_iterator             _lend   = nullptr;
  dart_unit_t                _myid   = DART_UNDEFINED_UNIT_ID;
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
  /// Mapping unit id to cumulative size of buckets in the unit's attached
  /// local memory space.
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

}; // class GlobDynamicMem

} // namespace dash

#endif // DASH__GLOB_DYNAMIC_MEM_H_
