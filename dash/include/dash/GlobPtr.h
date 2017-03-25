#ifndef DASH__GLOB_PTR_H_
#define DASH__GLOB_PTR_H_

#include <dash/dart/if/dart.h>

#include <dash/internal/Logging.h>

#include <dash/Pattern.h>
#include <dash/Exception.h>
#include <dash/Init.h>
#include <dash/Allocator.h>

#include <cstddef>
#include <sstream>
#include <iostream>


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
         class    AllocT>             class GlobStaticMem;
template<typename T,
         class    AllocT>             class GlobHeapMem;
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
  class    MemorySpace  = GlobStaticMem<
                            typename std::remove_const<ElementType>::type,
                            dash::allocator::SymmetricAllocator<
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
  // Raw global pointer used to initialize this pointer instance
  dart_gptr_t         _rbegin_gptr = DART_GPTR_NULL;
  // Memory space refernced by this global pointer
  const MemorySpace * _mem_space   = nullptr;
  // Size of the local section at the current position of this pointer
  index_type          _lsize       = 0;
  // Unit id of last unit in referenced global memory space
  dart_team_unit_t    _unit_end    = 0;

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
  GlobPtr(
    const MemorySpace * mem_space,
    dart_gptr_t         gptr,
    index_type          rindex = 0)
  : _rbegin_gptr(gptr)
  , _mem_space(reinterpret_cast<const MemorySpace *>(mem_space))
  , _lsize(
       mem_space == nullptr
       ? 0
       : mem_space->local_size(dart_team_unit_t { gptr.unitid }))
  , _unit_end(
       mem_space == nullptr
       ? mem_space->team().size()
       : gptr.unitid)
  {
    if (rindex < 0) { decrement(-rindex); }
    if (rindex > 0) { increment(rindex); }
  }

