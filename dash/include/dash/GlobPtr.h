#ifndef DASH__GLOB_PTR_H_
#define DASH__GLOB_PTR_H_

#include <dash/dart/if/dart.h>

#include <dash/Exception.h>
#include <dash/Init.h>

#include <dash/atomic/Type_traits.h>

#include <dash/internal/Logging.h>
#include <dash/iterator/internal/GlobPtrBase.h>

#include <dash/memory/MemorySpaceBase.h>

#include <cstddef>
#include <iostream>
#include <sstream>

std::ostream &operator<<(std::ostream &os, const dart_gptr_t &dartptr);

bool operator==(const dart_gptr_t &lhs, const dart_gptr_t &rhs);

bool operator!=(const dart_gptr_t &lhs, const dart_gptr_t &rhs);

namespace dash {

// Forward-declarations
template <typename T>
class GlobRef;

template <typename T, class MemSpaceT>
class GlobConstPtr;

template <class T, class MemSpaceT>
class GlobPtr;

template <class T, class MemSpaceT>
dash::gptrdiff_t distance(
    const GlobPtr<T, MemSpaceT> &gbegin, const GlobPtr<T, MemSpaceT> &gend);

namespace internal {
static bool is_local(dart_gptr_t gptr)
{
  dart_team_unit_t luid;
  DASH_ASSERT_RETURNS(dart_team_myid(gptr.teamid, &luid), DART_OK);
  return gptr.unitid == luid.id;
}
}  // namespace internal

/**
 * Pointer in global memory space with random access arithmetics.
 *
 * \see GlobIter
 *
 * \concept{DashMemorySpaceConcept}
 */
template <typename ElementType, class GlobMemT>
class GlobPtr {
private:
  typedef GlobPtr<ElementType, GlobMemT> self_t;

  using local_pointer_traits =
      std::pointer_traits<typename GlobMemT::local_void_pointer>;

public:
  typedef ElementType                          value_type;
  typedef GlobPtr<const ElementType, GlobMemT> const_type;
  typedef typename GlobMemT::index_type        index_type;
  typedef typename GlobMemT::size_type         size_type;
  typedef index_type                           gptrdiff_t;

  typedef typename local_pointer_traits::template rebind<value_type>
      local_type;

  typedef typename local_pointer_traits::template rebind<value_type const>
      const_local_type;

  /**
   * Rebind to a different type of pointer
   */
  template <typename T>
  using rebind = dash::GlobPtr<T, GlobMemT>;

public:
  template <typename T, class MemSpaceT>
  friend class GlobPtr;

  template <typename T, class MemSpaceT>
  friend std::ostream &operator<<(
      std::ostream &os, const GlobPtr<T, MemSpaceT> &gptr);

  template <class T, class MemSpaceT>
  friend dash::gptrdiff_t distance(
      const GlobPtr<T, MemSpaceT> &gbegin, const GlobPtr<T, MemSpaceT> &gend);

private:
  // Raw global pointer used to initialize this pointer instance
  dart_gptr_t _rbegin_gptr{DART_GPTR_NULL};
  // Memory space refernced by this global pointer
  const GlobMemT *_mem_space{nullptr};

protected:
  /**
   * Constructor, specifies underlying global address.
   *
   * Allows to instantiate GlobPtr without a valid memory
   * space reference.
   */
  GlobPtr(const GlobMemT *mem_space, dart_gptr_t gptr, index_type rindex = 0)
    : _rbegin_gptr(gptr)
    , _mem_space(mem_space)
  {
    if (rindex < 0) {
      decrement(-rindex);
    }
    if (rindex > 0) {
      increment(rindex);
    }
  }

public:
  /**
   * Default constructor, underlying global address is unspecified.
   */
  constexpr GlobPtr() = default;

  /**
   * Constructor, specifies underlying global address.
   */
  GlobPtr(const GlobMemT &mem_space, dart_gptr_t gptr)
    : _rbegin_gptr(gptr)
    , _mem_space(&mem_space)
  {
  }

