/* 
 * dash-lib/GlobPtr.h
 *
 * author(s): Karl Fuerlinger, LMU Munich 
 */
/* @DASH_HEADER@ */

#ifndef DASH__GLOB_PTR_H_
#define DASH__GLOB_PTR_H_

#include <cstddef>
#include <sstream>
#include <iostream>

#include <dash/dart/if/dart.h>
#include <dash/internal/Logging.h>
#include <dash/Exception.h>
#include <dash/Init.h>

namespace dash {

// Forward-declarations
template<typename T> class GlobRef;
template<typename T> class GlobPtr;
template<typename U>
std::ostream & operator<<(
  std::ostream & os,
  const GlobPtr<U> & it);

template<typename T>
class GlobPtr {
public:
  typedef long long gptrdiff_t;

private:
  dart_gptr_t _dart_gptr;

  template<typename U>
  friend std::ostream & operator<<(
    std::ostream & os,
    const GlobPtr<U> & gptr);

public:
  /**
   * Default constructor, underlying global address is unspecified.
   */
  explicit GlobPtr() = default;

  /**
   * Constructor, specifies underlying global address.
   */
  explicit GlobPtr(dart_gptr_t p) { 
    _dart_gptr = p; 
  }
  
  /**
   * Constructor for conversion of std::nullptr_t.
   */
  GlobPtr(std::nullptr_t p) {
    DASH_LOG_TRACE("GlobPtr()", "nullptr");
    _dart_gptr = DART_GPTR_NULL;
  }

  /**
   * Constructor, creates a new instance of dash::GlobPtr from a global
   * reference.
   */
  GlobPtr(dash::GlobRef<T> & globref)
  : _dart_gptr(globref.gptr().dart_gptr()) {
    DASH_LOG_TRACE("GlobPtr()", "GlobRef<T> globref");
  }

  /**
   * Copy constructor.
   */
  GlobPtr(const GlobPtr & other)
  : _dart_gptr(other._dart_gptr) {
    DASH_LOG_TRACE("GlobPtr()", "GlobPtr<T> other");
  }

  /**
   * Assignment operator.
   */
  GlobPtr<T> & operator=(const GlobPtr & rhs) {
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
  explicit operator dart_gptr_t() const {
    return _dart_gptr;
  }

  /**
   * The pointer's underlying global address.
   */
  dart_gptr_t dart_gptr() const {
    return _dart_gptr;
  }
  
  /**
   * Prefix increment operator.
   */
  GlobPtr<T> & operator++() {
    DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(&_dart_gptr, sizeof(T)),
      DART_OK);
    return *this;
  }
  
  /**
   * Postfix increment operator.
   */
  GlobPtr<T> operator++(int) {
    GlobPtr<T> result = *this;
    DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(&_dart_gptr, sizeof(T)),
      DART_OK);
    return result;
  }