public:
  /**
   * Default constructor, underlying global address is unspecified.
   */
  constexpr GlobPtr() = default;

  /**
   * Constructor, specifies underlying global address.
   */
  GlobPtr(
    const MemorySpace & mem_space,
    dart_gptr_t         gptr)
  : _rbegin_gptr(gptr)
  , _mem_space(reinterpret_cast<const MemorySpace *>(&mem_space))
  , _lsize(mem_space.local_size(dart_team_unit_t { gptr.unitid }))
  , _unit_end(mem_space.team().size())
  { }

  /**
   * Constructor, specifies underlying global address.
   */
  GlobPtr(
    MemorySpace && mem_space,
    dart_gptr_t    gptr)
  : _rbegin_gptr(gptr)
  , _mem_space(nullptr)
  , _lsize(mem_space.local_size(dart_team_unit_t { gptr.unitid }))
  , _unit_end(mem_space.team().size())
  {
    // No need to bind temporary as value type member:
    // The memory space instance can only be passed as temporary if
    // it is not the owner of its referenced memory range.
    // For a use case, see dash::memalloc in dash/memory/GlobUnitMem.h.
    //
    // We could introduce `GlobSharedPtr` which could act as owners of
    // their referenced resource, corresponding to `std::shared_ptr`.
  }

  /**
   * Constructor for conversion of std::nullptr_t.
   */
  explicit constexpr GlobPtr(std::nullptr_t)
  : _rbegin_gptr(DART_GPTR_NULL)
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
  : _rbegin_gptr(other._rbegin_gptr)
  , _mem_space(reinterpret_cast<const MemorySpace *>(other._mem_space))
  , _lsize(other._lsize)
  , _unit_end(other._unit_end)
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
    _rbegin_gptr = other._rbegin_gptr;
    _mem_space   = reinterpret_cast<const MemorySpace *>(other._mem_space);
    _lsize       = other._lsize;
    _unit_end    = other._unit_end;
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
    return _rbegin_gptr;
  }

  /**
   * Converts global pointer to global const pointer.
   */
  explicit constexpr operator GlobConstPtr<value_type>() const noexcept
  {
    return GlobConstPtr<value_type>(_rbegin_gptr);
  }

  /**
   * The pointer's underlying global address.
   */
  constexpr dart_gptr_t dart_gptr() const noexcept
  {
    return _rbegin_gptr;
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
    increment(1);
    return *this;
  }

  /**
   * Postfix increment operator.
   */
  self_t operator++(int)
  {
    self_t res = *this;
    increment(1);
    return res;
  }

  /**
   * Pointer increment operator.
   */
  self_t operator+(gptrdiff_t n) const
  {
    self_t res = *this;
    res += n;
    return res;
  }

  /**
   * Pointer increment operator.
   */
  self_t & operator+=(gptrdiff_t n)
  {
    increment(n);
    return *this;
  }

  /**
   * Prefix decrement operator.
   */
  self_t & operator--()
  {
    decrement(1);
    return *this;
  }

  /**
   * Postfix decrement operator.
   */
  self_t operator--(int)
  {
    self_t res = *this;
    decrement(1);
    return res;
  }

  /**
   * Pointer offset difference operator.
   */
  constexpr index_type operator-(const self_t & rhs) const noexcept
  {
    return ( _rbegin_gptr.unitid == rhs._rbegin_gptr.unitid ||
             _mem_space == nullptr
             ? ( _rbegin_gptr.addr_or_offs.offset
                 - rhs._rbegin_gptr.addr_or_offs.offset )
               / sizeof(value_type)
             : _mem_space->distance(rhs, *this) );
  }

  /**
   * Pointer decrement operator.
   */
  self_t operator-(index_type n) const
  {
    self_t res = *this;
    res -= n;
    return res;
  }

  /**
   * Pointer decrement operator.
   */
  self_t & operator-=(index_type n)
  {
    decrement(n);
    return *this;
  }

  /**
   * Equality comparison operator.
   */
  template <class GlobPtrT>
  constexpr bool operator==(const GlobPtrT & other) const noexcept
  {
    return DART_GPTR_EQUAL(_rbegin_gptr, other._rbegin_gptr);
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
   */
  template <class GlobPtrT>
  constexpr bool operator<(const GlobPtrT & other) const noexcept
  {
    return (other - *this) > 0;
  }

  /**
   * Less-equal comparison operator.
   */
  template <class GlobPtrT>
  constexpr bool operator<=(const GlobPtrT & other) const noexcept
  {
    return (other - *this) >= 0;
  }

  /**
   * Greater comparison operator.
   */
  template <class GlobPtrT>
  constexpr bool operator>(const GlobPtrT & other) const noexcept
  {
    return (*this - other) > 0;
  }

  /**
   * Greater-equal comparison operator.
   */
  template <class GlobPtrT>
  constexpr bool operator>=(const GlobPtrT & other) const noexcept
  {
    return (*this - other) >= 0;
  }

  /**
   * Subscript operator.
   */
  constexpr GlobRef<const value_type> operator[](gptrdiff_t n) const
  {
    return GlobRef<const value_type>(self_t((*this) + n));
  }

  /**
   * Subscript assignment operator.
   */
  GlobRef<value_type> operator[](gptrdiff_t n)
  {
    return GlobRef<value_type>(self_t((*this) + n));
  }

  /**
   * Dereference operator.
   */
  GlobRef<value_type> operator*()
  {
    return GlobRef<value_type>(*this);
  }

  /**
   * Dereference operator.
   */
  constexpr GlobRef<const value_type> operator*() const
  {
    return GlobRef<const value_type>(*this);
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
      dart_gptr_getaddr(_rbegin_gptr, &addr),
      DART_OK);
    return static_cast<value_type*>(addr);
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
      dart_gptr_getaddr(_rbegin_gptr, &addr),
      DART_OK);
    return static_cast<value_type*>(addr);
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
      dart_gptr_getaddr(_rbegin_gptr, &addr),
      DART_OK);
    return static_cast<const value_type*>(addr);
  }

  /**
   * Set the global pointer's associated unit.
   */
  void set_unit(team_unit_t unit_id) {
    DASH_ASSERT_RETURNS(
      dart_gptr_setunit(&_rbegin_gptr, unit_id),
      DART_OK);
  }

  /**
   * Check whether the global pointer is in the local
   * address space the pointer's associated unit.
   */
  bool is_local() const {
    dart_team_unit_t luid;
    DASH_ASSERT_RETURNS(
      dart_team_myid(_rbegin_gptr.teamid, &luid),
      DART_OK);
    return _rbegin_gptr.unitid == luid.id;
  }

