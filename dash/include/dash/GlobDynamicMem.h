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

#include <dash/internal/Logging.h>
#include <dash/internal/allocator/GlobDynamicMemTypes.h>
#include <dash/internal/allocator/LocalBucketIter.h>
#include <dash/internal/allocator/GlobBucketIter.h>

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

  typedef internal::LocalBucketIter<value_type, index_type>
    local_iterator;
  typedef internal::LocalBucketIter<const value_type, index_type>
    const_local_iterator;

  typedef internal::GlobBucketIter<
            value_type, self_t, pointer, reference>
    global_iterator;
  typedef internal::GlobBucketIter<
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
  friend class dash::internal::GlobBucketIter;

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
    _local_sizes(team.size()),
    _bucket_cumul_sizes(team.size()),
    _num_detach_buckets(team.size())
  {
    DASH_LOG_TRACE("GlobDynamicMem.(ninit,nunits)",
                   n_local_elem, team.size());

    _local_sizes.local[0]        = 0;
    _num_detach_buckets.local[0] = 0;

    _lbegin = lbegin(_myid);
    _lend   = lend(_myid);

    DASH_LOG_TRACE("GlobDynamicMem.GlobDynamicMem",
                   "allocating initial memory space");
    grow(n_local_elem);
    commit();

    _begin = at(0, 0);
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
    DASH_ASSERT_RANGE(0, unit, _nunits, "unit id out of range");
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
   * \see resize
   * \see shrink
   * \see commit
   */
  local_iterator grow(size_type num_elements)
  {
    DASH_LOG_DEBUG_VAR("GlobDynamicMem.grow()", num_elements);
    DASH_LOG_TRACE("GlobDynamicMem.grow",
                   "current local size:", _local_sizes.local[0]);
    if (num_elements == 0) {
      DASH_LOG_DEBUG("GlobDynamicMem.grow >", "no grow");
      return _lend;
    }
    // Update size of local memory space:
    _local_sizes.local[0] += num_elements;

    if (!_buckets.empty() && !_buckets.back().attached) {
      bucket_type & bucket_last = _buckets.back();
      DASH_LOG_TRACE("GlobDynamicMem.grow",
                     "extending existing unattached bucket,",
                     "old size:", _local_commit_buf.size());
      // Extend existing unattached bucket:
      auto num_unattached = bucket_last.size + num_elements;
      _local_commit_buf.reserve(num_unattached);
      _bucket_cumul_sizes[_myid].back() += num_elements;
      bucket_last.size = num_unattached;
      bucket_last.lptr = &_local_commit_buf[0];
      DASH_ASSERT(bucket_last.gptr == DART_GPTR_NULL);
    } else {
      // Create new unattached bucket:
      DASH_LOG_TRACE("GlobDynamicMem.grow", "creating new unattached bucket:",
                     "size:", num_elements);
      _local_commit_buf.reserve(num_elements);
      bucket_type bucket;
      bucket.size     = num_elements;
      bucket.lptr     = &_local_commit_buf[0];
      bucket.gptr     = DART_GPTR_NULL;
      bucket.attached = false;
      // Add bucket to local memory space:
      _buckets.push_back(bucket);
      _bucket_cumul_sizes[_myid].push_back(_local_sizes.local[0]);
      DASH_LOG_TRACE("GlobDynamicMem.grow", "added unattached bucket:",
                     "size:", bucket.size,
                     "lptr:", bucket.lptr);
    }
    DASH_LOG_TRACE("GlobDynamicMem.grow",
                   "local buffer bucket size:", _local_commit_buf.size());
    // Update local iteration space:
    _lbegin = lbegin(_myid);
    _lend   = lend(_myid);

    DASH_LOG_DEBUG("GlobDynamicMem.grow", "finished - ",
                   "new local size:",           _local_sizes.local[0],
                   "new iteration space size:", std::distance(
                                                  _lbegin, _lend),
                   "total number of buckets:",  _buckets.size(),
                   "unattached bucket:",        !_buckets.back().attached);
    DASH_LOG_DEBUG("GlobDynamicMem.grow >");
    return _lbegin + (local_size() - 1);
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
    // Update local iteration space:
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
    auto num_attach_buckets = _buckets.back().attached ? 0 : 1;
    auto num_detach_buckets = _detach_buckets.size();
    DASH_LOG_DEBUG("GlobDynamicMem.commit()",
                   "buckets to attach:", num_attach_buckets,
                   "buckets to detach:", num_detach_buckets);
    DASH_LOG_TRACE_VAR("GlobDynamicMem.commit", _buckets.size());

    // First detach, then attach to minimize number of elements allocated
    // at the same time.

    // Unregister buckets marked for detach in global memory:
    _num_detach_buckets.local[0] = num_detach_buckets;
    barrier();

    // TODO: use dash::min_max_element once it is available
    //
    auto min_detach_buckets_git  = dash::min_element(
                                     _num_detach_buckets.begin(),
                                     _num_detach_buckets.end());
    auto max_detach_buckets_git  = dash::max_element(
                                     _num_detach_buckets.begin(),
                                     _num_detach_buckets.end());
    auto unit_min_detach_buckets = min_detach_buckets_git.gpos();
    auto unit_max_detach_buckets = max_detach_buckets_git.gpos();
    int  min_detach_buckets      = *min_detach_buckets_git;
    int  max_detach_buckets      = *max_detach_buckets_git;
    if (min_detach_buckets == max_detach_buckets) {
      for (auto bucket_it = _detach_buckets.begin();
           bucket_it != _detach_buckets.end();
           ++bucket_it) {
        DASH_LOG_TRACE("GlobDynamicMem.commit", "detaching bucket:",
                       "size:", bucket_it->size,
                       "lptr:", bucket_it->lptr,
                       "gptr:", bucket_it->gptr);
        // Detach bucket from global memory region and deallocate its local
        // memory segment:
        if (bucket_it->attached) {
          _allocator.detach(bucket_it->gptr);
          bucket_it->attached = false;
        }
      }
      _detach_buckets.clear();
    } else {
      // Cannot detach
      DASH_LOG_TRACE("GlobDynamicMem.commit",
                     "inbalanced number of buckets to detach: ",
                     "local buckets:",      num_detach_buckets, ", ",
                     "min. local buckets:", min_detach_buckets,
                     "at unit",             unit_min_detach_buckets, ", "
                     "max. local buckets:", min_detach_buckets,
                     "at unit",             unit_max_detach_buckets);
    }

    // Attach local buffer bucket in global memory space if it exists and
    // contains values:
    //
    // As bucket sizes differ between units, units must collect gptr's
    // (dart_gptr_t) and size of buckets attached by other units and store
    // them locally so a remote unit's local index can be mapped to the
    // remote unit's bucket.
    //
    if (!_local_commit_buf.empty() &&
        !_buckets.empty() && !_buckets.back().attached) {
      bucket_type & bucket_last = _buckets.back();
      // Extend existing unattached bucket:
      DASH_LOG_TRACE("GlobDynamicMem.commit", "attaching bucket:",
                     "size:", bucket_last.size,
                     "lptr:", bucket_last.lptr,
                     "gptr:", bucket_last.gptr);
      bucket_last.lptr = _allocator.allocate_local(bucket_last.size);
      // Using placement new to avoid copy/assignment as value_type might
      // be const:
      value_type * lptr_alloc = bucket_last.lptr;
      for (auto elem : _local_commit_buf) {
        new (lptr_alloc) value_type(elem);
        ++lptr_alloc;
      }
      _local_commit_buf.resize(0);
      // Attach bucket's local memory segment in global memory:
      bucket_last.gptr = _allocator.attach(
                           bucket_last.lptr,
                           bucket_last.size);
      DASH_LOG_TRACE("GlobDynamicMem.commit", "attached bucket:",
                     "gptr:", bucket_last.gptr);
      bucket_last.attached = true;
    } else {
      // No bucket to attach but attaching is a collective operation.
      // Attach empty bucket:
      DASH_LOG_TRACE("GlobDynamicMem.commit", "attaching empty bucket");
      bucket_type bucket;
      bucket.size     = 0;
      bucket.lptr     = nullptr;
      bucket.gptr     = DART_GPTR_NULL;
      bucket.attached = true;
      bucket.gptr     = _allocator.attach(
                          bucket.lptr,
                          bucket.size);
      DASH_LOG_TRACE("GlobDynamicMem.commit", "attached empty bucket:",
                     "gptr:", bucket.gptr);
    }

    // Update new capacity of global attached memory space.
    //
    // TODO:
    //   dash::accumulate(_local_sizes.begin(),
    //                    _local_sizes.end(),
    //                    0, dash::plus<size_type>());
    _remote_size = 0;
    for (int u = 0; u < _nunits; ++u) {
      if (u != _myid) {
        size_type unit_local_size_new = _local_sizes[u];
        // Accumulating previous local capacity of unit from its list of
        // local bucket sizes instead of creating a local copy of
        // _local_sizes bevore committing as we avoid communication this way:
        auto    & unit_bucket_cumul_sizes   = _bucket_cumul_sizes[u];
        size_type unit_local_size_old = std::accumulate(
                                          unit_bucket_cumul_sizes.begin(),
                                          unit_bucket_cumul_sizes.end(), 0);
        // Size of the bucket attached by unit in this commit phase:
        size_type unit_attached_size  = unit_local_size_new -
                                        unit_local_size_old;
        DASH_LOG_TRACE("GlobDynamicMem.commit",
                       "unit", u,
                       "old local size:", unit_local_size_old,
                       "new local size:", unit_local_size_new,
                       "attached size:",  unit_attached_size);
        unit_bucket_cumul_sizes.push_back(unit_local_size_new);
        _remote_size += unit_local_size_new;
      }
    }
    DASH_LOG_TRACE_VAR("GlobDynamicMem.commit", _remote_size);
    DASH_LOG_TRACE_VAR("GlobDynamicMem.commit", size());
    // Update _begin iterator:
    DASH_LOG_TRACE("GlobDynamicMem.commit", "updating _begin");
    _begin = global_iterator(this, 0, 0);
    value_type v_begin = *_begin;
    dash__unused(v_begin);
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
    return _begin;
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
    return _begin + size();
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
      return local_iterator(
               // iteration space
               _buckets.begin(), _buckets.end(),
               // position in iteration space
               0,
               // bucket at position in iteration space,
               // offset in bucket
               _buckets.begin(), 0);
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
      return const_local_iterator(
               // iteration space
               _buckets.begin(), _buckets.end(),
               // position in iteration space
               0,
               // bucket at position in iteration space,
               // offset in bucket
               _buckets.begin(), 0);
    } else {
      DASH_THROW(dash::exception::NotImplemented,
                 "dash::GlobDynamicMem.lbegin(unit) is not implemented "
                 "for unit != dash::myid()");
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
    return _lbegin;
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
      return local_iterator(
               // iteration space
               _buckets.begin(), _buckets.end(),
               // position in iteration space
               local_size(),
               // bucket at position in iteration space,
               // offset in bucket
               _buckets.end(), 0);
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
    DASH_LOG_TRACE_VAR("GlobDynamicMem.lend()", unit_id);
    if (unit_id == _myid) {
      return const_local_iterator(
               // iteration space
               _buckets.cbegin(), _buckets.cend(),
               // position in iteration space
               local_size(),
               // bucket at position in iteration space,
               // offset in bucket
               _buckets.cend(), 0);
    } else {
      DASH_THROW(dash::exception::NotImplemented,
                 "dash::GlobDynamicMem.lend(unit) is not implemented "
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
    DASH_LOG_DEBUG("GlobDynamicMem.at(unit,l_idx)", unit, local_index);
    if (_nunits == 0) {
      DASH_THROW(dash::exception::RuntimeError, "No units in team");
    }
    global_iterator gbit(this, unit, local_index);
    DASH_LOG_DEBUG("GlobDynamicMem.at >");
    return gbit;
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
    DASH_LOG_DEBUG("GlobDynamicMem.at(unit,l_idx)", unit, local_index);
    if (_nunits == 0) {
      DASH_THROW(dash::exception::RuntimeError, "No units in team");
    }
    const_global_iterator gbit(this, unit, local_index);
    DASH_LOG_DEBUG("GlobDynamicMem.at >");
    return gbit;
  }

private:
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
    // Move dart_gptr to unit and local offset:
    DASH_ASSERT_RETURNS(
      dart_gptr_setunit(&dart_gptr, unit),
      DART_OK);
    DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(
        &dart_gptr,
        bucket_phase * sizeof(value_type)),
      DART_OK);

    DASH_LOG_DEBUG("GlobDynamicMem.dart_gptr_at >", dart_gptr);
    return dart_gptr;
  }

private:
  allocator_type             _allocator;
  dart_team_t                _teamid;
  size_type                  _nunits;
  local_iterator             _lbegin;
  local_iterator             _lend;
  dart_unit_t                _myid;
  /// Buckets in local memory space. Local buffer bucket is last element in
  /// list, if existing.
  ///
  ///   [ attached buckets, ... , local buffer bucket ]
  ///
  bucket_list                _buckets;
  /// List of buckets marked for detach.
  bucket_list                _detach_buckets;
  /// Mapping unit id to number of elements in the unit's attached local
  /// memory space.
  local_sizes_map            _local_sizes;
  /// Mapping unit id to cumulative size of buckets in the unit's attached
  /// local memory space.
  bucket_cumul_sizes_map     _bucket_cumul_sizes;
  /// Mapping unit id to number of buckets marked for detach in the unit's
  /// memory space.
  local_sizes_map            _num_detach_buckets;
  /// Total number of elements in attached memory space of remote units.
  size_type                  _remote_size;
  /// Vector storing elements in unattached bucket.
  std::vector<value_type>    _local_commit_buf;
  /// Global iterator referencing start of global memory space.
  global_iterator            _begin;

}; // class GlobDynamicMem

} // namespace dash

#endif // DASH__GLOB_DYNAMIC_MEM_H_
