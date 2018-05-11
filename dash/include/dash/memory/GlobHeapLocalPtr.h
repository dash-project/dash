#ifndef DASH__MEMORY__GLOB_HEAP_LOCAL_PTR_H__INCLUDED
#define DASH__MEMORY__GLOB_HEAP_LOCAL_PTR_H__INCLUDED

#include <dash/dart/if/dart.h>
#include <dash/Types.h>

#include <dash/internal/Logging.h>

#include <dash/memory/internal/GlobHeapMemTypes.h>

#include <type_traits>
#include <list>
#include <vector>
#include <iterator>
#include <sstream>
#include <iostream>
#include <memory>


namespace dash {

/**
 * Iterator on local buckets. Represents local pointer type.
 */
template<
  typename ElementType,
  typename IndexType,
  typename PointerType   = ElementType *,
  typename ReferenceType = ElementType & >
class GlobHeapLocalPtr
: public std::iterator<
           std::random_access_iterator_tag,
           ElementType,
           IndexType,
           PointerType,
           ReferenceType >
{
  template<typename E_, typename I_, typename P_, typename R_>
  friend class GlobHeapLocalPtr;

  template<typename E_, typename I_, typename P_, typename R_>
  friend std::ostream & dash::operator<<(
    std::ostream & os,
    const dash::GlobHeapLocalPtr<E_, I_, P_, R_> & it);

private:
  typedef GlobHeapLocalPtr<ElementType, IndexType, PointerType, ReferenceType>
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

  typedef internal::glob_dynamic_mem_bucket_type<size_type, value_type>
    bucket_type;

private:
  typedef typename std::list<bucket_type>
    bucket_list;

  typedef typename bucket_list::iterator
    bucket_iterator;

  typedef typename bucket_list::const_iterator
    const_bucket_iterator;

public:
  template<typename BucketIter>
  GlobHeapLocalPtr(
    const BucketIter & bucket_first,
    const BucketIter & bucket_last,
    index_type         position,
    const BucketIter & bucket_it,
    index_type         bucket_phase)
  : _bucket_first(bucket_first),
    _bucket_last(bucket_last),
    _idx(position),
    _bucket_it(bucket_it),
    _bucket_phase(bucket_phase),
    _is_nullptr(false)
  { }

  template<typename BucketIter>
  GlobHeapLocalPtr(
    const BucketIter & bucket_first,
    const BucketIter & bucket_last,
    index_type         position)
  : _bucket_first(bucket_first),
    _bucket_last(bucket_last),
    _idx(position),
    _bucket_it(bucket_first),
    _bucket_phase(0),
    _is_nullptr(false)
  {
    DASH_LOG_TRACE_VAR("GlobHeapLocalPtr(idx)", position);
#ifdef DASH_ENABLE_TRACE_LOGGING
    index_type bucket_idx = 0;
#endif
    for (_bucket_it  = _bucket_first;
         _bucket_it != _bucket_last; ++_bucket_it) {
      if (position >= _bucket_it->size) {
        position -= _bucket_it->size;
      } else {
        _bucket_phase = position;
        break;
      }
#ifdef DASH_ENABLE_TRACE_LOGGING
      ++bucket_idx;
#endif
    }
    DASH_LOG_TRACE("GlobHeapLocalPtr(idx) >",
                   "bucket:", bucket_idx,
                   "phase:",  _bucket_phase);
  }

  GlobHeapLocalPtr() = default;

  GlobHeapLocalPtr(const self_t & other)
  : _bucket_first(other._bucket_first),
    _bucket_last(other._bucket_last),
    _idx(other._idx),
    _bucket_it(other._bucket_it),
    _bucket_phase(other._bucket_phase),
    _is_nullptr(other._is_nullptr)
  { }

  self_t & operator=(const self_t & rhs)
  {
    if (this != std::addressof(rhs)) {
      _bucket_first = rhs._bucket_first;
      _bucket_last  = rhs._bucket_last;
      _idx          = rhs._idx;
      _bucket_it    = rhs._bucket_it;
      _bucket_phase = rhs._bucket_phase;
      _is_nullptr   = rhs._is_nullptr;
    }
    return *this;
  }

  /**
   * Conversion to const iterator.
   */
  template<typename I_, typename P_, typename R_>
  operator GlobHeapLocalPtr<const value_type, I_, P_, R_>() const
  {
    if (_is_nullptr) {
      return GlobHeapLocalPtr<const value_type, I_, P_, R_>(nullptr);
    }
    return GlobHeapLocalPtr<const value_type, I_, P_, R_>(
             _bucket_first,
             _bucket_last,
             _idx,
             _bucket_it,
             _bucket_phase);
  }

  GlobHeapLocalPtr(std::nullptr_t)
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

  /**
   * Dereference operator.
   */
  reference operator*()
  {
    DASH_ASSERT(!_is_nullptr);
    if (_bucket_phase > _bucket_it->size) {
      DASH_THROW(dash::exception::OutOfRange,
                 "dereferenced position " << _idx << " is out of range: " <<
                 "bucket phase: " << _bucket_phase << ", " <<
                 "bucket size: "  << _bucket_it->size);
    }
    return _bucket_it->lptr[_bucket_phase];
  }

  /**
   * Random access operator.
   */
  reference operator[](index_type offset)
  {
    DASH_ASSERT(!_is_nullptr);
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
               "dereferenced position " << _idx + offset << " " <<
               "is out of range: pointer position: " << _idx << ", " <<
               "offset: " << offset);
  }