  /**
   * Constructor for conversion of std::nullptr_t.
   */
  explicit constexpr GlobPtr(std::nullptr_t)
    : _rbegin_gptr(DART_GPTR_NULL)
    , _mem_space(nullptr)
  {
  }

  /**
   * Copy constructor.
   */
  constexpr GlobPtr(const self_t &other) = default;

  /**
   * Assignment operator.
   */
  self_t &operator=(const self_t &rhs) = default;

  //clang-format off
  /**
   * Implicit Converting Constructor, only allowed if one of the following
   * conditions is satisfied:
   *   - Either From or To (value_type) are void. Like in C we can convert any
   *     pointer type to a void pointer and back again.
   *   - From::value_type& is assignable to value_type&.
   *
   * NOTE: Const correctness is considered. We can assign a GlobPtr<const T>
   *       to a GlobPtr<T> but not the other way around
   */
  //clang-format on
  template <
      typename From,
      typename = typename std::enable_if<
          // We always allow GlobPtr<T> -> GlobPtr<void> or the other way)
          // or if From is assignable to To (value_type)
          dash::internal::is_pointer_assignable<
              typename dash::remove_atomic<From>::type,
              typename dash::remove_atomic<value_type>::type>::value>

      ::type>
  constexpr GlobPtr(const GlobPtr<From, GlobMemT> &other)
    : _rbegin_gptr(other._rbegin_gptr)
    , _mem_space(reinterpret_cast<const GlobMemT *>(other._mem_space))
  {
  }

  /**
   * Assignment operator.
   */
  template <typename T, class MemSpaceT>
  self_t &operator=(const GlobPtr<T, MemSpaceT> &other)
  {
    _rbegin_gptr = other._rbegin_gptr;
    _mem_space   = reinterpret_cast<const GlobMemT *>(other._mem_space);
    return *this;
  }

  /**
   * Move constructor.
   */
  constexpr GlobPtr(self_t &&other) = default;

  /**
   * Move-assignment operator.
   */
  self_t &operator=(self_t &&rhs) = default;

  /**
   * Converts pointer to its referenced native pointer or
   * \c nullptr if the \c GlobPtr does not point to a local
   * address.
   */
  explicit operator value_type *() const noexcept
  {
    return local();
  }

  /**
   * Conversion operator to local pointer.
   *
   * \returns  A native pointer to the local element referenced by this
   *           GlobPtr instance, or \c nullptr if the referenced element
   *           is not local to the calling unit.
   */
  explicit operator value_type *() noexcept
  {
    return local();
  }

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
  explicit constexpr operator GlobConstPtr<value_type, GlobMemT>() const
      noexcept
  {
    return GlobConstPtr<value_type, GlobMemT>(_rbegin_gptr);
  }

  /**
   * The pointer's underlying global address.
   */
  constexpr dart_gptr_t dart_gptr() const noexcept
  {
    return _rbegin_gptr;
  }

  /**
   * Pointer offset difference operator.
   */
  constexpr index_type operator-(const self_t &rhs) const noexcept
  {
    return dash::distance(rhs, *this);
  }

  /**
   * Prefix increment operator.
   */
  self_t &operator++()
  {
    increment(1);
    return *this;
  }

  /**
   * Postfix increment operator.
   */
  self_t operator++(int)
  {
    self_t res(*this);
    increment(1);
    return res;
  }

  /**
   * Pointer increment operator.
   */
  self_t operator+(index_type n) const
  {
    self_t res(*this);
    res += n;
    return res;
  }

  /**
   * Pointer increment operator.
   */
  self_t &operator+=(index_type n)
  {
    if (n >= 0) {
      increment(n);
    }
    else {
      decrement(-n);
    }
    return *this;
  }

  /**
   * Prefix decrement operator.
   */
  self_t &operator--()
  {
    decrement(1);
    return *this;
  }

  /**
   * Postfix decrement operator.
   */
  self_t operator--(int)
  {
    self_t res(*this);
    decrement(1);
    return res;
  }

  /**
   * Pointer decrement operator.
   */
  self_t operator-(index_type n) const
  {
    self_t res(*this);
    res -= n;
    return res;
  }

