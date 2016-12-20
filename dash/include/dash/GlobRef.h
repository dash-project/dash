#ifndef DASH__GLOBREF_H_
#define DASH__GLOBREF_H_

#include <dash/GlobMem.h>
#include <dash/Init.h>
#include <dash/algorithm/Operation.h>

namespace dash {

// Forward declaration
template<typename T, class A> class GlobMem;
// Forward declaration
template<typename T, class PatternT> class GlobPtr;
// Forward declaration
template<typename T, class PatternT>
void put_value(const T & newval, const GlobPtr<T, PatternT> & gptr);
// Forward declaration
template<typename T, class PatternT>
void get_value(T* ptr, const GlobPtr<T, PatternT> & gptr);


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
class GlobRef
{
  template<typename U>
  friend std::ostream & operator<<(
    std::ostream & os,
    const GlobRef<U> & gref);

private:

  typedef GlobRef<T>
    self_t;

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
  template<typename PatternT>
  GlobRef(
    /// Pointer to referenced object in global memory
    GlobPtr<T, PatternT> & gptr)
  : GlobRef(gptr.dart_gptr())
  { }

  /**
   * Constructor, creates an GlobRef object referencing an element in global
   * memory.
   */
  template<typename PatternT>
  GlobRef(
    /// Pointer to referenced object in global memory
    const GlobPtr<T, PatternT> & gptr)
  : GlobRef(gptr.dart_gptr())
  { }

  /**
   * Constructor, creates an GlobRef object referencing an element in global
   * memory.
   */
  explicit GlobRef(dart_gptr_t dart_gptr)
  : _gptr(dart_gptr)
  {
    DASH_LOG_TRACE_VAR("GlobRef(dart_gptr_t)", dart_gptr);
  }

  /**
   * Copy constructor.
   */
  GlobRef(
    /// GlobRef instance to copy.
    const GlobRef<T> & other)
  : _gptr(other._gptr)
  { }

  /**
   * Assignment operator.
   */
  GlobRef<T> & operator=(const GlobRef<T> & other)
  {
    DASH_LOG_TRACE_VAR("GlobRef.=()", other);
    // This results in a dart_put, required for STL algorithms like
    // std::copy to work on global ranges.
    // TODO: Not well-defined:
    //       This violates copy semantics, as
    //         GlobRef(const GlobRef & other)
    //       copies the GlobRef instance while
    //         GlobRef=(const GlobRef & other)
    //       puts the value.
    return *this = static_cast<T>(other);
//  _gptr = other._gptr;
//  return *this;
  }

  inline bool operator==(const self_t & other) const noexcept
  {
    return _gptr == other._gptr;
  }

  inline bool operator!=(const self_t & other) const noexcept
  {
    return !(*this == other);
  }

  inline bool operator==(const T & value) const
  {
    return static_cast<T>(*this) == value;
  }

  inline bool operator!=(const T & value) const
  {
    return !(*this == value);
  }

  friend void swap(GlobRef<T> a, GlobRef<T> b) {
    T temp = (T)a;
    a = b;
    b = temp;
  }

  T get() const {
    DASH_LOG_TRACE("T GlobRef.get()", "explicit get");
    DASH_LOG_TRACE_VAR("GlobRef.T()", _gptr);
    T t;
    dart_storage_t ds = dash::dart_storage<T>(1);
    dart_get_blocking(static_cast<void *>(&t), _gptr, ds.nelem, ds.dtype);
    return t;
  }

  operator T() const {
    DASH_LOG_TRACE("GlobRef.T()", "conversion operator");
    DASH_LOG_TRACE_VAR("GlobRef.T()", _gptr);
    T t;
    dart_storage_t ds = dash::dart_storage<T>(1);
    dart_get_blocking(static_cast<void *>(&t), _gptr, ds.nelem, ds.dtype);
    return t;
  }

