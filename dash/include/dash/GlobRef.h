#ifndef DASH__GLOBREF_H_
#define DASH__GLOBREF_H_

#include <dash/GlobPtr.h>
#include <dash/Onesided.h>
#include <dash/iterator/internal/GlobRefBase.h>

namespace dash {

// forward declaration
template<typename T>
class GlobAsyncRef;

// Forward declarations
template<typename T, class MemSpaceT> class GlobPtr;

template<typename T>
class GlobRef
{
  template<typename U>
  friend std::ostream & operator<<(
    std::ostream & os,
    const GlobRef<U> & gref);

  template <typename ElementT>
  friend class GlobRef;

public:
  using value_type          = T;
  using const_value_type    = typename std::add_const<T>::type;
  using nonconst_value_type = typename std::remove_const<T>::type;
  using self_t              = GlobRef<T>;
  using const_type          = GlobRef<const_value_type>;


  template <class _T, class _M>
  friend class GlobPtr;

  template <class _T, class _Pat, class _M, class _Ptr, class _Ref>
  friend class GlobIter;

  template <class _T, class _Pat, class _M, class _Ptr, class _Ref>
  friend class GlobViewIter;

private:
  /**
   * PRIVATE: Constructor, creates an GlobRef object referencing an element in global
   * memory.
   */
  template<class ElementT, class MemSpaceT>
  explicit constexpr GlobRef(
    /// Pointer to referenced object in global memory
    const GlobPtr<ElementT, MemSpaceT> & gptr)
  : GlobRef(gptr.dart_gptr())
  { }

public:
  /**
   * Reference semantics forbid declaration without definition.
   */
  GlobRef() = delete;

  GlobRef(const GlobRef & other) = delete;

  /**
   * Constructor, creates an GlobRef object referencing an element in global
   * memory.
   */
  explicit constexpr GlobRef(dart_gptr_t dart_gptr) noexcept
  : _gptr(dart_gptr)
  {
  }

  /**
   * Copy constructor, implicit if at least one of the following conditions is
   * satisfied:
   *    1) value_type and _T are exactly the same types (including const and
   *    volatile qualifiers
   *    2) value_type and _T are the same types after removing const and
   *    volatile qualifiers and value_type itself is const.
   */
  template <
      typename _T,
      long = internal::enable_implicit_copy_ctor<value_type, _T>::value>
  constexpr GlobRef(const GlobRef<_T>& gref) noexcept
    : GlobRef(gref.dart_gptr())
  {
  }

  /**
   * Copy constructor, explicit if the following conditions are satisfied.
   *    1) value_type and _T are the same types after excluding const and
   *    volatile qualifiers
   *    2) value_type is const and _T is non-const
   */
  template <
      typename _T,
      int = internal::enable_explicit_copy_ctor<value_type, _T>::value>
  explicit constexpr GlobRef(const GlobRef<_T>& gref) noexcept
    : GlobRef(gref.dart_gptr())
  {
  }

  /**
   * Constructor to convert \c GlobAsyncRef to GlobRef. Set to explicit to
   * avoid unintendet conversion
   */
  template <
      typename _T,
      long = internal::enable_implicit_copy_ctor<value_type, _T>::value>
  constexpr GlobRef(const GlobAsyncRef<_T>& gref) noexcept
    : _gptr(gref.dart_gptr())
  {
  }

  template <
      typename _T,
      int = internal::enable_explicit_copy_ctor<value_type, _T>::value>
  explicit constexpr GlobRef(const GlobAsyncRef<_T>& gref) noexcept
    : GlobRef(gref.dart_gptr())
  {
  }

  /**
   * Move Constructor
   */
  GlobRef(self_t&& other) noexcept
    :_gptr(std::move(other._gptr))
  {
    DASH_LOG_TRACE("GlobRef.GlobRef(GlobRef &&)", _gptr);
  }

  /**
   * Copy Assignment
   */
  const self_t & operator=(const self_t & other) const
  {
    if (DART_GPTR_EQUAL(_gptr, other._gptr)) {
      return *this;
    }
    set(static_cast<T>(other));
    return *this;
  }