  /**
   * Pointer decrement operator.
   */
  self_t &operator-=(index_type n)
  {
    if (n >= 0) {
      decrement(n);
    }
    else {
      increment(-n);
    }
    return *this;
  }

  /**
   * Equality comparison operator.
   */
  template <class GlobPtrT>
  constexpr bool operator==(const GlobPtrT &other) const noexcept
  {
    return DART_GPTR_EQUAL(
        static_cast<dart_gptr_t>(_rbegin_gptr),
        static_cast<dart_gptr_t>(other._rbegin_gptr));
  }

  /**
   * Equality comparison operator.
   */
  constexpr bool operator==(std::nullptr_t) const noexcept
  {
    return DART_GPTR_ISNULL(static_cast<dart_gptr_t>(_rbegin_gptr));
  }

  /**
   * Inequality comparison operator.
   */
  template <class GlobPtrT>
  constexpr bool operator!=(const GlobPtrT &other) const noexcept
  {
    return !(*this == other);
  }

  /**
   * Less comparison operator.
   */
  template <class GlobPtrT>
  constexpr bool operator<(const GlobPtrT &other) const noexcept
  {
    return (other - *this) > 0;
  }

  /**
   * Less-equal comparison operator.
   */
  template <class GlobPtrT>
  constexpr bool operator<=(const GlobPtrT &other) const noexcept
  {
    return !(*this > other);
  }

  /**
   * Greater comparison operator.
   */
  template <class GlobPtrT>
  constexpr bool operator>(const GlobPtrT &other) const noexcept
  {
    return (*this - other) > 0;
  }

  /**
   * Greater-equal comparison operator.
   */
  template <class GlobPtrT>
  constexpr bool operator>=(const GlobPtrT &other) const noexcept
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
   * Conversion to local pointer.
   *
   * \returns  A native pointer to the local element referenced by this
   *           GlobPtr instance, or \c nullptr if the referenced element
   *           is not local to the calling unit.
   */
  value_type *local()
  {
    void *addr = 0;
    if (dart_gptr_getaddr(_rbegin_gptr, &addr) == DART_OK) {
      return static_cast<value_type*>(addr);
    }
    return nullptr;
  }

  /**
   * Conversion to local const pointer.
   *
   * \returns  A native pointer to the local element referenced by this
   *           GlobPtr instance, or \c nullptr if the referenced element
   *           is not local to the calling unit.
   */
  const value_type *local() const
  {
    void *addr = 0;
    if (dart_gptr_getaddr(_rbegin_gptr, &addr) == DART_OK) {
      return static_cast<const value_type *>(addr);
    }
    return nullptr;
  }

  /**
   * Set the global pointer's associated unit.
   */
  void set_unit(team_unit_t unit_id)
  {
    DASH_ASSERT_RETURNS(dart_gptr_setunit(&_rbegin_gptr, unit_id), DART_OK);
  }

  /**
   * Check whether the global pointer is in the local
   * address space the pointer's associated unit.
   */
  bool is_local() const
  {
    return dash::internal::is_local(_rbegin_gptr);
  }

  constexpr explicit operator bool() const noexcept
  {
    return !DART_GPTR_ISNULL(_rbegin_gptr);
  }

private:
  void do_increment(size_type offs, memory_space_noncontiguous)
  {
  }

