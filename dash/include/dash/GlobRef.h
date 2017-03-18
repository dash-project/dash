#ifndef DASH__GLOBREF_H_
#define DASH__GLOBREF_H_

#include <dash/GlobStaticHeap.h>
#include <dash/Init.h>
#include <dash/Meta.h>


namespace dash {

// Forward declarations
template<typename T, class A> class GlobStaticHeap;
template<typename T> class GlobConstPtr;
template<typename T, class MemSpaceT> class GlobPtr;

#if 0
template<typename T>
void put_value(const T & newval, const GlobConstPtr<T> & gptr);
template<typename T, class MemSpaceT>
void put_value(const T & newval, const GlobPtr<T, MemSpaceT> & gptr);

template<typename T>
void get_value(T* ptr, const GlobConstPtr<T> & gptr);
template<typename T, class MemSpaceT>
void get_value(T* ptr, const GlobPtr<T, MemSpaceT> & gptr);
#endif

template<typename T>
struct has_subscript_operator
{
  typedef char (& yes)[1];
  typedef char (& no)[2];

  template <typename C> static yes check(decltype(&C::operator[]));
  template <typename>   static no  check(...);

  static bool const value = sizeof(check<T>(0)) == sizeof(yes);
};

template<typename T>
class GlobRef
{
  template<typename U>
  friend std::ostream & operator<<(
    std::ostream & os,
    const GlobRef<U> & gref);

  template <
    typename ElementT >
  friend class GlobRef;
  
  typedef typename std::remove_const<T>::type
    nonconst_value_type;
public:
  typedef T                 value_type;

  typedef GlobRef<const T>  const_type;
  
private:
  typedef GlobRef<T>
    self_t;
  typedef GlobRef<const T>
    self_const_t;

private:
  dart_gptr_t _gptr;

public:
  /**
   * Reference semantics forbid declaration without definition.
   */
  GlobRef() = delete;

  /**
   * Constructor, creates an GlobRef object referencing an element in global
   * memory.
   */
  template<class ElementT, class MemSpaceT>
  explicit constexpr GlobRef(
    /// Pointer to referenced object in global memory
    GlobPtr<ElementT, MemSpaceT> & gptr)
  : GlobRef(gptr.dart_gptr())
  { }

  /**
   * Constructor, creates an GlobRef object referencing an element in global
   * memory.
   */
  template<class ElementT>
  explicit constexpr GlobRef(
    /// Pointer to referenced object in global memory
    GlobConstPtr<ElementT> & gptr)
  : GlobRef(gptr.dart_gptr())
  { }

  /**
   * Constructor, creates an GlobRef object referencing an element in global
   * memory.
   */
  template<class ElementT, class MemSpaceT>
  explicit constexpr GlobRef(
    /// Pointer to referenced object in global memory
    const GlobPtr<ElementT, MemSpaceT> & gptr)
  : GlobRef(gptr.dart_gptr())
  { }

  /**
   * Constructor, creates an GlobRef object referencing an element in global
   * memory.
   */
  template<class ElementT>
  explicit constexpr GlobRef(
    /// Pointer to referenced object in global memory
    const GlobConstPtr<ElementT> & gptr)
  : GlobRef(gptr.dart_gptr())
  { }

  /**
   * Constructor, creates an GlobRef object referencing an element in global
   * memory.
   */
  explicit constexpr GlobRef(dart_gptr_t dart_gptr)
  : _gptr(dart_gptr)
  { }

  /**
   * Like native references, global reference types cannot be copied.
   *
   * Default definition of copy constructor would conflict with semantics
   * of \c operator=(const self_t &).
   */
  GlobRef(const self_t & other) = delete;
 
  /**
   * Unlike native reference types, global reference types are moveable.
   */
  GlobRef(self_t && other)      = default;

  /**
   * Value-assignment operator.
   */
  GlobRef<T> & operator=(const T val) {
    set(val);
    return *this;
  }

  /**
   * Assignment operator.
   */
  GlobRef<T> & operator=(const self_t & other)
  {
    set(static_cast<T>(other));
    return *this;
  }

  /**
   * Assignment operator.
   */
  template <typename GlobRefOrElementT>
  GlobRef<T> & operator=(GlobRefOrElementT && other)
  {
    set(std::forward<GlobRefOrElementT>(other));
    return *this;
  }

  operator nonconst_value_type() const {
    DASH_LOG_TRACE("GlobRef.T()", "conversion operator");
    DASH_LOG_TRACE_VAR("GlobRef.T()", _gptr);
    nonconst_value_type t;
    dart_storage_t ds = dash::dart_storage<T>(1);
    dart_get_blocking(static_cast<void *>(&t), _gptr, ds.nelem, ds.dtype);
    DASH_LOG_TRACE_VAR("GlobRef.T >", _gptr);
    return t;
  }

  template <class GlobRefT>
  constexpr bool operator==(const GlobRefT & other) const noexcept
  {
    return _gptr == other._gptr;
  }

  template <class GlobRefT>
  constexpr bool operator!=(const GlobRefT & other) const noexcept
  {
    return !(*this == other);
  }

  constexpr bool operator==(const nonconst_value_type & value) const noexcept
  {
    return static_cast<T>(*this) == value;
  }

  constexpr bool operator!=(const nonconst_value_type & value) const noexcept
  {
    return !(*this == value);
  }

  friend void swap(GlobRef<T> a, GlobRef<T> b) {
    nonconst_value_type temp = static_cast<nonconst_value_type>(a);
    a = b;
    b = temp;
  }