  /**
   * Move Assignment: Redirects to Copy Assignment
   */
  self_t& operator=(self_t&& other) noexcept {
    DASH_LOG_TRACE("GlobRef.operator=(GlobRef &&)", _gptr);
    operator=(other);
    return *this;
  }

  /**
   * Value-assignment operator.
   */
  const self_t & operator=(const value_type& val) const {
    set(val);
    return *this;
  }


  operator nonconst_value_type() const {
    DASH_LOG_TRACE("GlobRef.T()", "conversion operator");
    DASH_LOG_TRACE_VAR("GlobRef.T()", _gptr);
    nonconst_value_type t;
    dash::internal::get_blocking(_gptr, &t, 1);
    DASH_LOG_TRACE_VAR("GlobRef.T >", _gptr);
    return t;
  }

  template <typename ValueT>
  bool operator==(const GlobRef<ValueT> & other) const {
    ValueT val = other.get();
    return operator==(val);
  }

  template <typename ValueT>
  bool operator!=(const GlobRef<ValueT> & other) const {
    return !(*this == other);
  }

  template<typename ValueT>
  constexpr bool operator==(const ValueT& value) const
  {
    return static_cast<T>(*this) == value;
  }

  template<typename ValueT>
  constexpr bool operator!=(const ValueT& value) const
  {
    return !(*this == value);
  }

  void
  set(const value_type & val) const {
    static_assert(std::is_same<value_type, nonconst_value_type>::value,
                  "Cannot modify value referenced by GlobRef<const T>!");

    DASH_LOG_TRACE_VAR("GlobRef.set()", val);
    DASH_LOG_TRACE_VAR("GlobRef.set", _gptr);
    // TODO: Clarify if dart-call can be avoided if
    //       _gptr->is_local()
    dash::internal::put_blocking(_gptr, &val, 1);
    DASH_LOG_TRACE_VAR("GlobRef.set >", _gptr);
  }

  nonconst_value_type get() const {
    DASH_LOG_TRACE("T GlobRef.get()", "explicit get");
    DASH_LOG_TRACE_VAR("GlobRef.T()", _gptr);
    nonconst_value_type t;
    dash::internal::get_blocking(_gptr, &t, 1);
    return t;
  }

  void get(nonconst_value_type *tptr) const {
    DASH_LOG_TRACE("GlobRef.get(T*)", "explicit get into provided ptr");
    DASH_LOG_TRACE_VAR("GlobRef.T()", _gptr);
    dash::internal::get_blocking(_gptr, tptr, 1);
  }

  void get(nonconst_value_type& tref) const {
    DASH_LOG_TRACE("GlobRef.get(T&)", "explicit get into provided ref");
    DASH_LOG_TRACE_VAR("GlobRef.T()", _gptr);
    dash::internal::get_blocking(_gptr, &tref, 1);
  }

  void
  put(const_value_type& tref) const {
    static_assert(std::is_same<value_type, nonconst_value_type>::value,
                  "Cannot assign to GlobRef<const T>!");
    DASH_LOG_TRACE("GlobRef.put(T&)", "explicit put of provided ref");
    DASH_LOG_TRACE_VAR("GlobRef.T()", _gptr);
    dash::internal::put_blocking(_gptr, &tref, 1);
  }

  void
  put(const_value_type* tptr) const {
    static_assert(std::is_same<value_type, nonconst_value_type>::value,
                  "Cannot modify value referenced by GlobRef<const T>!");
    DASH_LOG_TRACE("GlobRef.put(T*)", "explicit put of provided ptr");
    DASH_LOG_TRACE_VAR("GlobRef.T()", _gptr);
    dash::internal::put_blocking(_gptr, tptr, 1);
  }

