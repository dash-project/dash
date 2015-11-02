#ifndef DASH__GLOBREF_H_
#define DASH__GLOBREF_H_

#include <dash/GlobMem.h>
#include <dash/Init.h>

namespace dash {

// Forward declaration
template<typename T> class GlobMem;
// Forward declaration
template<typename T> class GlobPtr;
// Forward declaration
template<typename T>
void put_value(const T & newval, const GlobPtr<T> & gptr);
// Forward declaration
template<typename T>
void get_value(T* ptr, const GlobPtr<T> & gptr);


template<typename T>
struct has_subscript_operator
{
  typedef char (& yes)[1];
  typedef char (& no)[2];
  
  template <typename C> static yes check(decltype(&C::operator[]));
  template <typename> static no check(...);
  
  static bool const value = sizeof(check<T>(0)) == sizeof(yes);
};

template<typename T>
class GlobRef {
private:
  GlobPtr<T> m_gptr;
  
public:
  /**
   * Default constructor, creates an GlobRef object referencing an element in
   * global memory.
   */
  GlobRef()
  : m_gptr(DART_GPTR_NULL) {
  }

  /**
   * Constructor, creates an GlobRef object referencing an element in global
   * memory.
   */
  GlobRef(
    /// Pointer to referenced object in global memory
    GlobPtr<T> & gptr)
  : m_gptr(gptr) {
  }

  /**
   * Copy constructor.
   */
  GlobRef(
    /// GlobRef instance to copy.
    const GlobRef<T> & other)
  : m_gptr(other.m_gptr) {
  }

  /**
   * Assignment operator.
   */
  GlobRef<T> & operator=(const GlobRef<T> & other) {
    // This would result in a dart_put:
//  return *this = T(other);
    m_gptr = other.m_gptr;
    return *this;
  }

  friend void swap(GlobRef<T> a, GlobRef<T> b) {
    T temp = (T)a;
    a = b;
    b = temp;
  }
  
  operator T() const {
    T t;
    dash::get_value(&t, m_gptr);
    return t;
  }

  GlobRef<T> & operator=(const T val) {
    DASH_LOG_TRACE_VAR("GlobRef.=()", val);
    DASH_LOG_TRACE_VAR("GlobRef.=", m_gptr);
    // TODO: Clarify if dart-call can be avoided if
    //       m_gptr->is_local()
    dash::put_value(val, m_gptr);
    return *this;
  }

  GlobRef<T> & operator+=(const T& ref) {
    T val = operator T();
    val += ref;
    operator=(val);
    return *this;
  }

  GlobRef<T> & operator-=(const T& ref) {
    T val = operator T();
    val -= ref;
    operator=(val);
    return *this;
  }

  GlobRef<T> & operator++() {
    T val = operator T();
    ++val;
    operator=(val);
    return *this;
  }

  GlobRef<T> operator++(int) {
    GlobRef<T> result = *this;
    T val = operator T();
    ++val;
    operator=(val);
    return result;
  }

  GlobRef<T> & operator--() {
    T val = operator T();
    --val;
    operator=(val);
    return *this;
  }

  GlobRef<T> operator--(int) {
    GlobRef<T> result = *this;
    T val = operator T();
    --val;
    operator=(val);
    return result;
  }

#if 0
  // Might lead to unintended behaviour
  GlobPtr<T> operator &() {
    return m_gptr;
  }
#endif
  GlobPtr<T> & gptr() {
    return m_gptr;
  }

#if 0
  template<
    typename X=T, 
	  typename std::enable_if<has_subscript_operator<X>::value, int>::type
      * ptr = nullptr>
  auto operator[](size_t pos) -> 
    typename std::result_of<decltype(&T::operator[])(T, size_t)>::type
  {
    T val = operator T();
    return val[pos];
  }
#endif

  /**
   * Checks whether the globally referenced element is in
   * the calling unit's local memory.
   */
  bool is_local() const {
    return m_gptr.is_local();
  }

  /**
   * Get a global ref to a member of a certain type at the
   * specified offset
   */
  template<typename MEMTYPE>
  GlobRef<MEMTYPE> member(size_t offs) {
    dart_gptr_t dartptr = m_gptr.dart_gptr();    
    DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(&dartptr, offs),
      DART_OK);
    GlobPtr<MEMTYPE> gptr(dartptr);
    return GlobRef<MEMTYPE>(gptr);
  }

  /**
   * Get the member via pointer to member
   */
  template<class MEMTYPE, class P=T>
  GlobRef<MEMTYPE> member(
    const MEMTYPE P::*mem) {
    // TODO: Thaaaat ... looks hacky.
    size_t offs = (size_t) &( reinterpret_cast<P*>(0)->*mem);
    return member<MEMTYPE>(offs);
  }
};

} // namespace dash

#endif // DASH__GLOBREF_H_