  void set(const T & val) {
    DASH_LOG_TRACE_VAR("GlobRef.set()", val);
    DASH_LOG_TRACE_VAR("GlobRef.set", _gptr);
    // TODO: Clarify if dart-call can be avoided if
    //       _gptr->is_local()
    dart_storage_t ds = dash::dart_storage<T>(1);
    dart_put_blocking(
        _gptr, static_cast<const void *>(&val), ds.nelem, ds.dtype);
    DASH_LOG_TRACE_VAR("GlobRef.set >", _gptr);
  }

  nonconst_value_type get() const {
    DASH_LOG_TRACE("T GlobRef.get()", "explicit get");
    DASH_LOG_TRACE_VAR("GlobRef.T()", _gptr);
    nonconst_value_type t;
    dart_storage_t ds = dash::dart_storage<T>(1);
    dart_get_blocking(static_cast<void *>(&t), _gptr, ds.nelem, ds.dtype);
    return t;
  }

  void get(nonconst_value_type *tptr) const {
    DASH_LOG_TRACE("GlobRef.get(T*)", "explicit get into provided ptr");
    DASH_LOG_TRACE_VAR("GlobRef.T()", _gptr);
    dart_storage_t ds = dash::dart_storage<T>(1);
    dart_get_blocking(static_cast<void *>(tptr), _gptr, ds.nelem, ds.dtype);
  }

  void get(nonconst_value_type& tref) const {
    DASH_LOG_TRACE("GlobRef.get(T&)", "explicit get into provided ref");
    DASH_LOG_TRACE_VAR("GlobRef.T()", _gptr);
    dart_storage_t ds = dash::dart_storage<T>(1);
    dart_get_blocking(static_cast<void *>(&tref), _gptr, ds.nelem, ds.dtype);
  }

  void put(nonconst_value_type& tref) const {
    DASH_LOG_TRACE("GlobRef.put(T&)", "explicit put of provided ref");
    DASH_LOG_TRACE_VAR("GlobRef.T()", _gptr);
    dart_storage_t ds = dash::dart_storage<T>(1);
    dart_put_blocking(_gptr, static_cast<void *>(&tref), ds.nelem, ds.dtype);
  }

  void put(nonconst_value_type* tptr) const {
    DASH_LOG_TRACE("GlobRef.put(T*)", "explicit put of provided ptr");
    DASH_LOG_TRACE_VAR("GlobRef.T()", _gptr);
    dart_storage_t ds = dash::dart_storage<T>(1);
    dart_put_blocking(_gptr, static_cast<void *>(tptr), ds.nelem, ds.dtype);
  }

  GlobRef<T> & operator+=(const nonconst_value_type& ref) {
  #if 0
    // TODO: Alternative implementation, possibly more efficient:
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
    nonconst_value_type val  = operator nonconst_value_type();
    val   += ref;
    operator=(val);
  #endif
    return *this;
  }

  GlobRef<T> & operator-=(const nonconst_value_type& ref) {
    nonconst_value_type val  = operator nonconst_value_type();
    val   -= ref;
    operator=(val);
    return *this;
  }

  GlobRef<T> & operator++() {
    nonconst_value_type val = operator nonconst_value_type();
    ++val;
    operator=(val);
    return *this;
  }

  GlobRef<T> operator++(int) {
    GlobRef<T> result = *this;
    nonconst_value_type val = operator nonconst_value_type();
    ++val;
    operator=(val);
    return result;
  }

  GlobRef<T> & operator--() {
    nonconst_value_type val = operator nonconst_value_type();
    --val;
    operator=(val);
    return *this;
  }

  GlobRef<T> operator--(int) {
    GlobRef<T> result = *this;
    nonconst_value_type val = operator nonconst_value_type();
    --val;
    operator=(val);
    return result;
  }

  GlobRef<T> & operator*=(const nonconst_value_type& ref) {
    nonconst_value_type val = operator nonconst_value_type();
    val   *= ref;
    operator=(val);
    return *this;
  }

  GlobRef<T> & operator/=(const nonconst_value_type& ref) {
    nonconst_value_type val = operator nonconst_value_type();
    val   /= ref;
    operator=(val);
    return *this;
  }

  GlobRef<T> & operator^=(const nonconst_value_type& ref) {
    nonconst_value_type val = operator nonconst_value_type();
    val   ^= ref;
    operator=(val);
    return *this;
  }

  constexpr dart_gptr_t dart_gptr() const noexcept {
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
    nonconst_value_type val = operator nonconst_value_type();
    return val[pos];
  }
#endif

  /**
   * Checks whether the globally referenced element is in
   * the calling unit's local memory.
   */
  bool is_local() const {
    dart_team_unit_t luid;
    dart_team_myid(_gptr.teamid, &luid);
    return _gptr.unitid == luid.id;
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
    GlobConstPtr<MEMTYPE> gptr(dartptr);
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

};

template<typename T>
std::ostream & operator<<(
  std::ostream     & os,
  const GlobRef<T> & gref)
{
  char buf[100];
  sprintf(buf,
          "(%06X|%02X|%04X|%04X|%016lX)",
          gref._gptr.unitid,
          gref._gptr.flags,
          gref._gptr.segid,
          gref._gptr.teamid,
          gref._gptr.addr_or_offs.offset);
  os << dash::typestr(gref) << buf;
  return os;
}

} // namespace dash

#endif // DASH__GLOBREF_H_
