#ifndef DASH__GLOB_SHARED_EF_H_
#define DASH__GLOB_SHARED_EF_H_

#include <dash/Init.h>
#include <dash/memory/GlobStaticMem.h>
#include <dash/memory/GlobHeapMem.h>
#include <dash/algorithm/Operation.h>


namespace dash {

// Forward declaration
template<typename T, class A> class GlobStaticMem;
// Forward declaration
template<typename T, class A> class GlobHeapMem;
// Forward declaration
template<typename T, class MemSpaceT> class GlobPtr;

template<
  typename T,
  typename GlobalPointerType = GlobPtr<T> >
class GlobSharedRef
{
  template<typename U>
  friend std::ostream & operator<<(
    std::ostream & os,
    const GlobSharedRef<U> & gref);

private:
  typedef GlobSharedRef<T, GlobalPointerType>
    self_t;

public:
  typedef GlobalPointerType              global_pointer;
  typedef GlobalPointerType        const_global_pointer;
  typedef       T *                       local_pointer;
  typedef const T *                 const_local_pointer;

public:
  /// Convert
  ///   GlobSharedRef<T, GlobalPointer<T>>
  /// to
  ///   GlobSharedRef<U, GlobalPointer<U>>.
  template<typename U>
  struct rebind {
    typedef GlobSharedRef<
              U,
              // recursively rebind depending global pointer type:
              typename GlobalPointerType::template rebind<U>::other
            > other;
  };

private:
  dart_gptr_t   _gptr;
  local_pointer _lptr;

public:
  /**
   * Default constructor, creates an GlobSharedRef object referencing an
   * element in global memory.
   */
  GlobSharedRef()
  : _gptr(DART_GPTR_NULL),
    _lptr(nullptr)
  { }

  /**
   * Constructor, creates an GlobSharedRef object referencing an element in
   * global memory.
   */
  explicit GlobSharedRef(
    /// Pointer to referenced object in global memory
    dart_gptr_t   gptr,
    local_pointer lptr)
  : _gptr(gptr),
    _lptr(lptr)
  {
    DASH_LOG_TRACE_VAR("GlobSharedRef(gptr,lptr)", gptr);
    DASH_LOG_TRACE_VAR("GlobSharedRef(gptr,lptr)", lptr);
  }

  /**
   * Constructor, creates an GlobSharedRef object referencing an element in
   * global memory.
   */
  template<class GlobalType>
  GlobSharedRef(
    /// Pointer to referenced object in global memory
    GlobalType    & gptr,
    local_pointer   lptr = nullptr)
  : _gptr(gptr.dart_gptr()),
    _lptr(lptr)
  {
    DASH_LOG_TRACE_VAR("GlobSharedRef(gptr,lptr)", gptr);
    DASH_LOG_TRACE_VAR("GlobSharedRef(gptr,lptr)", lptr);
  }

  /**
   * Constructor, creates an GlobSharedRef object referencing an element in
   * global memory.
   */
  GlobSharedRef(
    /// Pointer to referenced object in global memory
    local_pointer lptr)
  : _gptr(DART_GPTR_NULL),
    _lptr(lptr)
  {
    DASH_LOG_TRACE_VAR("GlobSharedRef(lptr)", lptr);
  }

  /**
   * Constructor, creates an GlobSharedRef object referencing an element in
   * global memory.
   */
  explicit GlobSharedRef(dart_gptr_t dart_gptr)
  : _gptr(dart_gptr),
    _lptr(nullptr)
  {
    DASH_LOG_TRACE_VAR("GlobSharedRef(dart_gptr_t)", dart_gptr);
  }

  /**
   * Copy constructor.
   */
  GlobSharedRef(
    /// GlobSharedRef instance to copy.
    const self_t & other)
  : _gptr(other._gptr),
    _lptr(other._lptr)
  { }

#if 0
  /**
   * Like native references, global reference types cannot be copied.
   *
   * Default definition of copy constructor would conflict with semantics
   * of \c operator=(const self_t &).
   */
  GlobSharedRef(const self_t & other) = delete;
#endif

  /**
   * Assignment operator.
   */
  self_t & operator=(const self_t & other)
  {
    DASH_LOG_TRACE_VAR("GlobSharedRef.=()", other);
    // This results in a dart_put, required for STL algorithms like
    // std::copy to work on global ranges.
    // TODO: Not well-defined:
    //       This violates copy semantics:
    //         GlobSharedRef(const GlobSharedRef & other)
    //       copies the GlobSharedRef instance while
    //         GlobSharedRef=(const GlobSharedRef & other)
    //       puts the value.
    return *this = static_cast<T>(other);
//  _gptr = other._gptr;
//  return *this;
  }

  inline bool operator==(const self_t & other) const noexcept
  {
    return _lptr == other._lptr && _gptr == other._gptr;
  }

  inline bool operator!=(const self_t & other) const noexcept
  {
    return !(*this == other);
  }

