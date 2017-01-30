#ifndef DASH__ATOMIC_GLOBREF_H_
#define DASH__ATOMIC_GLOBREF_H_

#include <dash/GlobPtr.h>
#include <dash/atomic/Operation.h>



namespace dash {

// forward decls
template<typename T>
class Atomic;

/**
 * Specialization for atomic values. All atomic operations are 
 * \c const as the \c GlobRef does not own the atomic values.
 */
template<typename T>
class GlobRef<dash::Atomic<T>>
{
  template<typename U>
  friend std::ostream & operator<<(
    std::ostream & os,
    const GlobRef<U> & gref);
  
public:
  typedef T value_type;
  
private:
  typedef dash::Atomic<T>      atomic_t;
  typedef GlobRef<atomic_t>      self_t;
  
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
    GlobPtr<atomic_t, PatternT> & gptr)
  : GlobRef(gptr.dart_gptr())
  { }
  
  /**
   * Constructor, creates an GlobRef object referencing an element in global
   * memory.
   */
  template<typename PatternT>
  GlobRef(
    /// Pointer to referenced object in global memory
    const GlobPtr<atomic_t, PatternT> & gptr)
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
    const GlobRef<atomic_t> & other)
  : _gptr(other._gptr)
  { }
  
  /**
   * Assignment operator.
   */
  self_t & operator=(const self_t & other)
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
    return *this = static_cast<atomic_t>(other);
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
    return (load() == value);
  }
  
  inline bool operator!=(const T & value) const
  {
    return !(*this == value);
  }
  
  operator T() const {
    return load();
  }
  
  operator GlobPtr<T>() const {
    DASH_LOG_TRACE("GlobRef.GlobPtr()", "conversion operator");
    DASH_LOG_TRACE_VAR("GlobRef.T()", _gptr);
    return GlobPtr<atomic_t>(_gptr);
  }
  
  dart_gptr_t dart_gptr() const {
    return _gptr;
  }
  
  /**
   * Checks whether the globally referenced element is in
   * the calling unit's local memory.
   */
  bool is_local() const {
    return GlobPtr<T>(_gptr).is_local();
  }
  
  /// atomically assigns value
  GlobRef<atomic_t> operator=(const T & value) const {
    dash::atomic::store(*this, value);
    return *this;
  }
  
  void store(const T & value) const {
    dash::atomic::store(*this, value);
  }
  /// atomically fetches value
  T load() const {
    return dash::atomic::load(*this);
  }
  
  /**
   * atomically exchanges value
   */
  T exchange(const T & value) const {
    return dash::atomic::exchange(*this, value);
  }
  
  /**
   * Atomically compares the value with the value of expected and if those are
   * bitwise-equal, replaces the former with desired.
   * 
   * \note not implemented yet
   * 
   * \return  True if value is exchanged
   * 
   * \see \c dash::atomic::compare_exchange
   */
  bool compare_exchange(const T & expected, const T & desired) const {
    return dash::atomic::compare_exchange(*this, expected, desired);
  }
  
  /*
   * ---------------------------------------------------------------------------
   * ------------ specializations for atomic integral types --------------------
   * ---------------------------------------------------------------------------
   *
   *  As the check for integral type is already implemented in the 
   *  dash::atomic::* algorithms, no check is performed here
   */
  
  /**
   * DASH specific variant which is faster than \cfetch_add
   * but does not return value
   */
  void add(const T & value) const {
    dash::atomic::add(*this, value);
  }
  
  T fetch_add(const T & value) const {
    return dash::atomic::fetch_add(*this, value);
  }
  
  /**
   * DASH specific variant which is faster than \cfetch_sub
   * but does not return value
   */
  void sub(const T & value) const {
    dash::atomic::sub(*this, value);
  }
  
  T fetch_sub(const T & value) const {
    return dash::atomic::fetch_sub(*this, value);
  }
  
  /// prefix atomically increment value by one
  T operator++ () const {
    return fetch_add(1) + 1;
  }
  
  /// postfix atomically increment value by one
  T operator++ (int) const {
    return fetch_add(1);
  }
  
  /// prefix atomically decrement value by one
  T operator-- () const {
    return fetch_sub(1) - 1;
  }
  
  /// postfix atomically decrement value by one
  T operator-- (int) const {
    return fetch_sub(1);
  }
  
  /// atomically increment value by ref
  T operator+=(const T & value) const {
    return fetch_add(value) + value;
  }
  
  /// atomically decrement value by ref
  T operator-=(const T & value) const {
    return fetch_sub(value) - value;
  }
  
private:
  
  dart_gptr_t _gptr;
  
};

} // namespace dash

#endif // DASH__ATOMIC_GLOBREF_H_

