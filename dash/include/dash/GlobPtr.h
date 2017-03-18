#ifndef DASH__GLOB_PTR_H_
#define DASH__GLOB_PTR_H_

#include <cstddef>
#include <sstream>
#include <iostream>

#include <dash/dart/if/dart.h>
#include <dash/internal/Logging.h>
#include <dash/Pattern.h>
#include <dash/Exception.h>
#include <dash/Init.h>
#include <dash/Allocator.h>


std::ostream & operator<<(
  std::ostream      & os,
  const dart_gptr_t & dartptr);

bool operator==(
  const dart_gptr_t & lhs,
  const dart_gptr_t & rhs);

bool operator!=(
  const dart_gptr_t & lhs,
  const dart_gptr_t & rhs);

namespace dash {

// Forward-declarations
template<typename T>                  class GlobRef;
template<typename T,
         class    AllocT // = dash::allocator::CollectiveAllocator<T>
        >                             class GlobMem;
#if 0
template<typename T,
         class    MemorySpace =
                    GlobMem<
                      typename std::remove_const<T>::type,
                      dash::allocator::CollectiveAllocator<
                        typename std::remove_const<T>::type > >
        >                             class GlobPtr;

template<typename T, class MemSpaceT>
std::ostream & operator<<(
  std::ostream                & os,
  const GlobPtr<T, MemSpaceT> & it);
#endif
template<typename T>                  class GlobConstPtr;

/**
 * Pointer in global memory space.
 *
 * \note
 * For performance considerations, the iteration space of \c GlobPtr
 * is restricted to *local address space*.
 * If an instance of \c GlobPtr is incremented past the last address
 * of the underlying local memory range, it is not advanced to the
 * next unit's local address range.
 * Iteration over unit borders is implemented by \c GlobalIterator
 * types which perform a mapping of local and global index sets
 * specified by a \c Pattern.
 *
 * \todo Distance between two global pointers is not well-defined yet.
 *
 * \see GlobIter
 * \see GlobViewIter
 * \see DashPatternConcept
 *
 */
template<
  typename ElementType,
  class    MemorySpace  = GlobMem<
                            typename std::remove_const<ElementType>::type,
                            dash::allocator::CollectiveAllocator<
                              typename std::remove_const<ElementType>::type
                            > >
>
class GlobPtr
{
private:
  typedef GlobPtr<ElementType, MemorySpace>                self_t;

public:
  typedef ElementType                                  value_type;
  typedef GlobPtr<const ElementType, MemorySpace>      const_type;
  typedef typename dash::default_index_t               index_type;

  typedef index_type                                   gptrdiff_t;

public:
  /// Convert GlobPtr<T> to GlobPtr<U>.
  template<typename U, class MSp = MemorySpace>
  struct rebind {
    typedef GlobPtr<U, MSp> other;
  };

private:
  dart_gptr_t         _dart_gptr = DART_GPTR_NULL;
  const MemorySpace * _mem_space = nullptr;

public:
  template <typename T, class MemSpaceT>
  friend class GlobPtr;

  template <typename T, class MemSpaceT>
  friend std::ostream & operator<<(
    std::ostream                & os,
    const GlobPtr<T, MemSpaceT> & gptr);

protected:
  /**
   * Constructor, specifies underlying global address.
   *
   * Allows to instantiate GlobPtr without a valid memory
   * space reference.
   */
  constexpr GlobPtr(
    const MemorySpace * mem_space,
    dart_gptr_t gptr)
  : _dart_gptr(gptr)
  , _mem_space(reinterpret_cast<const MemorySpace *>(mem_space))
  { }

public:
  /**
   * Default constructor, underlying global address is unspecified.
   */
  constexpr GlobPtr() = default;

  /**
   * Constructor, specifies underlying global address.
   */
  constexpr GlobPtr(
    const MemorySpace & mem_space,
    dart_gptr_t gptr)
  : _dart_gptr(gptr)
  , _mem_space(reinterpret_cast<const MemorySpace *>(&mem_space))
  { }