  void do_increment(size_type offs, memory_space_contiguous)
  {
    if (_mem_space == nullptr || *this >= _mem_space->end()) {
      return;
    }

    auto current_uid = _rbegin_gptr.unitid;

    // current local size
    auto lsize = _mem_space->capacity(dart_team_unit_t{current_uid}) /
                 sizeof(value_type);

    // current local offset
    auto ptr_offset = _rbegin_gptr.addr_or_offs.offset / sizeof(value_type);

    // unit at global end points to (last_unit + 1, 0)
    auto const unit_end = _mem_space->end()._rbegin_gptr.unitid;

    if (offs + ptr_offset < lsize) {
      // Case A: Pointer position still in same unit space:
      _rbegin_gptr.addr_or_offs.offset += offs * sizeof(value_type);
    }
    else if (++current_uid >= unit_end) {
      // We are iterating beyond the end
      _rbegin_gptr.addr_or_offs.offset = 0;
      _rbegin_gptr.unitid = current_uid;
    }
    else {
      // Case B: jump across units and find the correct local offset

      // substract remaining local capacity
      offs -= (lsize - ptr_offset);

      // first iter
      lsize =
          _mem_space->capacity(static_cast<dash::team_unit_t>(current_uid)) /
          sizeof(value_type);

      // Skip units until we have ther the correct one or the last valid unit.
      while (offs >= lsize && current_uid < (unit_end - 1)) {
        offs -= lsize;
        ++current_uid;
        lsize = _mem_space->capacity(
                    static_cast<dash::team_unit_t>(current_uid)) /
                sizeof(value_type);
      }

      if (offs >= lsize && current_uid == unit_end - 1) {
        // This implys that we have reached unit_end and offs is greater than
        // its capacity. More precisely, we increment beyond the global end
        // and in order to prevent this we set the local offset to 0.

        // Log the number of positions beyond the global end.
        DASH_LOG_ERROR(
            "GlobPtr.increment",
            "offset goes beyond the global memory end",
            offs == lsize ? 1 : offs - lsize + 1);

        offs = 0;
        ++current_uid;
        DASH_ASSERT_EQ(
            current_uid, unit_end, "current_uid must equal unit_end");
      }

      _rbegin_gptr.addr_or_offs.offset = offs * sizeof(value_type);
      _rbegin_gptr.unitid = current_uid;
    }
  }
  void increment(size_type offs)
  {
    using memory_space_traits = dash::memory_space_traits<GlobMemT>;
    do_increment(
        offs, typename memory_space_traits::memory_space_layout_tag{});
  }

  void do_decrement(size_type offs, memory_space_contiguous)
  {
    if (_mem_space == nullptr || *this <= _mem_space->begin()) {
      return;
    }

    auto current_uid = _rbegin_gptr.unitid;

    // current local offset
    auto ptr_offset = _rbegin_gptr.addr_or_offs.offset / sizeof(value_type);

    // unit at global begin
    auto const unit_begin = _mem_space->begin()._rbegin_gptr.unitid;

    if (ptr_offset >= offs) {
      // Case A: Pointer position still in same unit space
      _rbegin_gptr.addr_or_offs.offset -= offs * sizeof(value_type);
    }
    else if (--current_uid < unit_begin) {
      // We iterate beyond the begin
      _rbegin_gptr.addr_or_offs.offset = 0;
      _rbegin_gptr.unitid = DART_UNDEFINED_UNIT_ID;
    }
    else {
      // Case B: jump across units and find the correct local offset

      // substract remaining local capacity
      offs -= ptr_offset;

      // first iter
      auto lsize = _mem_space->capacity(dart_team_unit_t{current_uid}) /
                   sizeof(value_type);

      while (offs >= lsize && current_uid > unit_begin) {
        offs -= lsize;
        --current_uid;
        lsize = _mem_space->capacity(dart_team_unit_t{current_uid}) / sizeof(value_type);
      }

      if (offs > lsize) {
        // This implys that we have reached unit_begin and offs is greater
        // than its capacity. More precisely, we decrement beyond the global
        // begin border.

        // Log the number of positions beyond the begin idx.
        DASH_LOG_ERROR(
            "GlobPtr.decrement",
            "offset goes beyond the global memory begin",
            offs - lsize);

        offs        = 0;
        current_uid = DART_UNDEFINED_UNIT_ID;
      }
      else {
        offs = lsize - offs;
      }

      _rbegin_gptr.addr_or_offs.offset = offs * sizeof(value_type);
      _rbegin_gptr.unitid = current_uid;
    }
  }

  void do_decrement(size_type offs, memory_space_noncontiguous)
  {
  }