  const self_t &
  operator+=(const nonconst_value_type& ref) const {
    static_assert(std::is_same<value_type, nonconst_value_type>::value,
                  "Cannot modify value referenced by GlobRef<const T>!");
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

  const self_t &
  operator-=(const nonconst_value_type& ref) const {
    static_assert(std::is_same<value_type, nonconst_value_type>::value,
                  "Cannot modify value referenced by GlobRef<const T>!");
    nonconst_value_type val  = operator nonconst_value_type();
    val   -= ref;
    operator=(val);
    return *this;
  }

  const self_t &
  operator++() const {
    static_assert(std::is_same<value_type, nonconst_value_type>::value,
                  "Cannot modify value referenced by GlobRef<const T>!");
    nonconst_value_type val = operator nonconst_value_type();
    operator=(++val);
    return *this;
  }

  nonconst_value_type
  operator++(int) const {
    static_assert(std::is_same<value_type, nonconst_value_type>::value,
                  "Cannot modify value referenced by GlobRef<const T>!");
    nonconst_value_type val = operator nonconst_value_type();
    nonconst_value_type res = val++;
    operator=(val);
    return res;
  }

  const self_t &
  operator--() const {
    static_assert(std::is_same<value_type, nonconst_value_type>::value,
                  "Cannot modify value referenced by GlobRef<const T>!");
    nonconst_value_type val = operator nonconst_value_type();
    operator=(--val);
    return *this;
  }

  nonconst_value_type
  operator--(int) const {
    static_assert(std::is_same<value_type, nonconst_value_type>::value,
                  "Cannot modify value referenced by GlobRef<const T>!");
    nonconst_value_type val = operator nonconst_value_type();
    nonconst_value_type res = val--;
    operator=(val);
    return res;
  }

  const self_t &
  operator*=(const_value_type& ref) const {
    static_assert(std::is_same<value_type, nonconst_value_type>::value,
                  "Cannot modify value referenced by GlobRef<const T>!");
    nonconst_value_type val = operator nonconst_value_type();
    val   *= ref;
    operator=(val);
    return *this;
  }

  const self_t &
  operator/=(const_value_type& ref) const {
    static_assert(std::is_same<value_type, nonconst_value_type>::value,
                  "Cannot modify value referenced by GlobRef<const T>!");
    nonconst_value_type val = operator nonconst_value_type();
    val   /= ref;
    operator=(val);
    return *this;
  }

  const self_t &
  operator^=(const_value_type& ref) const {
    static_assert(std::is_same<value_type, nonconst_value_type>::value,
                  "Cannot modify value referenced by GlobRef<const T>!");
    nonconst_value_type val = operator nonconst_value_type();
    val   ^= ref;
    operator=(val);
    return *this;
  }

  constexpr dart_gptr_t dart_gptr() const noexcept {
    return _gptr;
  }

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
  GlobRef<typename internal::add_const_from_type<T, MEMTYPE>::type>
  member(size_t offs) const {
    dart_gptr_t dartptr = _gptr;
    DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(&dartptr, offs),
      DART_OK);
    return GlobRef<typename internal::add_const_from_type<T, MEMTYPE>::type>(dartptr);
  }

  /**
   * Get the member via pointer to member
   */
  template<class MEMTYPE, class P=T>
  GlobRef<typename internal::add_const_from_type<T, MEMTYPE>::type>
  member(
    const MEMTYPE P::*mem) const {
    // TODO: Thaaaat ... looks hacky.
    auto offs = (size_t) & (reinterpret_cast<P*>(0)->*mem);
    return member<typename internal::add_const_from_type<T, MEMTYPE>::type>(offs);
  }

  /**
   * specialization which swappes the values of two global references
   */
  inline void swap(dash::GlobRef<T> & b) const{
    static_assert(std::is_same<value_type, nonconst_value_type>::value,
                  "Cannot modify value referenced by GlobRef<const T>!");
    auto tmp = static_cast<T>(*this);
    *this = b;
    b = tmp;
  }

private:
  dart_gptr_t _gptr{};
};

template<typename T>
std::ostream & operator<<(
  std::ostream     & os,
  const GlobRef<T> & gref)
{
  char buf[100]; //
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
inline void swap(dash::GlobRef<T> && a, dash::GlobRef<T> && b){
  a.swap(b);
}

/**
 * specialization for unqualified calls to swap
 */
template <class MemSpaceT, class T>
inline auto addressof(dash::GlobRef<T> const & ref)
{
  return dash::GlobPtr<T, MemSpaceT>(ref.dart_gptr());
}

} // namespace dash

#endif // DASH__GLOBREF_H_