  /**
   * Constructor for conversion of std::nullptr_t.
   */
  explicit constexpr GlobPtr(std::nullptr_t p)
  : _dart_gptr(DART_GPTR_NULL)
  , _mem_space(nullptr)
  { }

  /**
   * Copy constructor.
   */
  constexpr GlobPtr(const self_t & other)      = default;

  /**
   * Copy constructor.
   */
  template <typename T, class MemSpaceT>
  constexpr GlobPtr(const GlobPtr<T, MemSpaceT> & other)
  : _dart_gptr(other._dart_gptr)
  , _mem_space(reinterpret_cast<const MemorySpace *>(other._mem_space))
  { }

  /**
   * Assignment operator.
   */
  self_t & operator=(const self_t & rhs)       = default;

  /**
   * Assignment operator.
   */
  template <typename T, class MemSpaceT>
  self_t & operator=(const GlobPtr<T, MemSpaceT> & other)
  {
    _dart_gptr = other._dart_gptr;
    _mem_space = reinterpret_cast<const MemorySpace *>(other._mem_space);
    return *this;
  }

  /**
   * Move constructor.
   */
  constexpr GlobPtr(self_t && other)           = default;

  /**
   * Move-assignment operator.
   */
  self_t & operator=(self_t && rhs)            = default;

  /**
   * Converts pointer to its underlying global address.
   */
  explicit constexpr operator dart_gptr_t() const noexcept
  {
    return _dart_gptr;
  }

  /**
   * Converts global pointer to global const pointer.
   */
  explicit constexpr operator GlobConstPtr<value_type>() const noexcept
  {
    return GlobConstPtr<value_type>(_dart_gptr);
  }

  /**
   * The pointer's underlying global address.
   */
  constexpr dart_gptr_t dart_gptr() const noexcept
  {
    return _dart_gptr;
  }

  /**
   * Converts pointer to its referenced native pointer or
   * \c nullptr if the \c GlobPtr does not point to a local
   * address.
   */
  explicit constexpr operator value_type *() const noexcept
  {
    return local();
  }

  /**
   * Prefix increment operator.
   */
  self_t & operator++()
  {
    DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(&_dart_gptr, sizeof(ElementType)),
      DART_OK);
    return *this;
  }

