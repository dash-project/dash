#ifndef DASH__GLOB_DYNAMIC_MEM_H_
#define DASH__GLOB_DYNAMIC_MEM_H_

#include <dash/dart/if/dart.h>

#include <dash/Types.h>
#include <dash/GlobPtr.h>
#include <dash/Allocator.h>
#include <dash/Team.h>
#include <dash/Onesided.h>
#include <dash/Array.h>

#include <dash/internal/Logging.h>

#include <list>
#include <vector>
#include <iterator>
#include <sstream>
#include <iostream>


namespace dash {
namespace internal {

template<
  typename SizeType,
  typename ElementType >
struct _glob_dynamic_mem_bucket_type
{
  SizeType      size;
  ElementType * lptr;
  dart_gptr_t   gptr;
  bool          attached;
};

#if 0
template<
  typename SizeType,
  typename ElementType >
std::ostream & operator<<(
  std::ostream & os,
  const dash::internal::_glob_dynamic_mem_bucket_type<
          SizeType, ElementType> & bucket)
{
  std::ostringstream ss;
  ss << "GlobDynamicMem::bucket_type("
     << "size: "     << bucket.size << ", "
     << "lptr: "     << bucket.lptr << ", "
     << "gptr: "     << bucket.gptr << ", "
     << "attached: " << bucket.attached
     << ")";
  return operator<<(os, ss.str());
}
#endif

template<
  typename ElementType,
  typename IndexType,
  typename PointerType   = ElementType *,
  typename ReferenceType = ElementType & >
class LocalBucketIter
: public std::iterator<
           std::random_access_iterator_tag,
           ElementType,
           IndexType,
           PointerType,
           ReferenceType >
{
private:
  typedef LocalBucketIter<ElementType, IndexType>
    self_t;

public:
  typedef IndexType                                              index_type;
  typedef typename std::make_unsigned<index_type>::type           size_type;

/// Type definitions required for std::iterator_traits:
public:
  typedef std::random_access_iterator_tag                 iterator_category;
  typedef IndexType                                         difference_type;
  typedef ElementType                                            value_type;
  typedef ElementType *                                             pointer;
  typedef ElementType &                                           reference;

  typedef _glob_dynamic_mem_bucket_type<size_type, value_type>  bucket_type;

private:
  typedef typename std::list<bucket_type>
    bucket_list;
  typedef typename bucket_list::iterator
    bucket_iterator;

public:
  LocalBucketIter(
    const bucket_iterator & bucket_first,
    const bucket_iterator & bucket_last,
    index_type              position,
    const bucket_iterator & bucket_it,
    index_type              bucket_phase)
  : _bucket_first(bucket_first),
    _bucket_last(bucket_last),
    _idx(position),
    _bucket_it(bucket_it),
    _bucket_phase(bucket_phase),
    _is_nullptr(false)
  { }

  LocalBucketIter(
    const bucket_iterator & bucket_first,
    const bucket_iterator & bucket_last,
    index_type              position)
  : _bucket_first(bucket_first),
    _bucket_last(bucket_last),
    _idx(position),
    _bucket_it(bucket_first),
    _bucket_phase(0),
    _is_nullptr(false)
  {
    for (_bucket_it = _bucket_first;
         _bucket_it != _bucket_last; ++_bucket_it) {
      if (position >= _bucket_it->size) {
        position -= _bucket_it->size;
      } else if (position < _bucket_it->size) {
        _bucket_phase = position;
        break;
      }
    }
  }

  LocalBucketIter() = default;

  LocalBucketIter(const self_t & other)
  : _bucket_first(other._bucket_first),
    _bucket_last(other._bucket_last),
    _idx(other._idx),
    _bucket_it(other._bucket_it),
    _bucket_phase(other._bucket_phase),
    _is_nullptr(other._is_nullptr)
  { }

  self_t & operator=(const self_t & rhs)
  {
    if (this != &rhs) {
      _bucket_first = rhs._bucket_first;
      _bucket_last  = rhs._bucket_last;
      _idx          = rhs._idx;
      _bucket_it    = rhs._bucket_it;
      _bucket_phase = rhs._bucket_phase;
      _is_nullptr   = rhs._is_nullptr;
    }
    return *this;
  }

  LocalBucketIter(std::nullptr_t)
  : _is_nullptr(true)
  { }

  self_t & operator=(std::nullptr_t)
  {
    _is_nullptr = true;
    return *this;
  }

  inline bool operator==(std::nullptr_t) const
  {
    return _is_nullptr;
  }

  inline bool operator!=(std::nullptr_t) const
  {
    return !_is_nullptr;
  }

public:
  reference operator*()
  {
    return _bucket_it->lptr[_bucket_phase];
  }

  reference operator[](index_type offset)
  {
    if (_bucket_phase + offset < _bucket_it->size) {
      // element is in bucket currently referenced by this iterator:
      return _bucket_it->lptr[_bucket_phase + offset];
    } else {
      // find bucket containing element at given offset:
      for (auto b_it = _bucket_it; b_it != _bucket_last; ++b_it) {
        if (offset >= b_it->size) {
          offset -= b_it->size;
        } else if (offset < b_it->size) {
          return b_it->lptr[offset];
        }
      }
    }
    DASH_THROW(dash::exception::OutOfRange,
               "offset " << offset << " is out of range");
  }

  self_t & operator++()
  {
    increment(1);
    return *this;
  }

  self_t & operator--()
  {
    decrement(1);
    return *this;
  }

  self_t & operator++(int)
  {
    auto res = *this;
    increment(1);
    return res;
  }

  self_t & operator--(int)
  {
    auto res = *this;
    decrement(1);
    return res;
  }

  self_t & operator+=(int offset)
  {
    increment(offset);
    return *this;
  }

  self_t & operator-=(int offset)
  {
    decrement(offset);
    return *this;
  }

  self_t operator+(int offset) const
  {
    auto res = *this;
    res += offset;
    return res;
  }

  self_t operator-(int offset) const
  {
    auto res = *this;
    res -= offset;
    return res;
  }

  inline index_type operator+(
    const self_t & other) const
  {
    return _idx + other._idx;
  }

  inline index_type operator-(
    const self_t & other) const
  {
    return _idx - other._idx;
  }

  inline bool operator<(const self_t & other) const
  {
    return (_idx < other._idx);
  }

  inline bool operator<=(const self_t & other) const
  {
    return (_idx <= other._idx);
  }

  inline bool operator>(const self_t & other) const
  {
    return (_idx > other._idx);
  }

  inline bool operator>=(const self_t & other) const
  {
    return (_idx >= other._idx);
  }

  inline bool operator==(const self_t & other) const
  {
    return (this == &other || _idx == other._idx);
  }

  inline bool operator!=(const self_t & other) const
  {
    return (*this != other);
  }

  constexpr bool is_local() const
  {
    return true;
  }

private:
  void increment(int offset)
  {
    _idx += offset;
    if (_bucket_phase + offset < _bucket_it->size) {
      // element is in bucket currently referenced by this iterator:
      _bucket_phase += offset;
    } else {
      // find bucket containing element at given offset:
      for (; _bucket_it != _bucket_last; ++_bucket_it) {
        if (offset >= _bucket_it->size) {
          offset -= _bucket_it->size;
        } else if (offset < _bucket_it->size) {
          _bucket_phase = offset;
          break;
        }
      }
    }
    // end iterator
    if (_bucket_it == _bucket_last) {
      _bucket_phase = offset;
    }
  }

  void decrement(int offset)
  {
    if (offset > _idx) {
      DASH_THROW(dash::exception::OutOfRange,
                 "offset " << offset << " is out of range");
    }
    _idx -= offset;
    if (offset <= _bucket_phase) {
      // element is in bucket currently referenced by this iterator:
      _bucket_phase -= offset;
    } else {
      offset -= _bucket_phase;
      // find bucket containing element at given offset:
      for (; _bucket_it != _bucket_first; --_bucket_it) {
        if (offset >= _bucket_it->size) {
          offset -= _bucket_it->size;
        } else if (offset < _bucket_it->size) {
          _bucket_phase = _bucket_it->size - offset;
          break;
        }
      }
    }
    if (_bucket_it == _bucket_first) {
      _bucket_phase = _bucket_it->size - offset;
    }
    if (false) {
      DASH_THROW(dash::exception::OutOfRange,
                 "offset " << offset << " is out of range");
    }
  }

private:
  bucket_iterator _bucket_first;
  bucket_iterator _bucket_last;
  index_type      _idx           = 0;
  bucket_iterator _bucket_it;
  index_type      _bucket_phase  = 0;
  bool            _is_nullptr    = false;

}; // class LocalBucketIter

} // namespace internal

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
 *     auto unit_1_last = gdmem.index_to_gptr(1, initial_local_capacity-1);
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