  void decrement(size_type offs)
  {
    using memory_space_traits = dash::memory_space_traits<GlobMemT>;
    do_decrement(
        offs, typename memory_space_traits::memory_space_layout_tag{});
  }
};



template <typename T, class MemSpaceT>
std::ostream &operator<<(std::ostream &os, const GlobPtr<T, MemSpaceT> &gptr)
{
  std::ostringstream ss;
  char               buf[100];
  sprintf(
      buf,
      "u%06X|f%02X|s%04X|t%04X|o%016lX",
      gptr._rbegin_gptr.unitid,
      gptr._rbegin_gptr.flags,
      gptr._rbegin_gptr.segid,
      gptr._rbegin_gptr.teamid,
      gptr._rbegin_gptr.addr_or_offs.offset);
  ss << "dash::GlobPtr<" << typeid(T).name() << ">(" << buf << ")";
  return operator<<(os, ss.str());
}

#ifndef DOXYGEN

/**
 * Internal type.
 *
 * Specialization of \c dash::GlobPtr, wraps underlying global address
 * as global const pointer (not pointer to const).
 *
 * As pointer arithmetics are inaccessible for const pointer types,
 * no coupling to global memory space is required.
 *
 * TODO: Will be replaced by specialization of GlobPtr for global
 *       memory space tagged as unit-scope address space
 *       (see GlobUnitMem).
 */
template <typename T, class MemSpaceT>
class GlobConstPtr : GlobPtr<T, MemSpaceT>  // NOTE: non-public subclassing
{
  typedef GlobConstPtr<T, MemSpaceT> self_t;
  typedef GlobPtr<T, MemSpaceT>      base_t;

public:
  typedef T                           value_type;
  typedef typename base_t::const_type const_type;
  typedef typename base_t::index_type index_type;
  typedef typename base_t::gptrdiff_t gptrdiff_t;

  template <typename T_, class _MemSpaceT>
  friend std::ostream &operator<<(
      std::ostream &os, const GlobConstPtr<T_, _MemSpaceT> &gptr);

public:
  /**
   * Default constructor.
   *
   * Pointer arithmetics are undefined for the created instance.
   */
  constexpr GlobConstPtr()
    : base_t(nullptr, DART_GPTR_NULL)
  {
  }

  /**
   * Constructor for conversion of std::nullptr_t.
   */
  explicit constexpr GlobConstPtr(std::nullptr_t p)
    : base_t(nullptr, DART_GPTR_NULL)
  {
  }

  /**
   * Constructor, wraps underlying global address without coupling
   * to global memory space.
   *
   * Pointer arithmetics are undefined for the created instance.
   */
  explicit constexpr GlobConstPtr(dart_gptr_t gptr)
    : base_t(nullptr, gptr)
  {
  }

  /**
   * Copy constructor.
   */
  constexpr GlobConstPtr(const self_t &other) = default;

  /**
   * Move constructor.
   */
  constexpr GlobConstPtr(self_t &&other) = default;

  /**
   * Assignment operator.
   */
  self_t &operator=(const self_t &rhs) = default;

  /**
   * Move-assignment operator.
   */
  self_t &operator=(self_t &&rhs) = default;

  value_type *local()
  {
    return base_t::local();
  }

  const value_type *local() const
  {
    return base_t::local();
  }