  /**
   * Postfix increment operator.
   */
  self_t operator++(int)
  {
    self_t result = *this;
    DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(&_dart_gptr, sizeof(ElementType)),
      DART_OK);
    return result;
  }

  /**
   * Pointer increment operator.
   */
  self_t operator+(gptrdiff_t n) const
  {
    dart_gptr_t gptr = _dart_gptr;
    DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(&gptr, n * sizeof(ElementType)),
      DART_OK);
    return self_t(*_mem_space, gptr);
  }

  /**
   * Pointer increment operator.
   */
  self_t operator+=(gptrdiff_t n)
  {
    self_t result = *this;
    DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(&_dart_gptr, n * sizeof(ElementType)),
      DART_OK);
    return result;
  }

  /**
   * Prefix decrement operator.
   */
  self_t & operator--()
  {
    DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(&_dart_gptr, -sizeof(ElementType)),
      DART_OK);
    return *this;
  }

  /**
   * Postfix decrement operator.
   */
  self_t operator--(int)
  {
    self_t result = *this;
    DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(&_dart_gptr, -sizeof(ElementType)),
      DART_OK);
    return result;
  }

  /**
   * Pointer distance operator.
   *
   * \note
   * Distance between two global pointers is not well-defined, yet.
   * This method is only provided to comply to the pointer concept.
   */
  constexpr index_type operator-(const self_t & rhs) const noexcept
  {
    return _dart_gptr - rhs._dart_gptr;
  }

  /**
   * Pointer decrement operator.
   */
  self_t operator-(index_type n) const
  {
    dart_gptr_t gptr = _dart_gptr;
    DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(&gptr, -(n * sizeof(ElementType))),
      DART_OK);
    return self_t(*_mem_space, gptr);
  }

  /**
   * Pointer decrement operator.
   */
  self_t operator-=(index_type n)
  {
    self_t result = *this;
    DASH_ASSERT_RETURNS(
      dart_gptr_incaddr(&_dart_gptr, -(n * sizeof(ElementType))),
      DART_OK);
    return result;
  }

  /**
   * Equality comparison operator.
   */
  template <class GlobPtrT>
  constexpr bool operator==(const GlobPtrT & other) const noexcept
  {
    return DART_GPTR_EQUAL(_dart_gptr, other._dart_gptr);
  }

  /**
   * Inequality comparison operator.
   */
  template <class GlobPtrT>
  constexpr bool operator!=(const GlobPtrT & other) const noexcept
  {
    return !(*this == other);
  }

  /**
   * Less comparison operator.
   *
   * \note
   * Distance between two global pointers is not well-defined, yet.
   * This method is only provided to comply to the pointer concept.
   */
  template <class GlobPtrT>
  constexpr bool operator<(const GlobPtrT & other) const noexcept
  {
    return (
      ( _dart_gptr.unitid <  other._dart_gptr.unitid )
        ||
      ( _dart_gptr.unitid == other._dart_gptr.unitid
        &&
        ( _dart_gptr.segid <  other._dart_gptr.segid
          ||
        ( _dart_gptr.segid == other._dart_gptr.segid
          &&
          _dart_gptr.addr_or_offs.offset <
          other._dart_gptr.addr_or_offs.offset ) )
      )
    );
  }

  /**
   * Less-equal comparison operator.
   *
   * \note
   * Distance between two global pointers is not well-defined, yet.
   * This method is only provided to comply to the pointer concept.
   */
  template <class GlobPtrT>
  constexpr bool operator<=(const GlobPtrT & other) const noexcept
  {
    return (
      ( _dart_gptr.unitid <  other._dart_gptr.unitid )
        ||
      ( _dart_gptr.unitid == other._dart_gptr.unitid
        &&
        ( _dart_gptr.segid <  other._dart_gptr.segid
          ||
        ( _dart_gptr.segid == other._dart_gptr.segid
          &&
          _dart_gptr.addr_or_offs.offset <=
          other._dart_gptr.addr_or_offs.offset ) )
      )
    );
  }

  /**
   * Greater comparison operator.
   *
   * \note
   * Distance between two global pointers is not well-defined, yet.
   * This method is only provided to comply to the pointer concept.
   */
  template <class GlobPtrT>
  constexpr bool operator>(const GlobPtrT & other) const noexcept
  {
    return (
      ( _dart_gptr.unitid >  other._dart_gptr.unitid )
        ||
      ( _dart_gptr.unitid == other._dart_gptr.unitid
        &&
        ( _dart_gptr.segid >  other._dart_gptr.segid
          ||
        ( _dart_gptr.segid == other._dart_gptr.segid
          &&
          _dart_gptr.addr_or_offs.offset >
          other._dart_gptr.addr_or_offs.offset ) )
      )
    );
  }

  /**
   * Greater-equal comparison operator.
   *
   * \note
   * Distance between two global pointers is not well-defined, yet.
   * This method is only provided to comply to the pointer concept.
   */
  template <class GlobPtrT>
  constexpr bool operator>=(const GlobPtrT & other) const noexcept
  {
    return (
      ( _dart_gptr.unitid >  other._dart_gptr.unitid )
        ||
      ( _dart_gptr.unitid == other._dart_gptr.unitid
        &&
        ( _dart_gptr.segid >  other._dart_gptr.segid
          ||
        ( _dart_gptr.segid == other._dart_gptr.segid
          &&
          _dart_gptr.addr_or_offs.offset >=
          other._dart_gptr.addr_or_offs.offset ) )
      )
    );
  }

  /**
   * Subscript operator.
   */
  constexpr GlobRef<const ElementType> operator[](gptrdiff_t n) const
  {
    return GlobRef<const ElementType>(self_t((*this) + n));
  }

  /**
   * Subscript assignment operator.
   */
  GlobRef<ElementType> operator[](gptrdiff_t n)
  {
    return GlobRef<ElementType>(self_t((*this) + n));
  }

  /**
   * Dereference operator.
   */
  GlobRef<ElementType> operator*()
  {
    return GlobRef<ElementType>(*this);
  }

  /**
   * Dereference operator.
   */
  constexpr GlobRef<const ElementType> operator*() const
  {
    return GlobRef<const ElementType>(*this);
  }

  /**
   * Conversion operator to local pointer.
   *
   * \returns  A native pointer to the local element referenced by this
   *           GlobPtr instance, or \c nullptr if the referenced element
   *           is not local to the calling unit.
   */
  explicit operator value_type*() {
    void *addr = 0;
    DASH_ASSERT_RETURNS(
      dart_gptr_getaddr(_dart_gptr, &addr),
      DART_OK);
    return static_cast<ElementType*>(addr);
  }

  /**
   * Conversion to local pointer.
   *
   * \returns  A native pointer to the local element referenced by this
   *           GlobPtr instance, or \c nullptr if the referenced element
   *           is not local to the calling unit.
   */
  value_type * local() {
    void *addr = 0;
    DASH_ASSERT_RETURNS(
      dart_gptr_getaddr(_dart_gptr, &addr),
      DART_OK);
    return static_cast<ElementType*>(addr);
  }

  /**
   * Conversion to local const pointer.
   *
   * \returns  A native pointer to the local element referenced by this
   *           GlobPtr instance, or \c nullptr if the referenced element
   *           is not local to the calling unit.
   */
  const value_type * local() const {
    void *addr = 0;
    DASH_ASSERT_RETURNS(
      dart_gptr_getaddr(_dart_gptr, &addr),
      DART_OK);
    return static_cast<const ElementType*>(addr);
  }

  /**
   * Set the global pointer's associated unit.
   */
  void set_unit(team_unit_t unit_id) {
    DASH_ASSERT_RETURNS(
      dart_gptr_setunit(&_dart_gptr, unit_id),
      DART_OK);
  }

  /**
   * Check whether the global pointer is in the local
   * address space the pointer's associated unit.
   */
  bool is_local() const {
    dart_team_unit_t luid;
    dart_team_myid(_dart_gptr.teamid, &luid);
    return _dart_gptr.unitid == luid.id;
  }
};