  typedef local_iterator                                      local_pointer;
  typedef const_local_iterator                          const_local_pointer;

  typedef typename local_iterator::bucket_type                  bucket_type;

private:
  typedef typename std::list<bucket_type>
    bucket_list;
  typedef typename bucket_list::iterator
    bucket_iterator;

  typedef dash::Array<size_type, int,
                      dash::CSRPattern<1, dash::ROW_MAJOR, int> >
    local_sizes_map;

public:
  /**
   * Constructor, collectively allocates the given number of elements in
   * local memory of every unit in a team.
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
    _local_sizes(team.size())
  {
    DASH_LOG_TRACE("GlobDynamicMem(nunits,nelem)",
                   team.size(), n_local_elem);

    _bucket_attached_last = _buckets.end();
    _local_sizes.local[0] = 0;

    _lbegin = lbegin(_myid);
    _lend   = lend(_myid);

    grow(n_local_elem);
    commit();
  }

  /**
   * Destructor, collectively frees underlying global memory.
   */
  ~GlobDynamicMem()
  {
    DASH_LOG_TRACE_VAR("GlobDynamicMem.~GlobDynamicMem()", _begptr);
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
    return (_begptr               == rhs._begptr &&
            _teamid               == rhs._teamid &&
            _nunits               == rhs._nunits &&
            _lbegin               == rhs._lbegin &&
            _lend                 == rhs._lend &&
            _buckets              == rhs._buckets &&
            _bucket_attached_last == rhs._bucket_attached_last &&
            _detach_buckets       == rhs._detach_buckets);
  }

