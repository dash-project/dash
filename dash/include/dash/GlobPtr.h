/* 
 * dash-lib/GlobPtr.h
 *
 * author(s): Karl Fuerlinger, LMU Munich 
 */
/* @DASH_HEADER@ */

#ifndef DASH__GLOB_PTR_H_
#define DASH__GLOB_PTR_H_

#include <cstddef>
#include <iostream>

#include <dash/dart/if/dart.h>
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
  dart_gptr_t m_dartptr;

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
    m_dartptr = p; 
  }
  
  /**
   * Constructor for conversion of std::nullptr_t.
   */
  GlobPtr(std::nullptr_t p) {
    m_dartptr = DART_GPTR_NULL;
  }

  /**
   * Copy constructor.
   */
  GlobPtr(const GlobPtr & other) = default;

  /**
   * Assignment operator.
   */
  GlobPtr<T>& operator=(const GlobPtr & other) = default;

  /**
   * Converts pointer to its underlying global address.
   */
  explicit operator dart_gptr_t() const {
    return m_dartptr;
  }

  /**
   * The pointer's underlying global address.
   */
  dart_gptr_t dartptr() const {
    return m_dartptr;
  }
  
  /**
   * Prefix increment operator.
   */
  GlobPtr<T> & operator++() {
    DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(&m_dartptr, sizeof(T)),
      DART_OK);
    return *this;
  }
  
  /**
   * Postfix increment operator.
   */
  GlobPtr<T> operator++(int) {
    GlobPtr<T> result = *this;
    DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(&m_dartptr, sizeof(T)),
      DART_OK);
    return result;
  }

  /**
   * Pointer increment operator.
   */
  GlobPtr<T> operator+(gptrdiff_t n) const {
    dart_gptr_t gptr = m_dartptr;
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
      dart_gptr_incaddr(&m_dartptr, n * sizeof(T)),
      DART_OK);
    return result;
  }

  /**
   * Prefix decrement operator.
   */
  GlobPtr<T> & operator--() {
    DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(&m_dartptr, -sizeof(T)),
      DART_OK);
    return *this;
  }
  
  /**
   * Postfix decrement operator.
   */
  GlobPtr<T> operator--(int) {
    GlobPtr<T> result = *this;
    DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(&m_dartptr, -sizeof(T)),
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
    return m_dartptr - rhs.m_dart_ptr;
  }

  /**
   * Pointer decrement operator.
   */
  GlobPtr<T> operator-(gptrdiff_t n) const {
    dart_gptr_t gptr = m_dartptr;
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
      dart_gptr_incaddr(&m_dartptr, -(n * sizeof(T))),
      DART_OK);
    return result;
  }

  /**
   * Equality comparison operator.
   */
  bool operator==(const GlobPtr<T>& other) const { 
    return DART_GPTR_EQUAL(m_dartptr, other.m_dartptr);
  }
  
  /**
   * Inequality comparison operator.
   */
  bool operator!=(const GlobPtr<T>& other) const {
    return !DART_GPTR_EQUAL(m_dartptr, other.m_dartptr);
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
    if(is_local()) {
      DASH_ASSERT_RETURNS(
        dart_gptr_getaddr(m_dartptr, &addr),
        DART_OK);
    }
    return static_cast<T*>(addr);
  }

  /**
   * Set the global pointer's associated unit.
   */
  void set_unit(dart_unit_t unit_id) {
    DASH_ASSERT_RETURNS(
      dart_gptr_setunit(&m_dartptr, unit_id),
      DART_OK);
  }

  /**
   * Check whether the global pointer is in the local
   * address space the pointer's associated unit.
   */
  bool is_local() const {
    return m_dartptr.unitid == dash::myid();
  }
};

template<typename T>
std::ostream & operator<<(
  std::ostream & os,
  const GlobPtr<T> & gptr) {
  char buf[100];
  sprintf(buf,
          "(%08X|%04X|%04X|%016X)",
          gptr.m_dartptr.unitid,
          gptr.m_dartptr.segid,
          gptr.m_dartptr.flags,
          gptr.m_dartptr.addr_or_offs.offset);
  os << "dash::GlobPtr<" << typeid(T).name() << ">" << buf;
  return os;
}

} // namespace dash

std::ostream & operator<<(
  std::ostream & os,
  const dart_gptr_t & dartptr);

#endif // DASH__GLOB_PTR_H_