  /**
   * Conversion to native pointer.
   *
   * Use with caution: This conversion returns a pointer a that does not
   * iterate over buckets, pointer arithmetics may lead to undefined
   * behaviour.
   */
  explicit operator pointer() const
  {
    DASH_LOG_TRACE("GlobHeapLocalPtr.pointer()");
    pointer lptr = nullptr;
    if (_is_nullptr) {
      DASH_LOG_TRACE("GlobHeapLocalPtr.pointer", "is nullptr");
    } else {
      auto bucket_size = _bucket_it->size;
      // This iterator type represents a local pointer so no bounds checks
      // have to be performed in pointer arithmetics.
      // Moving a pointer to out-of-bounds address is allowed, however
      // dereferencing it will lead to segfault. This is a prerequisite for
      // many common pointer arithmetic use cases.
      // Example:
      //   value = *((globmem.lend() + 2) - 3);
      // is a valid operation and equivalent to
      //   value = *(globmem.lend() + (2 - 3));
      // as it creates a temporary pointer to an address beyond _lend (+2)
      // which is then moved back into valid memory range (-3).
      if (_bucket_it == _bucket_last) {
        DASH_LOG_TRACE("GlobHeapLocalPtr.pointer", "position at lend");
      } else if (_bucket_phase >= bucket_size) {
        DASH_LOG_TRACE("GlobHeapLocalPtr.pointer",
                       "bucket size:",  bucket_size, ",",
                       "bucket phase:", _bucket_phase);
        DASH_LOG_TRACE("GlobHeapLocalPtr.pointer",
                       "note: iterator position out of bounds (lend?)");
      }
      lptr = _bucket_it->lptr + _bucket_phase;
    }
    DASH_LOG_TRACE_VAR("GlobHeapLocalPtr.pointer >", lptr);
    return lptr;
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

  self_t operator++(int)
  {
    auto res = *this;
    increment(1);
    return res;
  }

  self_t operator--(int)
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

  template<typename E_, typename I_, typename P_, typename R_>
  inline bool operator<(const GlobHeapLocalPtr<E_,I_,P_,R_> & other) const
  {
    return (_idx < other._idx);
  }

  template<typename E_, typename I_, typename P_, typename R_>
  inline bool operator<=(const GlobHeapLocalPtr<E_,I_,P_,R_> & other) const
  {
    return (_idx <= other._idx);
  }

  template<typename E_, typename I_, typename P_, typename R_>
  inline bool operator>(const GlobHeapLocalPtr<E_,I_,P_,R_> & other) const
  {
    return (_idx > other._idx);
  }

  template<typename E_, typename I_, typename P_, typename R_>
  inline bool operator>=(const GlobHeapLocalPtr<E_,I_,P_,R_> & other) const
  {
    return (_idx >= other._idx);
  }

  template<typename E_, typename I_, typename P_, typename R_>
  inline bool operator==(const GlobHeapLocalPtr<E_,I_,P_,R_> & other) const
  {
    return (this == std::addressof(other) || _idx == other._idx);
  }

  template<typename E_, typename I_, typename P_, typename R_>
  inline bool operator!=(const GlobHeapLocalPtr<E_,I_,P_,R_> & other) const
  {
    return !(*this == other);
  }

  /**
   * Whether the pointer references an element in local memory space.
   *
   * \return  true
   */
  constexpr bool is_local() const
  {
    return true;
  }

  /**
   * Position of the pointer relative to its referenced memory space.
   */
  index_type pos() const
  {
    return _idx;
  }

private:
  /**
   * Advance pointer by specified position offset.
   */
  void increment(int offset)
  {
    DASH_ASSERT(!_is_nullptr);
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

  /**
   * Decrement pointer by specified position offset.
   */
  void decrement(int offset)
  {
    DASH_ASSERT(!_is_nullptr);
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
  bucket_iterator  _bucket_first;
  bucket_iterator  _bucket_last;
  index_type       _idx           = 0;
  bucket_iterator  _bucket_it;
  index_type       _bucket_phase  = 0;
  bool             _is_nullptr    = false;

}; // class GlobHeapLocalPtr

/**
 * Resolve the number of elements between two local bucket iterators.
 *
 * \complexity  O(1)
 *
 * \ingroup     Algorithms
 */
template<
  typename ElementType,
  typename IndexType,
  class    Pointer,
  class    Reference>
auto distance(
  /// Global iterator to the first position in the global sequence
  const dash::GlobHeapLocalPtr<
          ElementType, IndexType, Pointer, Reference> & first,
  /// Global iterator to the final position in the global sequence
  const dash::GlobHeapLocalPtr<
          ElementType, IndexType, Pointer, Reference> & last)
-> IndexType
{
  return last - first;
}

template<
  typename ElementType,
  typename IndexType,
  class    Pointer,
  class    Reference>
std::ostream & operator<<(
  std::ostream & os,
  const dash::GlobHeapLocalPtr<
          ElementType, IndexType, Pointer, Reference> & it)
{
  std::ostringstream ss;
  ElementType * lptr = static_cast<ElementType *>(it);
  ss << "dash::GlobHeapLocalPtr<"
     << typeid(ElementType).name() << ">"
     << "("
     << "idx:"  << it._idx          << ", "
     << "bp:"   << it._bucket_phase << ", "
     << "lptr:" << lptr
     << ")";
  return operator<<(os, ss.str());
}

} // namespace dash

#endif // DASH__MEMORY__GLOB_HEAP_LOCAL_PTR_H__INCLUDED