template<typename T, class MemSpaceT>
std::ostream & operator<<(
  std::ostream                & os,
  const GlobPtr<T, MemSpaceT> & gptr)
{
  std::ostringstream ss;
  char buf[100];
  sprintf(buf,
          "%06X|%02X|%04X|%04X|%016lX",
          gptr._dart_gptr.unitid,
          gptr._dart_gptr.flags,
          gptr._dart_gptr.segid,
          gptr._dart_gptr.teamid,
          gptr._dart_gptr.addr_or_offs.offset);
  ss << "dash::GlobPtr<" << typeid(T).name() << ">(" << buf << ")";
  return operator<<(os, ss.str());
}


/**
 * Wraps underlying global address as global const pointer.
 *
 * As pointer arithmetics are inaccessible for const pointer types,
 * no coupling to global memory space is required.
 */
template<typename T>
class GlobConstPtr
: GlobPtr<T> // NOTE: non-public subclassing
{
  typedef GlobConstPtr<T> self_t;
  typedef GlobPtr<T>      base_t;
 public:
  typedef T                                            value_type;
  typedef typename base_t::const_type                  const_type;
  typedef typename base_t::index_type                  index_type;
  typedef typename base_t::gptrdiff_t                  gptrdiff_t;

  template <typename T_>
  friend std::ostream & operator<<(
    std::ostream           & os,
    const GlobConstPtr<T_> & gptr);

 public:
  /**
   * Default constructor.
   *
   * Pointer arithmetics are undefined for the created instance.
   */
  constexpr GlobConstPtr()
  : base_t(nullptr, DART_GPTR_NULL)
  { }

  /**
   * Constructor for conversion of std::nullptr_t.
   */
  explicit constexpr GlobConstPtr(std::nullptr_t p)
  : base_t(nullptr, DART_GPTR_NULL)
  { }

  /**
   * Constructor, wraps underlying global address without coupling
   * to global memory space.
   *
   * Pointer arithmetics are undefined for the created instance.
   */
  explicit constexpr GlobConstPtr(dart_gptr_t gptr)
  : base_t(nullptr, gptr)
  { }

  /**
   * Move constructor.
   */
  constexpr GlobConstPtr(self_t && other)      = default;

  /**
   * Move-assignment operator.
   */
  self_t & operator=(self_t && rhs)            = default;

  value_type * local() {
    return base_t::local();
  }

  const value_type * local() const {
    return base_t::local();
  }

  bool is_local() const {
    return base_t::is_local();
  }

  explicit constexpr operator dart_gptr_t() const noexcept
  {
    return base_t::dart_gptr();
  }

  constexpr dart_gptr_t dart_gptr() const noexcept
  {
    return base_t::dart_gptr();
  }

  explicit constexpr operator value_type *() const noexcept
  {
    return local();
  }

  explicit operator value_type*() {
    return local();
  }

  GlobRef<value_type> operator*()
  {
    return base_t::operator*();
  }

  constexpr GlobRef<const value_type> operator*() const
  {
    return base_t::operator*();
  }

  /**
   * Equality comparison operator.
   */
  template <class GlobPtrT>
  constexpr bool operator==(const GlobPtrT & other) const noexcept
  {
    return base_t::operator==(other);
  }

  /**
   * Inequality comparison operator.
   */
  template <class GlobPtrT>
  constexpr bool operator!=(const GlobPtrT & other) const noexcept
  {
    return base_t::operator!=(other);
  }

  /**
   * Less comparison operator.
   *
   * \note
   * Distance between two global pointers is not well-defined, yet.
   * This method is only provided to comply to the pointer concept.
   */
  template <class GlobPtrT>
  constexpr bool operator<(const GlobPtrT & other) const noexcept
  {
    return base_t::operator<(other);
  }

  /**
   * Less-equal comparison operator.
   *
   * \note
   * Distance between two global pointers is not well-defined, yet.
   * This method is only provided to comply to the pointer concept.
   */
  template <class GlobPtrT>
  constexpr bool operator<=(const GlobPtrT & other) const noexcept
  {
    return base_t::operator<=(other);
  }

  /**
   * Greater comparison operator.
   *
   * \note
   * Distance between two global pointers is not well-defined, yet.
   * This method is only provided to comply to the pointer concept.
   */
  template <class GlobPtrT>
  constexpr bool operator>(const GlobPtrT & other) const noexcept
  {
    return base_t::operator>(other);
  }

  /**
   * Greater-equal comparison operator.
   *
   * \note
   * Distance between two global pointers is not well-defined, yet.
   * This method is only provided to comply to the pointer concept.
   */
  template <class GlobPtrT>
  constexpr bool operator>=(const GlobPtrT & other) const noexcept
  {
    return base_t::operator>=(other);
  }
};

template<typename T>
std::ostream & operator<<(
  std::ostream          & os,
  const GlobConstPtr<T> & gptr)
{
  std::ostringstream ss;
  char buf[100];
  sprintf(buf,
          "%06X|%02X|%04X|%04X|%016lX",
          gptr.dart_gptr().unitid,
          gptr.dart_gptr().flags,
          gptr.dart_gptr().segid,
          gptr.dart_gptr().teamid,
          gptr.dart_gptr().addr_or_offs.offset);
  ss << "dash::GlobConstPtr<" << typeid(T).name() << ">(" << buf << ")";
  return operator<<(os, ss.str());
}

} // namespace dash

#endif // DASH__GLOB_PTR_H_