  void get(T *tptr) const {
    DASH_LOG_TRACE("GlobRef.get(T*)", "explicit get into provided ptr");
    DASH_LOG_TRACE_VAR("GlobRef.T()", _gptr);
    dart_storage_t ds = dash::dart_storage<T>(1);
    dart_get_blocking(static_cast<void *>(tptr), _gptr, ds.nelem, ds.dtype);
  }

  void get(T& tref) const {
    DASH_LOG_TRACE("GlobRef.get(T&)", "explicit get into provided ref");
    DASH_LOG_TRACE_VAR("GlobRef.T()", _gptr);
    dart_storage_t ds = dash::dart_storage<T>(1);
    dart_get_blocking(static_cast<void *>(&tref), _gptr, ds.nelem, ds.dtype);
  }

  void put(T& tref) const {
    DASH_LOG_TRACE("GlobRef.put(T&)", "explicit put of provided ref");
    DASH_LOG_TRACE_VAR("GlobRef.T()", _gptr);
    dart_storage_t ds = dash::dart_storage<T>(1);
    dart_put_blocking(_gptr, static_cast<void *>(&tref), ds.nelem, ds.dtype);
  }

  void put(T* tptr) const {
    DASH_LOG_TRACE("GlobRef.put(T*)", "explicit put of provided ptr");
    DASH_LOG_TRACE_VAR("GlobRef.T()", _gptr);
    dart_storage_t ds = dash::dart_storage<T>(1);
    dart_put_blocking(_gptr, static_cast<void *>(tptr), ds.nelem, ds.dtype);
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
    dart_storage_t ds = dash::dart_storage<T>(1);
    dart_put_blocking(_gptr, static_cast<const void *>(&val), ds.nelem, ds.dtype);
    return *this;
  }

  GlobRef<T> & operator+=(const T& ref)
  {
#if 0
    T add_val = ref;
    T old_val;
    dart_ret_t result = dart_fetch_and_op(
                          _gptr,
                          reinterpret_cast<void *>(&add_val),
                          reinterpret_cast<void *>(&old_val),
                          dash::dart_datatype<T>::value,
                          dash::plus<T>().dart_operation(),
                          dash::Team::All().dart_id());
    dart_flush(_gptr);
#else
    T val  = operator T();
    val   += ref;
    operator=(val);
#endif
    return *this;
  }

  GlobRef<T> & operator-=(const T& ref) {
    T val  = operator T();
    val   -= ref;
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

  GlobRef<T> & operator*=(const T& ref) {
    T val  = operator T();
    val   *= ref;
    operator=(val);
    return *this;
  }

  GlobRef<T> & operator/=(const T& ref) {
    T val  = operator T();
    val   /= ref;
    operator=(val);
    return *this;
  }

  GlobRef<T> & operator^=(const T& ref) {
    T val  = operator T();
    val   ^= ref;
    operator=(val);
    return *this;
  }

#if 0
  // Might lead to unintended behaviour
  GlobPtr<T> operator &() {
    return _gptr;
  }
#endif
  dart_gptr_t dart_gptr() const {
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
    return GlobPtr<T>(_gptr).is_local();
  }

  /**
   * Get a global ref to a member of a certain type at the
   * specified offset
   */
  template<typename MEMTYPE>
  GlobRef<MEMTYPE> member(size_t offs) const {
    dart_gptr_t dartptr = _gptr;
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
    const MEMTYPE P::*mem) const {
    // TODO: Thaaaat ... looks hacky.
    size_t offs = (size_t) &( reinterpret_cast<P*>(0)->*mem);
    return member<MEMTYPE>(offs);
  }

private:

  dart_gptr_t _gptr;

};

template<typename T>
std::ostream & operator<<(
  std::ostream & os,
  const GlobRef<T> & gref) {
  char buf[100];
  sprintf(buf,
          "(%08X|%04X|%04X|%016lX)",
          gref._gptr.unitid,
          gref._gptr.segid,
          gref._gptr.flags,
          gref._gptr.addr_or_offs.offset);
  os << "dash::GlobRef<" << typeid(T).name() << ">" << buf;
  return os;
}

} // namespace dash

#endif // DASH__GLOBREF_H_