  bool is_local() const
  {
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

  explicit operator value_type *()
  {
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
  constexpr bool operator==(const GlobPtrT &other) const noexcept
  {
    return base_t::operator==(other);
  }

  /**
   * Inequality comparison operator.
   */
  template <class GlobPtrT>
  constexpr bool operator!=(const GlobPtrT &other) const noexcept
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
  constexpr bool operator<(const GlobPtrT &other) const noexcept
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
  constexpr bool operator<=(const GlobPtrT &other) const noexcept
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
  constexpr bool operator>(const GlobPtrT &other) const noexcept
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
  constexpr bool operator>=(const GlobPtrT &other) const noexcept
  {
    return base_t::operator>=(other);
  }
};



template <typename T, class MemSpaceT>
std::ostream &operator<<(
    std::ostream &os, const GlobConstPtr<T, MemSpaceT> &gptr)
{
  std::ostringstream ss;
  char               buf[100];
  sprintf(
      buf,
      "u%06X|f%02X|s%04X|t%04X|o%016lX",
      gptr.dart_gptr().unitid,
      gptr.dart_gptr().flags,
      gptr.dart_gptr().segid,
      gptr.dart_gptr().teamid,
      gptr.dart_gptr().addr_or_offs.offset);
  ss << "dash::GlobConstPtr<" << typeid(T).name() << ">(" << buf << ")";
  return operator<<(os, ss.str());
}

#endif  // DOXYGEN

/**
 * Specialization of \c dash::distance for \c dash::GlobPtr as default
 * definition of pointer distance in global memory spaces.
 *
 * Equivalent to \c (gend - gbegin).
 *
 * \note
 * Defined with independent value types T1 and T2 to allow calculation
 * of distance between \c GlobPtr<T> and \c GlobPtr<const T>.
 * The pointer value types must have identical size.
 *
 * \todo
 * Validate compatibility of memory space types using memory space traits
 * once they are available.
 *
 * \return  Number of elements in the range between the first and second
 *          global pointer
 *
 * \concept{DashMemorySpaceConcept}
 */
template <class T, class MemSpaceT>
dash::gptrdiff_t distance(
    // First global pointer in range
    const GlobPtr<T, MemSpaceT> &gbegin,
    // Final global pointer in range
    const GlobPtr<T, MemSpaceT> &gend)
{
  using index_type = dash::gptrdiff_t;
  using val_type_b = typename std::decay<decltype(gbegin)>::type::value_type;
  using val_type_e = typename std::decay<decltype(gend)>::type::value_type;
  using value_type = val_type_b;

  static_assert(
      sizeof(val_type_b) == sizeof(val_type_e),
      "value types of global pointers are not compatible for dash::distance");

  // Both pointers in same unit space:
  if (gbegin._rbegin_gptr.unitid == gend._rbegin_gptr.unitid ||
      gbegin._mem_space == nullptr) {
    auto offset_end =
        static_cast<dash::gptrdiff_t>(gend._rbegin_gptr.addr_or_offs.offset);
    auto offset_begin =
        static_cast<dash::gptrdiff_t>(gbegin._rbegin_gptr.addr_or_offs.offset);

    return (offset_end - offset_begin) / dash::gptrdiff_t{sizeof(value_type)};
  }
  // If unit of begin pointer is after unit of end pointer,
  // return negative distance with swapped argument order:
  if (gbegin._rbegin_gptr.unitid > gend._rbegin_gptr.unitid) {
    return -(dash::distance(gend, gbegin));
  }
  // Pointers span multiple unit spaces, accumulate sizes of
  // local unit memory ranges in the pointer range:
  index_type const remaining_dist_begin_unit =
    //remaining capacity of this unit in bytes
      (gbegin._mem_space->capacity(dash::team_unit_t{gbegin._rbegin_gptr.unitid}) -
      gbegin._rbegin_gptr.addr_or_offs.offset)
      // get number of elements
      / sizeof(value_type);

  index_type const remaining_dist_end_unit =
      (gend._rbegin_gptr.addr_or_offs.offset / sizeof(value_type));

  //sum remainders of begin and end unit
  index_type dist = remaining_dist_begin_unit + remaining_dist_end_unit;

  //accumulate units in between
  for (auto u = static_cast<dash::team_unit_t>(gbegin._rbegin_gptr.unitid + 1);
       u < gend._rbegin_gptr.unitid;
       ++u) {
    dist += gend._mem_space->capacity(u) / sizeof(value_type);
  }
  return dist;
}

/**
 * Resolve the number of elements between two global pointers.
 *
 * \tparam      ElementType  Type of the elements in the range
 * \complexity  O(1)
 *
 * \ingroup     Algorithms
 *
 * \concept{DashMemorySpaceConcept}
 */
template <typename ElementType, class MemSpaceT>
dash::default_index_t distance(
    /// Global pointer to the initial position in the global range
    dart_gptr_t first,
    /// Global pointer to the final position in the global range
    dart_gptr_t last)
{
  GlobPtr<ElementType, MemSpaceT> gptr_first(first);
  GlobPtr<ElementType, MemSpaceT> gptr_last(last);
  return gptr_last - gptr_first;
}

}  // namespace dash

#endif  // DASH__GLOB_PTR_H_