private:
  void increment(index_type offs) {
    // Pointer position still in same unit space:
    if (_mem_space == nullptr ||
        offs + (_rbegin_gptr.addr_or_offs.offset / sizeof(value_type))
          < _lsize) {
      _rbegin_gptr.addr_or_offs.offset += (offs * sizeof(value_type));
      return;
    }
    // Pointer position moved outside unit space:
    while (offs + (_rbegin_gptr.addr_or_offs.offset / sizeof(value_type))
           >= _lsize &&
           _rbegin_gptr.unitid <= _unit_end.id) {
      offs -= (_lsize
                - (_rbegin_gptr.addr_or_offs.offset / sizeof(value_type)));
      _rbegin_gptr.unitid++;
      _rbegin_gptr.addr_or_offs.offset = 0;
      _lsize = ( _rbegin_gptr.unitid < _unit_end.id
                 ? _mem_space->local_size(
                     dart_team_unit_t { _rbegin_gptr.unitid })
                 : 1);
    }
    _rbegin_gptr.addr_or_offs.offset = (offs * sizeof(value_type));
  }

  void decrement(index_type offs) {
    if (_mem_space == nullptr ||
        (_rbegin_gptr.addr_or_offs.offset / sizeof(value_type)) >= offs) {
      _rbegin_gptr.addr_or_offs.offset -= (offs * sizeof(value_type));
      return;
    }
    // Pointer position moved outside unit space:
    while ((_rbegin_gptr.addr_or_offs.offset / sizeof(value_type)) < offs &&
           _rbegin_gptr.unitid > 0) {
      offs  -= (_rbegin_gptr.addr_or_offs.offset / sizeof(value_type)) - 1;
      _rbegin_gptr.unitid--;
      _lsize = ( _rbegin_gptr.unitid < _unit_end.id
                 ? _mem_space->local_size(
                     dart_team_unit_t { _rbegin_gptr.unitid })
                 : 1);
      _rbegin_gptr.addr_or_offs.offset = (_lsize - 1) * sizeof(value_type);
    }
    _rbegin_gptr.addr_or_offs.offset = (offs * sizeof(value_type));
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
          "u%06X|f%02X|s%04X|t%04X|o%016lX",
          gptr._rbegin_gptr.unitid,
          gptr._rbegin_gptr.flags,
          gptr._rbegin_gptr.segid,
          gptr._rbegin_gptr.teamid,
          gptr._rbegin_gptr.addr_or_offs.offset);
  ss << "dash::GlobPtr<" << typeid(T).name() << ">("
     << "nloc:" << gptr._lsize     << " "
     << "uend:" << gptr._unit_end  << " " << buf << ")";
  return operator<<(os, ss.str());
}


/**
 * Wraps underlying global address as global const pointer.
 *
 * As pointer arithmetics are inaccessible for const pointer types,
 * no coupling to global memory space is required.
 *
 * TODO: Will be replaced by specialization of GlobPtr for global
 *       memory space tagged as unit-scope address space
 *       (see GlobUnitMem).
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
   * Copy constructor.
   */
  constexpr GlobConstPtr(const self_t & other) = default;

  /**
   * Move constructor.
   */
  constexpr GlobConstPtr(self_t && other)      = default;

  /**
   * Assignment operator.
   */
  self_t & operator=(const self_t & rhs)       = default;

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
          "u%06X|f%02X|s%04X|t%04X|o%016lX",
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
