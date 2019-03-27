#ifndef DASH__GLOBREF_H_
#define DASH__GLOBREF_H_

#include <dash/Onesided.h>
#include <dash/iterator/internal/GlobRefBase.h>

namespace dash {

// Forward declarations
template<typename T> class GlobAsyncRef;

template<typename T>
class GlobRef : public detail::GlobRefBase<T>
{
  using base_t = detail::GlobRefBase<T>;
public:
  using value_type          = typename base_t::value_type;
  using const_value_type    = typename base_t::const_value_type;
  using nonconst_value_type = typename base_t::nonconst_value_type;
  using const_type          = GlobRef<const_value_type>;
private:
  template <class T_>
  friend class GlobRef; //required for .member()


  template<typename U>
  friend std::ostream & operator<<(
    std::ostream & os,
    const GlobRef<U> & gref);

public:
  //inherit all constructors from parent class
  using detail::GlobRefBase<T>::GlobRefBase;
  /**
   * COPY Construction
   */
  GlobRef(const GlobRef & other) = default;

  /**
   * MOVE Construction
   */
  GlobRef(GlobRef && other) = default;

  /**
   * Copy Assignment: We copy the value behind this address, NOT the reference
   */
  const GlobRef & operator=(const GlobRef & other) const
  {
    if (DART_GPTR_EQUAL(this->dart_gptr(), other.dart_gptr())) {
      return *this;
    }
    set(static_cast<T>(other));
    return *this;
  }

  /**
   * Move Assignment: Redirects to Copy Assignment
   */
  GlobRef& operator=(GlobRef&& other) {
    auto dart_pointer = this->dart_gptr();
    DASH_LOG_TRACE("GlobRef.operator=(GlobRef &&)", dart_pointer);
    operator=(other);
    return *this;
  }

  /**
   * Value-assignment operator.
   */
  auto& operator=(const value_type& val) const {
    static_assert(!std::is_const<value_type>::value, "must not be const");
    set(val);
    return *this;
  }


  operator nonconst_value_type() const {
    DASH_LOG_TRACE("GlobRef.T()", "conversion operator");
    auto dart_pointer = this->dart_gptr();
    DASH_LOG_TRACE_VAR("GlobRef.T()", dart_pointer);
    nonconst_value_type t;
    dash::internal::get_blocking(dart_pointer, &t, 1);
    return t;
  }

  void
  set(const value_type & val) const {
    static_assert(std::is_same<value_type, nonconst_value_type>::value,
                  "Cannot modify value referenced by GlobRef<const T>!");

    DASH_LOG_TRACE_VAR("GlobRef.set()", val);
    auto dart_pointer = this->dart_gptr();
    DASH_LOG_TRACE_VAR("GlobRef.set", dart_pointer);
    dash::internal::put_blocking(dart_pointer, &val, 1);
  }

  nonconst_value_type get() const {
    DASH_LOG_TRACE("T GlobRef.get()", "explicit get");
    auto dart_pointer = this->dart_gptr();
    nonconst_value_type t;
    dash::internal::get_blocking(dart_pointer, &t, 1);
    return t;
  }

  void get(nonconst_value_type *tptr) const {
    DASH_LOG_TRACE("GlobRef.get(T*)", "explicit get into provided ptr");
    auto dart_pointer = this->dart_gptr();
    DASH_LOG_TRACE_VAR("GlobRef.T()", dart_pointer);
    dash::internal::get_blocking(dart_pointer, tptr, 1);
  }

  void get(nonconst_value_type& tref) const {
    DASH_LOG_TRACE("GlobRef.get(T&)", "explicit get into provided ref");
    auto dart_pointer = this->dart_gptr();
    DASH_LOG_TRACE_VAR("GlobRef.T()", dart_pointer);
    dash::internal::get_blocking(dart_pointer, &tref, 1);
  }

  void
  put(const_value_type& tref) const {
    static_assert(std::is_same<value_type, nonconst_value_type>::value,
                  "Cannot assign to GlobRef<const T>!");
    DASH_LOG_TRACE("GlobRef.put(T&)", "explicit put of provided ref");
    auto dart_pointer = this->dart_gptr();
    DASH_LOG_TRACE_VAR("GlobRef.T()", dart_pointer);
    dash::internal::put_blocking(dart_pointer, &tref, 1);
  }

