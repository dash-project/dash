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
  GlobPtr<T> _gptr;
  
  template<typename U>
  friend std::ostream & operator<<(
    std::ostream & os,
    const GlobRef<U> & gref);

public:
  /**
   * Default constructor, creates an GlobRef object referencing an element in
   * global memory.
   */
  GlobRef()
  : _gptr(DART_GPTR_NULL) {
  }

  /**
   * Constructor, creates an GlobRef object referencing an element in global
   * memory.
   */
  GlobRef(
    /// Pointer to referenced object in global memory
    GlobPtr<T> & gptr)
  : _gptr(gptr) {
  }

  /**
   * Copy constructor.
   */
  GlobRef(
    /// GlobRef instance to copy.
    const GlobRef<T> & other)
  : _gptr(other._gptr) {
  }

  /**
   * Assignment operator.
   */
  GlobRef<T> & operator=(const GlobRef<T> & other) {
    DASH_LOG_TRACE_VAR("GlobRef.=()", other);
    // This would result in a dart_put:
//  return *this = T(other);
    _gptr = other._gptr;
    return *this;
  }

  friend void swap(GlobRef<T> a, GlobRef<T> b) {
    T temp = (T)a;
    a = b;
    b = temp;
  }
  
  operator T() const {
    DASH_LOG_TRACE("GlobRef.T()", "conversion operator");
    DASH_LOG_TRACE_VAR("GlobRef.T()", _gptr);
    T t;
    dash::get_value(&t, _gptr);
    return t;
  }

  operator GlobPtr<T>() const {
    DASH_LOG_TRACE("GlobRef.GlobPtr()", "conversion operator");
    DASH_LOG_TRACE_VAR("GlobRef.T()", _gptr);
    return GlobPtr<T>(_gptr);
  }

  GlobRef<T> & operator=(const T val) {
    DASH_LOG_TRACE_VAR("GlobRef.=()", val);
    DASH_LOG_TRACE_VAR("GlobRef.=", _gptr);
    // TODO: Clarify if dart-call can be avoided if
    //       _gptr->is_local()
    dash::put_value(val, _gptr);
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
    return _gptr;
  }
#endif
  GlobPtr<T> & gptr() {
    return _gptr;
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
    return _gptr.is_local();
  }

  /**
   * Get a global ref to a member of a certain type at the
   * specified offset
   */
  template<typename MEMTYPE>
  GlobRef<MEMTYPE> member(size_t offs) {
    dart_gptr_t dartptr = _gptr.dart_gptr();    
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

template<typename T>
std::ostream & operator<<(
  std::ostream & os,
  const GlobRef<T> & gref) {
  char buf[100];
  sprintf(buf,
          "(%08X|%04X|%04X|%016X)",
          gref._gptr.dart_gptr().unitid,
          gref._gptr.dart_gptr().segid,
          gref._gptr.dart_gptr().flags,
          gref._gptr.dart_gptr().addr_or_offs.offset);
  os << "dash::GlobRef<" << typeid(T).name() << ">" << buf;
  return os;
}

} // namespace dash

#endif // DASH__GLOBREF_H_
