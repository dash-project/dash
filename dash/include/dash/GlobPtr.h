#ifndef DASH__GLOB_PTR_H_
#define DASH__GLOB_PTR_H_

#include <cstddef>
#include <sstream>
#include <iostream>

#include <dash/dart/if/dart.h>
#include <dash/internal/Logging.h>
#include <dash/Pattern.h>
#include <dash/Exception.h>
#include <dash/Init.h>

std::ostream & operator<<(
  std::ostream      & os,
  const dart_gptr_t & dartptr);

bool operator==(
  const dart_gptr_t & lhs,
  const dart_gptr_t & rhs);

bool operator!=(
  const dart_gptr_t & lhs,
  const dart_gptr_t & rhs);

namespace dash {

// Forward-declarations
template<typename T> class GlobRef;
template<typename T, class PatternT> class GlobPtr;
template<typename T, class PatternT>
std::ostream & operator<<(
  std::ostream               & os,
  const GlobPtr<T, PatternT> & it);

/**
 * Pointer in global memory space.
 *
 * \note
 * For performance considerations, the iteration space of \c GlobPtr
 * is restricted to *local address space*.
 * If an instance of \c GlobPtr is incremented past the last address
 * of the underlying local memory range, it is not advanced to the
 * next unit's local address range.
 * Iteration over unit borders is implemented by \c GlobalIterator
 * types which perform a mapping of local and global index sets
 * specified by a \c Pattern.
 *
 * \see GlobIter
 * \see GlobViewIter
 * \see DashPatternConcept
 *
 */
template<
  typename ElementType,
  class    PatternType  = Pattern<1> >
class GlobPtr
{
private:
  typedef GlobPtr<ElementType, PatternType> self_t;
  typedef typename PatternType::index_type  IndexType;

public:
  typedef IndexType   index_type;
  typedef PatternType pattern_type;
  typedef IndexType   gptrdiff_t;

public:
  /// Convert GlobPtr<T, PatternType> to GlobPtr<U, PatternType>.
  template<typename U>
  struct rebind {
    typedef GlobPtr<U, PatternType> other;
  };

private:
  dart_gptr_t _dart_gptr;

public:
  template<typename T, class PatternT>
  friend std::ostream & operator<<(
    std::ostream               & os,
    const GlobPtr<T, PatternT> & gptr);

public:
  /**
   * Default constructor, underlying global address is unspecified.
   */
  explicit GlobPtr() = default;

  /**
   * Constructor, specifies underlying global address.
   */
  explicit GlobPtr(dart_gptr_t gptr)
  {
    DASH_LOG_TRACE_VAR("GlobPtr(dart_gptr_t)", gptr);
    _dart_gptr = gptr;
  }

  /**
   * Constructor for conversion of std::nullptr_t.
   */
  GlobPtr(std::nullptr_t p)
  {
    DASH_LOG_TRACE("GlobPtr()", "nullptr");
    _dart_gptr = DART_GPTR_NULL;
  }

  /**
   * Constructor, creates a new instance of dash::GlobPtr from a global
   * reference.
   */
  explicit GlobPtr(const dash::GlobRef<ElementType> & globref)
  : _dart_gptr(globref.dart_gptr())
  {
    DASH_LOG_TRACE("GlobPtr()", "GlobRef<T> globref");
  }

  /**
   * Copy constructor.
   */
  GlobPtr(const self_t & other)
  : _dart_gptr(other._dart_gptr)
  {
    DASH_LOG_TRACE("GlobPtr()", "GlobPtr<T> other");
  }

  /**
   * Assignment operator.
   */
  self_t & operator=(const self_t & rhs)
  {
    DASH_LOG_TRACE("GlobPtr.=()", "GlobPtr<T> rhs");
    DASH_LOG_TRACE_VAR("GlobPtr.=", rhs);
    if (this != &rhs) {
      _dart_gptr = rhs._dart_gptr;
    }
    DASH_LOG_TRACE("GlobPtr.= >");
    return *this;
  }

  /**
   * Converts pointer to its underlying global address.
   */
  explicit operator dart_gptr_t() const
  {
    return _dart_gptr;
  }

  /**
   * Converts pointer to its referenced native pointer or
   * \c nullptr if the \c GlobPtr does not point to a local
   * address.
   */
  explicit operator ElementType*() const
  {
    return local();
  }