  void
  put(const_value_type* tptr) const {
    static_assert(std::is_same<value_type, nonconst_value_type>::value,
                  "Cannot modify value referenced by GlobRef<const T>!");
    DASH_LOG_TRACE("GlobRef.put(T*)", "explicit put of provided ptr");
    auto dart_pointer = this->dart_gptr();
    DASH_LOG_TRACE_VAR("GlobRef.T()", dart_pointer);
    dash::internal::put_blocking(dart_pointer, tptr, 1);
  }

  const GlobRef &
  operator+=(const nonconst_value_type& ref) const {
    static_assert(std::is_same<value_type, nonconst_value_type>::value,
                  "Cannot modify value referenced by GlobRef<const T>!");
    nonconst_value_type val  = operator nonconst_value_type();
    val   += ref;
    operator=(val);
    return *this;
  }

  const GlobRef &
  operator-=(const nonconst_value_type& ref) const {
    static_assert(std::is_same<value_type, nonconst_value_type>::value,
                  "Cannot modify value referenced by GlobRef<const T>!");
    nonconst_value_type val  = operator nonconst_value_type();
    val   -= ref;
    operator=(val);
    return *this;
  }

  const GlobRef &
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

  const GlobRef &
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

  const GlobRef &
  operator*=(const_value_type& ref) const {
    static_assert(std::is_same<value_type, nonconst_value_type>::value,
                  "Cannot modify value referenced by GlobRef<const T>!");
    nonconst_value_type val = operator nonconst_value_type();
    val   *= ref;
    operator=(val);
    return *this;
  }

  const GlobRef &
  operator/=(const_value_type& ref) const {
    static_assert(std::is_same<value_type, nonconst_value_type>::value,
                  "Cannot modify value referenced by GlobRef<const T>!");
    nonconst_value_type val = operator nonconst_value_type();
    val   /= ref;
    operator=(val);
    return *this;
  }

  const GlobRef &
  operator^=(const_value_type& ref) const {
    static_assert(std::is_same<value_type, nonconst_value_type>::value,
                  "Cannot modify value referenced by GlobRef<const T>!");
    nonconst_value_type val = operator nonconst_value_type();
    val   ^= ref;
    operator=(val);
    return *this;
  }

  /**
   * Checks whether the globally referenced element is in
   * the calling unit's local memory.
   */
  bool is_local() const {
    dart_team_unit_t luid;
    auto dart_pointer = this->dart_gptr();
    dart_team_myid(dart_pointer.teamid, &luid);
    return dart_pointer.unitid == luid.id;
  }

  /**
   * Get a global ref to a member of a certain type at the
   * specified offset
   */
  template <typename MEMTYPE>
  auto member(size_t offs) const DASH_NOEXCEPT
  {
    using ref_t = GlobRef<typename std::conditional<
        std::is_const<value_type>::value,
        typename std::add_const<MEMTYPE>::type,
        MEMTYPE>::type>;

    dart_gptr_t dartptr = this->dart_gptr();
    DASH_ASSERT_RETURNS(dart_gptr_incaddr(&dartptr, offs), DART_OK);

    return ref_t{dartptr};
  }

  /**
   * Get the member via pointer to member
   */
  template <class MEMTYPE, class P = T>
  auto member(const MEMTYPE P::*mem) const DASH_NOEXCEPT
  {
    return member<MEMTYPE>(detail::offset_of(mem));
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

};

template<typename T>
std::ostream & operator<<(
  std::ostream     & os,
  const GlobRef<T> & gref)
{
  char buf[100]; //
  auto dart_pointer = gref.dart_gptr();
  sprintf(buf,
          "(%06X|%02X|%04X|%04X|%016lX)",
          dart_pointer.unitid,
          dart_pointer.flags,
          dart_pointer.segid,
          dart_pointer.teamid,
          dart_pointer.addr_or_offs.offset);
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