  operator T() const {
    DASH_LOG_TRACE("GlobSharedRef.T()", "dereference");
    if (_lptr != nullptr) {
      DASH_LOG_TRACE_VAR("GlobSharedRef.T()", _lptr);
      return *_lptr;
    } else if (!DART_GPTR_ISNULL(_gptr)) {
      DASH_LOG_TRACE_VAR("GlobSharedRef.T()", _gptr);
      T t;
      dash::internal::get_blocking(_gptr, &t, 1);
      return t;
    }
    DASH_THROW(
      dash::exception::RuntimeError,
      "GlobSharedRef: dereferenced null-pointer");
  }

#if 0
  friend void swap(self_t a, self_t b) {
    T temp = (T)a;
    a = b;
    b = temp;
  }
#endif

  T get() const {
    DASH_LOG_TRACE("T GlobSharedRef.get()", "explicit get");
    T t;
    if (_lptr != nullptr) {
      DASH_LOG_TRACE_VAR("GlobSharedRef.T()", _lptr);
      t = *_lptr;
    } else if (!DART_GPTR_ISNULL(_gptr)) {
      DASH_LOG_TRACE_VAR("GlobSharedRef.T()", _gptr);
      dash::internal::get_blocking(_gptr, &t, 1);
    }
    return t;
  }

  void put(T & val) const {
    DASH_LOG_TRACE("GlobSharedRef.put(T&)", "explicit put");
    if (_lptr != nullptr) {
      DASH_LOG_TRACE_VAR("GlobSharedRef.T()", _lptr);
      *_lptr = val;
    } else if (!DART_GPTR_ISNULL(_gptr)) {
      DASH_LOG_TRACE_VAR("GlobSharedRef.T()", _gptr);
      dash::dart_storage<T> ds(1);
      dash::internal::put_blocking(_gptr, &val, 1);
    }
    DASH_LOG_TRACE("GlobSharedRef.put >");
  }

// explicit operator global_pointer() const {
//   DASH_LOG_TRACE("GlobSharedRef.global_pointer()", "conversion operator");
//   DASH_LOG_TRACE_VAR("GlobSharedRef.T()", _gptr);
//   return global_pointer(_gptr);
// }

  self_t & operator=(const T val) {
    DASH_LOG_TRACE_VAR("GlobSharedRef.=()", val);
    if (_lptr != nullptr) {
      DASH_LOG_TRACE_VAR("GlobSharedRef.=", _lptr);
      *_lptr = val;
    } else if (!DART_GPTR_ISNULL(_gptr)) {
      DASH_LOG_TRACE_VAR("GlobSharedRef.=", _gptr);
      dash::internal::put_blocking(_gptr, &val, 1);
    }
    DASH_LOG_TRACE("GlobSharedRef.= >");
    return *this;
  }

  GlobSharedRef<T> & operator+=(const T& ref)
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

  self_t & operator-=(const T& ref) {
    T val  = operator T();
    val   -= ref;
    operator=(val);
    return *this;
  }

  self_t & operator++() {
    T val = operator T();
    ++val;
    operator=(val);
    return *this;
  }

  self_t operator++(int) {
    self_t result = *this;
    T val = operator T();
    ++val;
    operator=(val);
    return result;
  }

  self_t & operator--() {
    T val = operator T();
    --val;
    operator=(val);
    return *this;
  }

  self_t operator--(int) {
    self_t result = *this;
    T val = operator T();
    --val;
    operator=(val);
    return result;
  }

  self_t & operator*=(const T& ref) {
    T val  = operator T();
    val   *= ref;
    operator=(val);
    return *this;
  }

  self_t & operator/=(const T& ref) {
    T val  = operator T();
    val   /= ref;
    operator=(val);
    return *this;
  }

  self_t & operator^=(const T& ref) {
    T val  = operator T();
    val   ^= ref;
    operator=(val);
    return *this;
  }

  dart_gptr_t dart_gptr() const {
    return _gptr;
  }

  local_pointer local() const {
    return _lptr;
  }

  /**
   * Checks whether the globally referenced element is in
   * the calling unit's local memory.
   */
  bool is_local() const
  {
    if (_lptr == nullptr) {
      return false;
    }
    dart_team_unit_t luid;
    dart_team_myid(_gptr.teamid, &luid);
    return _gptr.unitid == luid.id;
  }

};

template<typename T>
std::ostream & operator<<(
  std::ostream & os,
  const GlobSharedRef<T> & gref) {
  char buf[100];
  sprintf(buf,
          "(%06X|%02X|%04X|%04X|%016lX)",
          gref._gptr.unitid,
          gref._gptr.flags,
          gref._gptr.segid,
          gref._gptr.flags,
          gref._gptr.addr_or_offs.offset);
  os << "dash::GlobSharedRef<lptr: " << gref._lptr << ", " <<
        "gptr: " << typeid(T).name() << ">" << buf;
  return os;
}

} // namespace dash

#endif // DASH__GLOB_SHARED_REF_H_