  /**
   * The pointer's underlying global address.
   */
  dart_gptr_t dart_gptr() const
  {
    return _dart_gptr;
  }

  /**
   * Prefix increment operator.
   */
  self_t & operator++()
  {
    DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(&_dart_gptr, sizeof(ElementType)),
      DART_OK);
    return *this;
  }

  /**
   * Postfix increment operator.
   */
  self_t operator++(int)
  {
    self_t result = *this;
    DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(&_dart_gptr, sizeof(ElementType)),
      DART_OK);
    return result;
  }

  /**
   * Pointer increment operator.
   */
  self_t operator+(gptrdiff_t n) const
  {
    dart_gptr_t gptr = _dart_gptr;
    DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(&gptr, n * sizeof(ElementType)),
      DART_OK);
    return self_t(gptr);
  }

  /**
   * Pointer increment operator.
   */
  self_t operator+=(gptrdiff_t n)
  {
    self_t result = *this;
    DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(&_dart_gptr, n * sizeof(ElementType)),
      DART_OK);
    return result;
  }

  /**
   * Prefix decrement operator.
   */
  self_t & operator--()
  {
    DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(&_dart_gptr, -sizeof(ElementType)),
      DART_OK);
    return *this;
  }

  /**
   * Postfix decrement operator.
   */
  self_t operator--(int)
  {
    self_t result = *this;
    DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(&_dart_gptr, -sizeof(ElementType)),
      DART_OK);
    return result;
  }

  /**
   * Pointer distance operator.
   *
   * TODO: Distance between two global pointers is not well-defined yet.
   *       This method is only provided to comply to the pointer concept.
   */
  index_type operator-(const self_t & rhs)
  {
    return _dart_gptr - rhs.m_dart_ptr;
  }

  /**
   * Pointer decrement operator.
   */
  self_t operator-(index_type n) const
  {
    dart_gptr_t gptr = _dart_gptr;
    DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(&gptr, -(n * sizeof(ElementType))),
      DART_OK);
    return self_t(gptr);
  }

  /**
   * Pointer decrement operator.
   */
  self_t operator-=(index_type n)
  {
    self_t result = *this;
    DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(&_dart_gptr, -(n * sizeof(ElementType))),
      DART_OK);
    return result;
  }

  /**
   * Equality comparison operator.
   */
  bool operator==(const self_t & other) const
  {
    return DART_GPTR_EQUAL(_dart_gptr, other._dart_gptr);
  }

  /**
   * Inequality comparison operator.
   */
  bool operator!=(const self_t & other) const
  {
    return !(*this == other);
  }

  /**
   * Less comparison operator.
   *
   * TODO: Distance between two global pointers is not well-defined yet.
   *       This method is only provided to comply to the pointer concept.
   */
  bool operator<(const self_t & other) const
  {
    return (
      ( _dart_gptr.unitid <  other._dart_gptr.unitid )
        ||
      ( _dart_gptr.unitid == other._dart_gptr.unitid
        &&
        ( _dart_gptr.segid <  other._dart_gptr.segid
          ||
        ( _dart_gptr.segid == other._dart_gptr.segid
          &&
          _dart_gptr.addr_or_offs.offset <
          other._dart_gptr.addr_or_offs.offset ) )
      )
    );
  }

  /**
   * Less-equal comparison operator.
   *
   * TODO: Distance between two global pointers is not well-defined yet.
   *       This method is only provided to comply to the pointer concept.
   */
  bool operator<=(const self_t & other) const
  {
    return (
      ( _dart_gptr.unitid <  other._dart_gptr.unitid )
        ||
      ( _dart_gptr.unitid == other._dart_gptr.unitid
        &&
        ( _dart_gptr.segid <  other._dart_gptr.segid
          ||
        ( _dart_gptr.segid == other._dart_gptr.segid
          &&
          _dart_gptr.addr_or_offs.offset <=
          other._dart_gptr.addr_or_offs.offset ) )
      )
    );
  }

  /**
   * Greater comparison operator.
   *
   * TODO: Distance between two global pointers is not well-defined yet.
   *       This method is only provided to comply to the pointer concept.
   */
  bool operator>(const self_t & other) const
  {
    return (
      ( _dart_gptr.unitid >  other._dart_gptr.unitid )
        ||
      ( _dart_gptr.unitid == other._dart_gptr.unitid
        &&
        ( _dart_gptr.segid >  other._dart_gptr.segid
          ||
        ( _dart_gptr.segid == other._dart_gptr.segid
          &&
          _dart_gptr.addr_or_offs.offset >
          other._dart_gptr.addr_or_offs.offset ) )
      )
    );
  }

  /**
   * Greater-equal comparison operator.
   *
   * TODO: Distance between two global pointers is not well-defined yet.
   *       This method is only provided to comply to the pointer concept.
   */
  bool operator>=(const self_t & other) const
  {
    return (
      ( _dart_gptr.unitid >  other._dart_gptr.unitid )
        ||
      ( _dart_gptr.unitid == other._dart_gptr.unitid
        &&
        ( _dart_gptr.segid >  other._dart_gptr.segid
          ||
        ( _dart_gptr.segid == other._dart_gptr.segid
          &&
          _dart_gptr.addr_or_offs.offset >=
          other._dart_gptr.addr_or_offs.offset ) )
      )
    );
  }

  /**
   * Subscript operator.
   */
  const GlobRef<ElementType> operator[](gptrdiff_t n) const
  {
    self_t ptr = (*this)+n;
    return GlobRef<ElementType>(ptr);
  }

  /**
   * Subscript assignment operator.
   */
  GlobRef<ElementType> operator[](gptrdiff_t n)
  {
    self_t ptr = (*this)+n;
    return GlobRef<ElementType>(ptr);
  }

  /**
   * Dereference operator.
   */
  GlobRef<ElementType> operator*()
  {
    return GlobRef<ElementType>(*this);
  }

  /**
   * Conversion operator to local pointer.
   *
   * \returns  A native pointer to the local element referenced by this
   *           GlobPtr instance, or \c nullptr if the referenced element
   *           is not local to the calling unit.
   */
  explicit operator ElementType*() {
    void *addr = 0;
    DASH_ASSERT_RETURNS(
      dart_gptr_getaddr(_dart_gptr, &addr),
      DART_OK);
    return static_cast<ElementType*>(addr);
  }

  /**
   * Conversion to local pointer.
   *
   * \returns  A native pointer to the local element referenced by this
   *           GlobPtr instance, or \c nullptr if the referenced element
   *           is not local to the calling unit.
   */
  ElementType * local() {
    void *addr = 0;
    DASH_ASSERT_RETURNS(
      dart_gptr_getaddr(_dart_gptr, &addr),
      DART_OK);
    return static_cast<ElementType*>(addr);
  }

  /**
   * Conversion to local const pointer.
   *
   * \returns  A native pointer to the local element referenced by this
   *           GlobPtr instance, or \c nullptr if the referenced element
   *           is not local to the calling unit.
   */
  const ElementType * local() const {
    void *addr = 0;
    DASH_ASSERT_RETURNS(
      dart_gptr_getaddr(_dart_gptr, &addr),
      DART_OK);
    return static_cast<const ElementType*>(addr);
  }

  /**
   * Set the global pointer's associated unit.
   */
  void set_unit(global_unit_t unit_id) {
    DASH_ASSERT_RETURNS(
      dart_gptr_setunit(&_dart_gptr, unit_id),
      DART_OK);
  }

  /**
   * Check whether the global pointer is in the local
   * address space the pointer's associated unit.
   */
  bool is_local() const {
    return _dart_gptr.unitid == dash::Team::GlobalUnitID();
  }
};

template<typename T, class PatternT>
std::ostream & operator<<(
  std::ostream               & os,
  const GlobPtr<T, PatternT> & gptr)
{
  std::ostringstream ss;
  char buf[100];
  sprintf(buf,
          "%08X|%04X|%04X|%016lX",
          gptr._dart_gptr.unitid,
          gptr._dart_gptr.segid,
          gptr._dart_gptr.flags,
          gptr._dart_gptr.addr_or_offs.offset);
  ss << "dash::GlobPtr<" << typeid(T).name() << ">(" << buf << ")";
  return operator<<(os, ss.str());
}

} // namespace dash

#endif // DASH__GLOB_PTR_H_