  /**
   * Pointer increment operator.
   */
  GlobPtr<T> operator+(gptrdiff_t n) const {
    dart_gptr_t gptr = _dart_gptr;
    DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(&gptr, n * sizeof(T)),
      DART_OK);
    return GlobPtr<T>(gptr);
  }

  /**
   * Pointer increment operator.
   */
  GlobPtr<T> operator+=(gptrdiff_t n) {
    GlobPtr<T> result = *this;
    DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(&_dart_gptr, n * sizeof(T)),
      DART_OK);
    return result;
  }

  /**
   * Prefix decrement operator.
   */
  GlobPtr<T> & operator--() {
    DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(&_dart_gptr, -sizeof(T)),
      DART_OK);
    return *this;
  }
  
  /**
   * Postfix decrement operator.
   */
  GlobPtr<T> operator--(int) {
    GlobPtr<T> result = *this;
    DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(&_dart_gptr, -sizeof(T)),
      DART_OK);
    return result;
  }

  /**
   * Pointer distance operator.
   * 
   * TODO: Distance between two global pointers is not well-defined yet.
   *       This method is only provided to comply to the pointer concept.
   */
  gptrdiff_t operator-(const GlobPtr<T> & rhs) {
    return _dart_gptr - rhs.m_dart_ptr;
  }

  /**
   * Pointer decrement operator.
   */
  GlobPtr<T> operator-(gptrdiff_t n) const {
    dart_gptr_t gptr = _dart_gptr;
    DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(&gptr, -(n * sizeof(T))),
      DART_OK);
    return GlobPtr<T>(gptr);
  }

  /**
   * Pointer decrement operator.
   */
  GlobPtr<T> operator-=(gptrdiff_t n) {
    GlobPtr<T> result = *this;
    DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(&_dart_gptr, -(n * sizeof(T))),
      DART_OK);
    return result;
  }

  /**
   * Equality comparison operator.
   */
  bool operator==(const GlobPtr<T>& other) const { 
    return DART_GPTR_EQUAL(_dart_gptr, other._dart_gptr);
  }
  
  /**
   * Inequality comparison operator.
   */
  bool operator!=(const GlobPtr<T>& other) const {
    return !DART_GPTR_EQUAL(_dart_gptr, other._dart_gptr);
  }
  
  /**
   * Less comparison operator.
   * 
   * TODO: Distance between two global pointers is not well-defined yet.
   *       This method is only provided to comply to the pointer concept.
   */
  bool operator<(const GlobPtr<T>& other) const {
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
  bool operator<=(const GlobPtr<T>& other) const {
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
  bool operator>(const GlobPtr<T>& other) const {
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
  bool operator>=(const GlobPtr<T>& other) const {
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
  const GlobRef<T> operator[](gptrdiff_t n) const {
    GlobPtr<T> ptr = (*this)+n;
    return GlobRef<T>(ptr);
  }

  /**
   * Subscript assignment operator.
   */
  GlobRef<T> operator[](gptrdiff_t n) {
    GlobPtr<T> ptr = (*this)+n;
    return GlobRef<T>(ptr);
  }

  /**
   * Dereference operator.
   */
  GlobRef<T> operator*() {
    return GlobRef<T>(*this);
  }
  
  /**
   * Conversion operator to local pointer.
   * 
   * \returns  A native pointer to the local element referenced by this
   *           GlobPtr instance, or \c nullptr if the referenced element
   *           is not local to the calling unit.
   */
  explicit operator T*() {
    void *addr = 0;
    DASH_ASSERT_RETURNS(
      dart_gptr_getaddr(_dart_gptr, &addr),
      DART_OK);
    return static_cast<T*>(addr);
  }

  /**
   * Conversion to local pointer.
   * 
   * \returns  A native pointer to the local element referenced by this
   *           GlobPtr instance, or \c nullptr if the referenced element
   *           is not local to the calling unit.
   */
  T * local() {
    void *addr = 0;
    DASH_ASSERT_RETURNS(
      dart_gptr_getaddr(_dart_gptr, &addr),
      DART_OK);
    return static_cast<T*>(addr);
  }

  /**
   * Conversion to local const pointer.
   * 
   * \returns  A native pointer to the local element referenced by this
   *           GlobPtr instance, or \c nullptr if the referenced element
   *           is not local to the calling unit.
   */
  const T * local() const {
    void *addr = 0;
    DASH_ASSERT_RETURNS(
      dart_gptr_getaddr(_dart_gptr, &addr),
      DART_OK);
    return static_cast<const T*>(addr);
  }

  /**
   * Set the global pointer's associated unit.
   */
  void set_unit(dart_unit_t unit_id) {
    DASH_ASSERT_RETURNS(
      dart_gptr_setunit(&_dart_gptr, unit_id),
      DART_OK);
  }

  /**
   * Check whether the global pointer is in the local
   * address space the pointer's associated unit.
   */
  bool is_local() const {
    return _dart_gptr.unitid == dash::myid();
  }
};

template<typename T>
std::ostream & operator<<(
  std::ostream & os,
  const GlobPtr<T> & gptr)
{
  std::ostringstream ss;
  char buf[100];
  sprintf(buf,
          "%08X|%04X|%04X|%016X",
          gptr._dart_gptr.unitid,
          gptr._dart_gptr.segid,
          gptr._dart_gptr.flags,
          gptr._dart_gptr.addr_or_offs.offset);
  ss << "dash::GlobPtr<" << typeid(T).name() << ">(" << buf << ")";
  return operator<<(os, ss.str());
}

} // namespace dash

std::ostream & operator<<(
  std::ostream & os,
  const dart_gptr_t & dartptr);

#endif // DASH__GLOB_PTR_H_
