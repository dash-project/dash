#ifndef DASH__INTERNAL__ALLOCATOR__GLOB_BUCKET_ITER_H__INCLUDED
#define DASH__INTERNAL__ALLOCATOR__GLOB_BUCKET_ITER_H__INCLUDED

#include <dash/dart/if/dart.h>

#include <dash/Types.h>
#include <dash/GlobPtr.h>
#include <dash/Allocator.h>
#include <dash/Team.h>
#include <dash/Onesided.h>

#include <dash/internal/Logging.h>
#include <dash/internal/allocator/GlobDynamicMemTypes.h>
#include <dash/internal/allocator/LocalBucketIter.h>

#include <type_traits>
#include <list>
#include <vector>
#include <iterator>
#include <sstream>
#include <iostream>


namespace dash {

// Forward-declaration
template<
  typename ElementType,
  class    AllocatorType >
class GlobDynamicMem;

namespace internal {

/**
 * Iterator on global buckets. Represents global pointer type.
 */
template<
  typename ElementType,
  class    GlobMemType,
  class    PointerType   = dash::GlobPtr<ElementType>,
  class    ReferenceType = dash::GlobRef<ElementType> >
class GlobBucketIter
: public std::iterator<
           std::random_access_iterator_tag,
           ElementType,
           typename GlobMemType::index_type,
           PointerType,
           ReferenceType >
{
private:
  typedef GlobBucketIter<
            ElementType,
            GlobMemType,
            PointerType,
            ReferenceType>
    self_t;

public:
  typedef typename GlobMemType::index_type                       index_type;
  typedef typename std::make_unsigned<index_type>::type           size_type;

  typedef       ElementType                                      value_type;
  typedef       ReferenceType                                     reference;
  typedef const ReferenceType                               const_reference;
  typedef       PointerType                                         pointer;
  typedef const PointerType                                   const_pointer;

  typedef typename
    std::conditional<
      std::is_const<value_type>::value,
      typename GlobMemType::const_local_pointer,
      typename GlobMemType::local_pointer
    >::type
    local_pointer;

  typedef struct {
    dart_unit_t unit;
    index_type  index;
  } local_index;

protected:
  /// Global memory used to dereference iterated values.
  GlobMemType          * _globmem  = nullptr;
  /// Pointer to first element in local data space.
  local_pointer          _lbegin;
  /// Current position of the iterator in global canonical index space.
  index_type             _idx      = 0;
  /// Maximum position allowed for this iterator.
  index_type             _max_idx  = 0;
  /// Unit id of the active unit
  dart_unit_t            _myid;

public:
  /**
   * Default constructor.
   */
  GlobBucketIter()
  : _globmem(nullptr),
    _idx(0),
    _max_idx(0),
    _myid(dash::myid())
  {
    DASH_LOG_TRACE_VAR("GlobBucketIter()", _idx);
    DASH_LOG_TRACE_VAR("GlobBucketIter()", _max_idx);
  }

  /**
   * Constructor, creates a global iterator on global memory following
   * logical storage order.
   */
  GlobBucketIter(
    GlobMemType * gmem,
	  index_type    position = 0)
  : _globmem(gmem),
    _lbegin(_globmem->lbegin()),
    _idx(position),
    _max_idx(gmem->size() - 1),
    _myid(dash::myid())
  {
    DASH_LOG_TRACE_VAR("GlobBucketIter(gmem,idx,abs)", _idx);
    DASH_LOG_TRACE_VAR("GlobBucketIter(gmem,idx,abs)", _max_idx);
  }

  /**
   * Copy constructor.
   */
  GlobBucketIter(
    const self_t & other) = default;

  /**
   * Assignment operator.
   */
  self_t & operator=(
    const self_t & other) = default;

  /**
   * Type conversion operator to \c GlobPtr.
   *
   * \return  A global reference to the element at the iterator's position
   */
  operator PointerType() const
  {
    DASH_LOG_TRACE_VAR("GlobBucketIter.GlobPtr", _idx);
    index_type idx    = _idx;
    index_type offset = 0;
    DASH_LOG_TRACE_VAR("GlobBucketIter.GlobPtr", _max_idx);
    // TODO
    local_index local_pos;

    // Global index to local index and unit:
    DASH_LOG_TRACE("GlobBucketIter.GlobPtr",
                   "unit:",        local_pos.unit,
                   "local index:", local_pos.index);
    // Create global pointer from unit and local offset,
    // using type conversion operator GlobBucketIter.GlobPtr():
    PointerType gptr = _globmem->at(
                         local_pos.unit,
                         local_pos.index);
    return gptr + offset;
  }

  /**
   * Explicit conversion to \c dart_gptr_t.
   *
   * \return  A DART global pointer to the element at the iterator's
   *          position
   */
  dart_gptr_t dart_gptr() const
  {
    DASH_LOG_TRACE_VAR("GlobBucketIter.dart_gptr()", _idx);
    index_type idx    = _idx;
    index_type offset = 0;
    // TODO
    local_index local_pos;

    // Global index to local index and unit:
    DASH_LOG_TRACE("GlobBucketIter.dart_gptr",
                   "unit:",        local_pos.unit,
                   "local index:", local_pos.index);
    // Global pointer to element at given position,
    // using type conversion operator GlobBucketIter.GlobPtr():
    auto gptr = _globmem->at(
                   local_pos.unit,
                   local_pos.index);
    dart_gptr_t dart_gptr = (gptr + offset).dart_gptr();
    DASH_LOG_TRACE_VAR("GlobBucketIter.dart_gptr >", dart_gptr);
    return dart_gptr;
  }

  /**
   * Dereference operator.
   *
   * \return  A global reference to the element at the iterator's position.
   */
  reference operator*() const
  {
    DASH_LOG_TRACE("GlobBucketIter.*", _idx);
    index_type idx = _idx;
    // Global index to local index and unit:
    local_index local_pos;
    // TODO

    DASH_LOG_TRACE_VAR("GlobBucketIter.*", local_pos.unit);
    DASH_LOG_TRACE_VAR("GlobBucketIter.*", local_pos.index);
    // Global reference to element at given position, using dereference
    // operator GlobBucketIter.*():
    return *_globmem->at(local_pos.unit,
                         local_pos.index);
  }

  /**
   * Subscript operator, returns global reference to element at given
   * global index.
   */
  reference operator[](
    /// The global position of the element
    index_type g_index) const
  {
    DASH_LOG_TRACE("GlobBucketIter.[]", g_index);
    index_type idx = g_index;
    // Global index to local index and unit:
    local_index local_pos;
    // TODO

    DASH_LOG_TRACE_VAR("GlobBucketIter.[]", local_pos.unit);
    DASH_LOG_TRACE_VAR("GlobBucketIter.[]", local_pos.index);
    // Global reference to element at given position, using dereference
    // operator GlobBucketIter.*():
    return *_globmem->at(local_pos.unit,
                         local_pos.index);
  }

  /**
   * Checks whether the element referenced by this global iterator is in
   * the calling unit's local memory.
   */
  inline bool is_local() const
  {
    return (_myid == lpos().unit);
  }

  /**
   * Conversion to local bucket iterator.
   */
  local_pointer local() const
  {
    DASH_LOG_TRACE_VAR("GlobBucketIter.local=()", _idx);
    index_type idx    = _idx;
    index_type offset = 0;
    DASH_LOG_TRACE_VAR("GlobBucketIter.local=", _max_idx);
    // TODO
    return nullptr;
  }

  /**
   * Position of the iterator in global index space.
   */
  inline index_type pos() const
  {
    return _idx;
  }

  /**
   * Unit and local offset at the iterator's position.
   */
  inline local_index lpos() const
  {
    DASH_LOG_TRACE_VAR("GlobBucketIter.lpos()", _idx);
    index_type idx    = _idx;
    index_type offset = 0;
    local_index local_pos;
    // TODO
    return local_pos;
  }

  /**
   * The instance of \c GlobMem used by this iterator to resolve addresses
   * in global memory.
   */
  inline const GlobMemType & globmem() const
  {
    return *_globmem;
  }

  /**
   * The instance of \c GlobMem used by this iterator to resolve addresses
   * in global memory.
   */
  inline GlobMemType & globmem()
  {
    return *_globmem;
  }

  /**
   * Prefix increment operator.
   */
  inline self_t & operator++()
  {
    ++_idx;
    return *this;
  }

  /**
   * Postfix increment operator.
   */
  inline self_t operator++(int)
  {
    self_t result = *this;
    ++_idx;
    return result;
  }

  /**
   * Prefix decrement operator.
   */
  inline self_t & operator--()
  {
    --_idx;
    return *this;
  }

  /**
   * Postfix decrement operator.
   */
  inline self_t operator--(int)
  {
    self_t result = *this;
    --_idx;
    return result;
  }

  inline self_t & operator+=(index_type n)
  {
    _idx += n;
    return *this;
  }

  inline self_t & operator-=(index_type n)
  {
    _idx -= n;
    return *this;
  }

  inline self_t operator+(index_type n) const
  {
    self_t res(
      _globmem,
      _idx + static_cast<index_type>(n));
    return res;
  }

  inline self_t operator-(index_type n) const
  {
    self_t res(
      _globmem,
      _idx - static_cast<index_type>(n));
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
    return _idx == other._idx;
  }

  inline bool operator!=(const self_t & other) const
  {
    return _idx != other._idx;
  }

}; // class GlobBucketIter

/**
 * Resolve the number of elements between two global bucket iterators.
 *
 * \complexity  O(1)
 *
 * \ingroup     Algorithms
 */
template<
  typename ElementType,
  class    GlobMemType,
  class    Pointer,
  class    Reference>
auto distance(
  /// Global iterator to the first position in the global sequence
  const dash::internal::GlobBucketIter<
          ElementType, GlobMemType, Pointer, Reference> & first,
  /// Global iterator to the final position in the global sequence
  const dash::internal::GlobBucketIter<
          ElementType, GlobMemType, Pointer, Reference> & last)
-> typename GlobMemType::index_type
{
  return last - first;
}

#if 0
template<
  typename ElementType,
  class    GlobMemType,
  class    Pointer,
  class    Reference>
std::ostream & operator<<(
  std::ostream & os,
  const dash::internal::GlobBucketIter<
          ElementType, GlobMemType, Pointer, Reference> & it)
{
  std::ostringstream ss;
  auto ptr = it.dart_gptr();
  ss << "dash::internal::GlobBucketIter<"
     << typeid(ElementType).name() << ">("
     << "idx:"  << it.pos() << ", "
     << "gptr:" << ptr      << ")";
  return operator<<(os, ss.str());
}
#endif

} // namespace internal
} // namespace dash

#endif // DASH__INTERNAL__ALLOCATOR__GLOB_BUCKET_ITER_H__INCLUDED