  /**
   * Total number of elements in attached memory space, including size of
   * local unattached memory segments.
   */
  inline size_type size() const
  {
    DASH_LOG_DEBUG("GlobDynamicMem.size()",
                   "local size:",  local_size(),
                   "remote size:", _remote_size);
    auto global_size = _remote_size + local_size();
    DASH_LOG_DEBUG("GlobDynamicMem.size >", global_size);
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
  void grow(size_type num_elements)
  {
    DASH_LOG_DEBUG_VAR("GlobDynamicMem.grow()", num_elements);
    DASH_LOG_DEBUG("GlobDynamicMem.grow",
                   "current local size:", _local_sizes.local[0]);
    if (num_elements == 0) {
      DASH_LOG_DEBUG("GlobDynamicMem.grow >", "no grow");
      return;
    }
    // Allocate new bucket:
    auto bucket_lptr = _allocator.allocate_local(num_elements);
    if (nullptr == bucket_lptr) {
      DASH_THROW(dash::exception::RuntimeError, "allocation failed");
    }
    bucket_type bucket;
    bucket.size     = num_elements;
    bucket.lptr     = bucket_lptr;
    bucket.gptr     = DART_GPTR_NULL;
    bucket.attached = false;
    // Add bucket to local memory space:
    _buckets.push_back(bucket);
    _bucket_attached_last = std::find_if(
                              _buckets.begin(),
                              _buckets.end(),
                              [](bucket_type bit) {
                                return !bit.attached;
                              });
    // Update size of local memory space:
    _local_sizes.local[0] += num_elements;
    // Update size of local iteration space:
    _lbegin = lbegin(_myid);
    _lend   = lend(_myid);

    DASH_LOG_DEBUG("GlobDynamicMem.grow", "new bucket:",
                   "size:", bucket.size,
                   "lptr:", bucket.lptr);
    DASH_LOG_DEBUG("GlobDynamicMem.grow >", "finished - ",
                   "new local size:",           _local_sizes.local[0],
                   "new iteration space size:", std::distance(
                                                  _lbegin, _lend),
                   "total number of buckets:",  _buckets.size(),
                   "unattached buckets:",       std::distance(
                                                  _bucket_attached_last,
                                                  _buckets.end()));
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
    DASH_LOG_DEBUG("GlobDynamicMem.shrink",
                   "current local size:", _local_sizes.local[0]);
    auto num_dealloc          = num_elements;
    auto num_realloc          = 0;
    auto num_dealloc_lbuckets = 0;
    // Try to reduce local capacity by deallocating un-attached local buckets
    // as they do not have to be detached collectively:
    for (auto bucket_it = _buckets.rbegin();
         bucket_it.base() != _bucket_attached_last;
         ++bucket_it) {
      if (bucket_it->size <= num_dealloc) {
        // mark entire bucket for deallocation:
        num_dealloc_lbuckets++;
        num_dealloc -= bucket_it->size;
      } else if (bucket_it->size > num_dealloc) {
        // As allocators do not provide resizing local memory segments,
        // mark entire bucket for deallocation and register its remaining
        // size for reallocation:
        num_dealloc_lbuckets++;
        num_realloc += bucket_it->size - num_dealloc;
        num_dealloc  = 0;
      }
      if (num_dealloc == 0) {
        break;
      }
    }
    // Deallocate un-attached buckets marked for deallocation:
    DASH_LOG_DEBUG_VAR("GlobDynamicMem.shrink", num_dealloc_lbuckets);
    while (num_dealloc_lbuckets-- > 0) {
      auto dealloc_bucket = _buckets.back();
      DASH_LOG_DEBUG("GlobDynamicMem.shrink",
                     "deallocating unattached bucket:"
                     "size:", dealloc_bucket.size,
                     "lptr:", dealloc_bucket.lptr);
      _allocator.deallocate_local(dealloc_bucket.lptr);
      // Unregister bucket:
      _buckets.pop_back();
      // Update size of local memory space:
      _local_sizes.local[0] -= dealloc_bucket.size;
    }
    // Number of elements to deallocate exceeds capacity of un-attached
    // buckets, deallocate attached buckets:
    auto num_dealloc_gbuckets = 0;
    for (auto bucket_it = _buckets.rbegin();
         bucket_it != _buckets.rend();
         ++bucket_it) {
      if (bucket_it->size <= num_dealloc) {
        // mark entire bucket for deallocation:
        num_dealloc_gbuckets++;
        num_dealloc -= bucket_it->size;
      } else if (bucket_it->size > num_dealloc) {
        // As allocators do not provide resizing local memory segments,
        // mark entire bucket for deallocation and register its remaining
        // size for reallocation:
        num_dealloc_gbuckets++;
        num_realloc += bucket_it->size - num_dealloc;
        num_dealloc  = 0;
      }
      if (num_dealloc == 0) {
        break;
      }
    }
    // Mark attached buckets for deallocation:
    DASH_LOG_DEBUG_VAR("GlobDynamicMem.shrink", num_dealloc_gbuckets);
    while (num_dealloc_gbuckets-- > 0) {
      auto dealloc_bucket = _buckets.back();
      DASH_LOG_DEBUG("GlobDynamicMem.shrink",
                     "deallocating attached bucket:"
                     "size:", dealloc_bucket.size,
                     "lptr:", dealloc_bucket.lptr);
      // Mark bucket to be detached in next call of commit():
      _detach_buckets.push_back(dealloc_bucket);
      // Unregister bucket:
      _buckets.pop_back();
      // Update size of local memory space:
      _local_sizes.local[0] -= dealloc_bucket.size;
    }
    _bucket_attached_last = std::find_if(
                              _buckets.begin(),
                              _buckets.end(),
                              [](bucket_type bit) {
                                return !bit.attached;
                              });
    if (num_realloc > 0) {
      DASH_LOG_DEBUG("GlobDynamicMem.shrink",
                     "reallocating", num_realloc, "elements");
      grow(num_realloc);
    }
    // Update size of local iteration space:
    _lbegin = lbegin(_myid);
    _lend   = lend(_myid);

    DASH_LOG_DEBUG("GlobDynamicMem.shrink >", "finished - ",
                   "new local size:",           _local_sizes.local[0],
                   "new iteration space size:", std::distance(
                                                  _lbegin, _lend),
                   "total number of buckets:",  _buckets.size(),
                   "unattached buckets:",       std::distance(
                                                  _bucket_attached_last,
                                                  _buckets.end()));
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
    auto num_attach_buckets = std::distance(_bucket_attached_last,
                                            _buckets.end());
    DASH_LOG_DEBUG("GlobDynamicMem.commit()",
                   "buckets to attach:", num_attach_buckets,
                   "buckets to detach:", _detach_buckets.size());
    DASH_LOG_DEBUG_VAR("GlobDynamicMem.commit", _buckets.size());

    barrier();

    // First detach, then attach to minimize number of elements allocated
    // at the same time.

    // Unregister buckets marked for detach in global memory:
    for (auto bucket_it = _detach_buckets.begin();
         bucket_it != _detach_buckets.end();
         ++bucket_it) {
      DASH_LOG_DEBUG("GlobDynamicMem.commit", "detaching bucket:",
                     "size:", bucket_it->size,
                     "lptr:", bucket_it->lptr,
                     "gptr:", bucket_it->gptr);
      // Detach bucket from global memory region and deallocate its local
      // memory segment:
      if (!bucket_it->attached) {
        _allocator.detach(bucket_it->gptr);
        bucket_it->attached = false;
      }
    }
    _detach_buckets.clear();

    // Register buckets marked for attach in global memory:
    for (auto bucket_it = _bucket_attached_last;
         bucket_it != _buckets.end();
         ++bucket_it) {
      DASH_LOG_DEBUG("GlobDynamicMem.commit", "attaching bucket:",
                     "size:", bucket_it->size,
                     "lptr:", bucket_it->lptr,
                     "gptr:", bucket_it->gptr);
      // Attach bucket's local memory segment in global memory:
      bucket_it->gptr = _allocator.attach(
                          bucket_it->lptr,
                          bucket_it->size);
      DASH_LOG_DEBUG("GlobDynamicMem.commit", "attached bucket:",
                     "gptr:", bucket_it->gptr);
      bucket_it->attached = true;
    }
    _bucket_attached_last = _buckets.end();

    // Update new capacity of global attached memory space.
    //
    // TODO:
    //
    //   dash::accumulate(_local_sizes.begin(),
    //                    _local_sizes.end(),
    //                    0, dash::plus<size_type>());
    _remote_size = 0;
    for (int u = 0; u < _nunits; ++u) {
      if (u != _myid) {
        size_type unit_local_size = _local_sizes[u];
        DASH_LOG_DEBUG("GlobDynamicMem.commit",
                       "unit", u, "local size:", unit_local_size);
        _remote_size += unit_local_size;
      }
    }
    DASH_LOG_DEBUG_VAR("GlobDynamicMem.commit", _remote_size);
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
    auto diff_capacity = num_elements - local_size();
    if (diff_capacity > 0) {
      grow(diff_capacity);
    } else if (diff_capacity < 0) {
      shrink(-diff_capacity);
    }
  }

  /**
   * Global pointer of the initial address of the global memory.
   */
  const_pointer begin() const
  {
    return const_pointer(_begptr);
  }

  /**
   * Global pointer of the initial address of the global memory.
   */
  pointer begin()
  {
    return pointer(_begptr);
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
   * the unit that initialized this GlobDynamicMem instance.
   */
  inline const_local_iterator lbegin() const
  {
    return _lbegin;
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
   * Native pointer of the final address of the local memory of
   * a unit.
   */
  const_local_iterator lend(
    dart_unit_t unit_id) const
  {
    DASH_LOG_TRACE_VAR("GlobDynamicMem.lend() const", unit_id);
    if (unit_id == _myid) {
      return const_local_iterator(
               // iteration space
               _buckets.begin(), _buckets.end(),
               // position in iteration space
               local_size(),
               // bucket at position in iteration space,
               // offset in bucket
               _buckets.end(), 0
             );
    } else {
      DASH_THROW(dash::exception::NotImplemented,
                 "dash::GlobDynamicMem.lend(unit) is not implemented "
                 "for unit != dash::myid()");
    }
#if 0
    void *addr;
    dart_gptr_t gptr = begin().dart_gptr();
    DASH_ASSERT_RETURNS(
      dart_gptr_setunit(&gptr, unit_id),
      DART_OK);
    DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(&gptr, _nlelem * sizeof(ElementType)),
      DART_OK);
    DASH_ASSERT_RETURNS(
      dart_gptr_getaddr(gptr, &addr),
      DART_OK);
    return const_local_pointer(addr);
#endif
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
#if 0
    void *addr;
    dart_gptr_t gptr = begin().dart_gptr();
    DASH_ASSERT_RETURNS(
      dart_gptr_setunit(&gptr, unit_id),
      DART_OK);
    DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(&gptr, _nlelem * sizeof(ElementType)),
      DART_OK);
    DASH_ASSERT_RETURNS(
      dart_gptr_getaddr(gptr, &addr),
      DART_OK);
    return local_pointer(addr);
#endif
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
   * Native pointer of the initial address of the local memory of
   * the unit that initialized this GlobDynamicMem instance.
   */
  inline local_iterator lend()
  {
    return _lend;
  }

  inline std::vector<dart_gptr_t> buckets() const
  {
    std::vector<dart_gptr_t> buckets;
    for (auto bit = _buckets.begin(); bit != _bucket_attached_last; ++bit) {
      buckets.push_back(bit->gptr);
    }
    return buckets;
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
    dart_gptr_t gptr = _begptr;
    DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(
        &gptr,
        global_index * sizeof(ValueType)),
      DART_OK);
    dash::put_value(newval, GlobPtr<ValueType>(gptr));
  }

  /**
   * Read value from global memory at given offset.
   *
   * \see  dash::get_value
   */
  template<typename ValueType = ElementType>
  void get_value(
    ValueType  * ptr,
    index_type   global_index)
  {
    DASH_LOG_TRACE("GlobDynamicMem.get_value(newval, gidx = %d)",
                   global_index);
    dart_gptr_t gptr = _begptr;
    dart_gptr_incaddr(&gptr, global_index * sizeof(ValueType));
    dash::get_value(ptr, GlobPtr<ValueType>(gptr));
  }

  /**
   * Synchronize all units associated with this global memory instance.
   */
  void barrier()
  {
    DASH_ASSERT_RETURNS(
      dart_barrier(_teamid),
      DART_OK);
  }

  /**
   * Complete all outstanding asynchronous operations on the referenced
   * global memory on all units.
   */
  void flush()
  {
    dart_flush(_begptr);
  }

  /**
   * Complete all outstanding asynchronous operations on the referenced
   * global memory on all units.
   */
  void flush_all()
  {
    dart_flush_all(_begptr);
  }

  void flush_local()
  {
    dart_flush_local(_begptr);
  }

  void flush_local_all()
  {
    dart_flush_local_all(_begptr);
  }

  /**
   * Resolve the global pointer from an element position in a unit's
   * local memory.
   */
  template<typename IndexT>
  dart_gptr_t index_to_gptr(
    /// The unit id
    dart_unit_t unit,
    /// The unit's local address offset
    IndexT      local_index) const
  {
    DASH_LOG_DEBUG("GlobDynamicMem.index_to_gptr(unit,l_idx)",
                   unit, local_index);
    if (_nunits == 0) {
      DASH_THROW(dash::exception::RuntimeError, "No units in team");
    }
    DASH_THROW(dash::exception::NotImplemented,
               "GlobDynamicMem.index_to_gptr is not implemented");
    // Initialize with global pointer to start address:
    dart_gptr_t gptr = _begptr;
    // Resolve global unit id
    dart_unit_t lunit, gunit;
    DASH_LOG_DEBUG("GlobDynamicMem.index_to_gptr (=g_begp)  ", gptr);
    DASH_LOG_TRACE_VAR("GlobDynamicMem.index_to_gptr", gptr.unitid);
    // Resolve local unit id from global unit id in global pointer:
    dart_team_unit_g2l(_teamid, gptr.unitid, &lunit);
    DASH_LOG_TRACE_VAR("GlobDynamicMem.index_to_gptr", lunit);
    lunit = (lunit + unit) % _nunits;
    DASH_LOG_TRACE_VAR("GlobDynamicMem.index_to_gptr", lunit);
    if (_teamid != dash::Team::All().dart_id()) {
      // Unit is member of a split team, resolve global unit id:
      dart_team_unit_l2g(_teamid, lunit, &gunit);
    } else {
      // Unit is member of top level team, no conversion to global unit id
      // necessary:
      gunit = lunit;
    }
    DASH_LOG_TRACE_VAR("GlobDynamicMem.index_to_gptr", gunit);
    // Apply global unit to global pointer:
    dart_gptr_setunit(&gptr, gunit);
    // Apply local offset to global pointer:
    dart_gptr_incaddr(&gptr, local_index * sizeof(ElementType));
    DASH_LOG_DEBUG("GlobDynamicMem.index_to_gptr (+g_unit) >", gptr);
    return gptr;
  }

private:
  allocator_type             _allocator;
  dart_gptr_t                _begptr;
  dart_team_t                _teamid;
  size_type                  _nunits;
  local_iterator             _lbegin;
  local_iterator             _lend;
  dart_unit_t                _myid;
  /// Buckets in local memory space, partitioned in two segments:
  /// - allocated buckets attached to global memory space.
  /// - allocated buckets to be attached
  ///
  ///   [ ... attached buckets ... | ... allocated buckets ... ]
  ///
  bucket_list                _buckets;
  /// Iterator partitioning buckets into attached and un-attached buckets,
  /// i.e. pointing past the final attached bucket in _buckets.
  bucket_iterator            _bucket_attached_last;
  /// List of buckets marked for detach.
  bucket_list                _detach_buckets;
  /// Map of unit id to number of elements in the unit's attached local
  /// memory space.
  local_sizes_map            _local_sizes;
  /// Total number of elements in attached memory space of remote units.
  size_type                  _remote_size;

}; // class GlobDynamicMem

} // namespace dash

#endif // DASH__GLOB_DYNAMIC_MEM_H_
