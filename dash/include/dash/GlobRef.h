#ifndef DASH__GLOBREF_H_
#define DASH__GLOBREF_H_

#include <dash/memory/GlobStaticMem.h>
#include <dash/Init.h>
#include <dash/Meta.h>

#include <dash/GlobAsyncRef.h>


namespace dash {

// Forward declarations
template<typename T, class A> class GlobStaticMem;
template<typename T, class MemSpaceT> class GlobPtr;

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

  typedef typename std::add_const<T>::type
    const_value_type;
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
   * Constructor to convert \c GlobAsyncRef to GlobRef. Set to explicit to
   * avoid unintendet conversion
   */
  explicit constexpr GlobRef(
    const GlobAsyncRef<T> & gref)
  : _gptr(gref.dart_gptr())
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
  self_t & operator=(const T val) {
    set(val);
    return *this;
  }

  /**
   * Assignment operator.
   */
  self_t & operator=(const self_t & other)
  {
    set(static_cast<T>(other));
    return *this;
  }

  /**
   * Assignment operator.
   */
  template <typename GlobRefOrElementT>
  self_t & operator=(GlobRefOrElementT && other)
  {
    set(std::forward<GlobRefOrElementT>(other));
    return *this;
  }

  operator nonconst_value_type() const {
    DASH_LOG_TRACE("GlobRef.T()", "conversion operator");
    DASH_LOG_TRACE_VAR("GlobRef.T()", _gptr);
    nonconst_value_type t;
    dash::dart_storage<T> ds(1);
    DASH_ASSERT_RETURNS(
      dart_get_blocking(static_cast<void *>(&t), _gptr, ds.nelem, ds.dtype),
      DART_OK
    );
    DASH_LOG_TRACE_VAR("GlobRef.T >", _gptr);
    return t;
  }

  template <class GlobRefT, typename ValueT = typename GlobRefT::value_type>
  bool operator==(const GlobRefT & other) {
    ValueT val = other.get();
    return operator==(val);
  }

  template <class GlobRefT>
  bool operator!=(const GlobRefT & other) {
    return !(*this == other);
  }

  constexpr bool operator==(const_value_type & value) const
  {
    return static_cast<T>(*this) == value;
  }

  constexpr bool operator!=(const_value_type & value) const
  {
    return !(*this == value);
  }

  void set(const_value_type & val) {
    DASH_LOG_TRACE_VAR("GlobRef.set()", val);
    DASH_LOG_TRACE_VAR("GlobRef.set", _gptr);
    // TODO: Clarify if dart-call can be avoided if
    //       _gptr->is_local()
    dash::dart_storage<T> ds(1);
    DASH_ASSERT_RETURNS(
      dart_put_blocking(
        _gptr, static_cast<const void *>(&val), ds.nelem, ds.dtype),
      DART_OK
    );
    DASH_LOG_TRACE_VAR("GlobRef.set >", _gptr);
  }

  nonconst_value_type get() const {
    DASH_LOG_TRACE("T GlobRef.get()", "explicit get");
    DASH_LOG_TRACE_VAR("GlobRef.T()", _gptr);
    nonconst_value_type t;
    dash::dart_storage<T> ds(1);
    DASH_ASSERT_RETURNS(
      dart_get_blocking(static_cast<void *>(&t), _gptr, ds.nelem, ds.dtype),
      DART_OK
    );
    return t;
  }

  void get(nonconst_value_type *tptr) const {
    DASH_LOG_TRACE("GlobRef.get(T*)", "explicit get into provided ptr");
    DASH_LOG_TRACE_VAR("GlobRef.T()", _gptr);
    dash::dart_storage<T> ds(1);
    DASH_ASSERT_RETURNS(
      dart_get_blocking(static_cast<void *>(tptr), _gptr, ds.nelem, ds.dtype),
      DART_OK
    );
  }

  void get(nonconst_value_type& tref) const {
    DASH_LOG_TRACE("GlobRef.get(T&)", "explicit get into provided ref");
    DASH_LOG_TRACE_VAR("GlobRef.T()", _gptr);
    dash::dart_storage<T> ds(1);
    DASH_ASSERT_RETURNS(
      dart_get_blocking(static_cast<void *>(&tref), _gptr, ds.nelem, ds.dtype),
      DART_OK
    );
  }

  void put(const_value_type& tref) {
    DASH_LOG_TRACE("GlobRef.put(T&)", "explicit put of provided ref");
    DASH_LOG_TRACE_VAR("GlobRef.T()", _gptr);
    dash::dart_storage<T> ds(1);
    DASH_ASSERT_RETURNS(
      dart_put_blocking(
          _gptr, static_cast<const void *>(&tref), ds.nelem, ds.dtype),
      DART_OK
    );
  }

  void put(const_value_type* tptr) {
    DASH_LOG_TRACE("GlobRef.put(T*)", "explicit put of provided ptr");
    DASH_LOG_TRACE_VAR("GlobRef.T()", _gptr);
    dash::dart_storage<T> ds(1);
    DASH_ASSERT_RETURNS(
      dart_put_blocking(
          _gptr, static_cast<const void *>(tptr), ds.nelem, ds.dtype),
      DART_OK
    );
  }

  self_t & operator+=(const nonconst_value_type& ref) {
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

  self_t & operator-=(const nonconst_value_type& ref) {
    nonconst_value_type val  = operator nonconst_value_type();
    val   -= ref;
    operator=(val);
    return *this;
  }

  self_t & operator++() {
    nonconst_value_type val = operator nonconst_value_type();
    operator=(++val);
    return *this;
  }

  nonconst_value_type operator++(int) {
    nonconst_value_type val = operator nonconst_value_type();
    nonconst_value_type res = val++;
    operator=(val);
    return res;
  }

  self_t & operator--() {
    nonconst_value_type val = operator nonconst_value_type();
    operator=(--val);
    return *this;
  }

  nonconst_value_type operator--(int) {
    nonconst_value_type val = operator nonconst_value_type();
    nonconst_value_type res = val--;
    operator=(val);
    return res;
  }

  self_t & operator*=(const_value_type& ref) {
    nonconst_value_type val = operator nonconst_value_type();
    val   *= ref;
    operator=(val);
    return *this;
  }

  self_t & operator/=(const_value_type& ref) {
    nonconst_value_type val = operator nonconst_value_type();
    val   /= ref;
    operator=(val);
    return *this;
  }

  self_t & operator^=(const_value_type& ref) {
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
    return GlobRef<MEMTYPE>(dartptr);
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

  /**
   * specialization which swappes the values of two global references
   */
  inline void swap(dash::GlobRef<T> & b){
    T tmp = static_cast<T>(*this);
    *this = b;
    b = tmp;
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

/**
 * specialization for unqualified calls to swap
 */
template<typename T>
void swap(dash::GlobRef<T> && a, dash::GlobRef<T> && b){
  a.swap(b);
}

} // namespace dash

#endif // DASH__GLOBREF_H_
