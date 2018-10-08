#ifndef DASH__GLOB_PTR_H_
#define DASH__GLOB_PTR_H_

#include <cstddef>
#include <iostream>
#include <numeric>
#include <sstream>

#include <dash/dart/if/dart.h>

#include <dash/Exception.h>
#include <dash/Init.h>

#include <dash/atomic/Type_traits.h>

#include <dash/internal/Logging.h>
#include <dash/iterator/internal/GlobPtrBase.h>

#include <dash/memory/MemorySpaceBase.h>


std::ostream &operator<<(std::ostream &os, const dart_gptr_t &dartptr);

bool operator==(const dart_gptr_t &lhs, const dart_gptr_t &rhs);

bool operator!=(const dart_gptr_t &lhs, const dart_gptr_t &rhs);

namespace dash {

// Forward-declarations
template <typename T>
class GlobRef;

template <class T, class MemSpaceT>
class GlobPtr;

template <class T, class MemSpaceT>
dash::gptrdiff_t distance(
    GlobPtr<T, MemSpaceT> gbegin, GlobPtr<T, MemSpaceT> gend);

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
      GlobPtr<T, MemSpaceT> gbegin, GlobPtr<T, MemSpaceT> gend);

private:
  // Raw global pointer used to initialize this pointer instance
  dart_gptr_t m_dart_pointer = DART_GPTR_NULL;
  // Memory space refernced by this global pointer
  const GlobMemT *m_mem_space{nullptr};

protected:
  /**
   * Constructor, specifies underlying global address.
   *
   * Allows to instantiate GlobPtr without a valid memory
   * space reference.
   */
  GlobPtr(const GlobMemT *mem_space, dart_gptr_t gptr, index_type rindex = 0)
    : m_dart_pointer(gptr)
    , m_mem_space(mem_space)
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
    : m_dart_pointer(gptr)
    , m_mem_space(&mem_space)
  {
  }

  /**
   * Constructor for conversion of std::nullptr_t.
   */
  explicit constexpr GlobPtr(std::nullptr_t)
    : m_dart_pointer(DART_GPTR_NULL)
    , m_mem_space(nullptr)
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
    : m_dart_pointer(other.m_dart_pointer)
    , m_mem_space(reinterpret_cast<const GlobMemT *>(other.m_mem_space))
  {
  }

  /**
   * Assignment operator.
   */
  template <typename T, class MemSpaceT>
  self_t &operator=(const GlobPtr<T, MemSpaceT> &other)
  {
    m_dart_pointer = other.m_dart_pointer;
    m_mem_space   = reinterpret_cast<const GlobMemT *>(other.m_mem_space);
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
    return m_dart_pointer;
  }

  /**
   * The pointer's underlying global address.
   */
  constexpr dart_gptr_t dart_gptr() const noexcept
  {
    return m_dart_pointer;
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
  //template <class GlobPtrT>
  constexpr bool operator==(const GlobPtr &other) const noexcept
  {
    return DART_GPTR_EQUAL(
        static_cast<dart_gptr_t>(m_dart_pointer),
        static_cast<dart_gptr_t>(other.m_dart_pointer));
  }

  /**
   * Equality comparison operator.
   */
  constexpr bool operator==(std::nullptr_t) const noexcept
  {
    return DART_GPTR_ISNULL(static_cast<dart_gptr_t>(m_dart_pointer));
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
    if (dart_gptr_getaddr(m_dart_pointer, &addr) == DART_OK) {
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
    if (dart_gptr_getaddr(m_dart_pointer, &addr) == DART_OK) {
      return static_cast<const value_type *>(addr);
    }
    return nullptr;
  }

  /**
   * Set the global pointer's associated unit.
   */
  void set_unit(team_unit_t unit_id)
  {
    DASH_ASSERT_RETURNS(dart_gptr_setunit(&m_dart_pointer, unit_id), DART_OK);
  }

  /**
   * Check whether the global pointer is in the local
   * address space the pointer's associated unit.
   */
  bool is_local() const
  {
    return dash::internal::is_local(m_dart_pointer);
  }

  constexpr explicit operator bool() const noexcept
  {
    return !DART_GPTR_ISNULL(m_dart_pointer);
  }

private:
  void do_increment(size_type offs, memory_space_noncontiguous)
  {

  }

  void do_increment(size_type offs, memory_space_contiguous)
  {
    if (m_mem_space == nullptr || *this >= m_mem_space->end()) {
      return;
    }

    auto current_uid = m_dart_pointer.unitid;

    // current local size
    auto lsize = m_mem_space->capacity(dart_team_unit_t{current_uid}) /
                 sizeof(value_type);

    // current local offset
    auto ptr_offset = m_dart_pointer.addr_or_offs.offset / sizeof(value_type);

    // unit at global end points to (last_unit + 1, 0)
    auto const unit_end = m_mem_space->end().m_dart_pointer.unitid;

    if (offs + ptr_offset < lsize) {
      // Case A: Pointer position still in same unit space:
      m_dart_pointer.addr_or_offs.offset += offs * sizeof(value_type);
    }
    else if (++current_uid >= unit_end) {
      // We are iterating beyond the end
      m_dart_pointer.addr_or_offs.offset = 0;
      m_dart_pointer.unitid = current_uid;
    }
    else {
      // Case B: jump across units and find the correct local offset

      // substract remaining local capacity
      offs -= (lsize - ptr_offset);

      // first iter
      lsize =
          m_mem_space->capacity(static_cast<dash::team_unit_t>(current_uid)) /
          sizeof(value_type);

      // Skip units until we have ther the correct one or the last valid unit.
      while (offs >= lsize && current_uid < (unit_end - 1)) {
        offs -= lsize;
        ++current_uid;
        lsize = m_mem_space->capacity(
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

      m_dart_pointer.addr_or_offs.offset = offs * sizeof(value_type);
      m_dart_pointer.unitid = current_uid;
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
    if (m_mem_space == nullptr || *this <= m_mem_space->begin()) {
      return;
    }

    auto current_uid = m_dart_pointer.unitid;

    // current local offset
    auto ptr_offset = m_dart_pointer.addr_or_offs.offset / sizeof(value_type);

    // unit at global begin
    auto const unit_begin = m_mem_space->begin().m_dart_pointer.unitid;

    if (ptr_offset >= offs) {
      // Case A: Pointer position still in same unit space
      m_dart_pointer.addr_or_offs.offset -= offs * sizeof(value_type);
    }
    else if (--current_uid < unit_begin) {
      // We iterate beyond the begin
      m_dart_pointer.addr_or_offs.offset = 0;
      m_dart_pointer.unitid = DART_UNDEFINED_UNIT_ID;
    }
    else {
      // Case B: jump across units and find the correct local offset

      // substract remaining local capacity
      offs -= ptr_offset;

      // first iter
      auto lsize = m_mem_space->capacity(dart_team_unit_t{current_uid}) /
                   sizeof(value_type);

      while (offs >= lsize && current_uid > unit_begin) {
        offs -= lsize;
        --current_uid;
        lsize = m_mem_space->capacity(dart_team_unit_t{current_uid}) / sizeof(value_type);
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

      m_dart_pointer.addr_or_offs.offset = offs * sizeof(value_type);
      m_dart_pointer.unitid = current_uid;
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
      gptr.m_dart_pointer.unitid,
      gptr.m_dart_pointer.flags,
      gptr.m_dart_pointer.segid,
      gptr.m_dart_pointer.teamid,
      gptr.m_dart_pointer.addr_or_offs.offset);
  ss << "dash::GlobPtr<" << typeid(T).name() << ">(" << buf << ")";
  return operator<<(os, ss.str());
}

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
*
 * \return  Number of elements in the range between the first and second
 *          global pointer
 *
 * \concept{DashMemorySpaceConcept}
 */
template <class T, class MemSpaceT>
dash::gptrdiff_t distance(
    // First global pointer in range
    GlobPtr<T, MemSpaceT> gbegin,
    // Final global pointer in range
    GlobPtr<T, MemSpaceT> gend)
{
  using index_type = typename GlobPtr<T, MemSpaceT>::index_type;
  using value_type = T;

  DASH_ASSERT_EQ(
      gbegin.m_dart_pointer.teamid,
      gend.m_dart_pointer.teamid,
      "cannot calculate difference between two pointers which do not belong "
      "to the same DART segment");

  DASH_ASSERT_EQ(
      gbegin.m_dart_pointer.segid,
      gend.m_dart_pointer.segid,
      "cannot calculate difference between two pointers which do not belong "
      "to the same DART segment");

  bool is_reverse = false;

  auto unit_at_begin =
      static_cast<dash::team_unit_t>(gbegin.m_dart_pointer.unitid);

  auto unit_at_end =
      static_cast<dash::team_unit_t>(gend.m_dart_pointer.unitid);

  if (unit_at_begin == unit_at_end || gbegin.m_mem_space == nullptr) {
    // Both pointers in same unit space:
    auto offset_end =
        static_cast<index_type>(gend.m_dart_pointer.addr_or_offs.offset);
    auto offset_begin =
        static_cast<index_type>(gbegin.m_dart_pointer.addr_or_offs.offset);

    return (offset_end - offset_begin) /
           static_cast<index_type>(sizeof(value_type));
  }
  else if (unit_at_begin > unit_at_end) {
    // If unit of begin pointer is after unit of end pointer,
    // return negative distance with swapped argument order:
    std::swap(gbegin, gend);
    std::swap(unit_at_begin, unit_at_end);
    is_reverse = true;
  }

  auto const &mem_space = *(gbegin.m_mem_space);

  // Pointers span multiple unit spaces, accumulate sizes of
  // local unit memory ranges in the pointer range:
  index_type const capacity_unit_begin =
      mem_space.capacity(dash::team_unit_t{unit_at_begin});

  index_type const remainder_unit_begin =
      // remaining capacity of this unit in bytes
      (capacity_unit_begin - gbegin.m_dart_pointer.addr_or_offs.offset)
      // get number of elements
      / sizeof(value_type);

  index_type const remainder_unit_end =
      (gend.m_dart_pointer.addr_or_offs.offset / sizeof(value_type));

  // sum remainders of begin and end unit
  index_type dist = remainder_unit_begin + remainder_unit_end;

  if (unit_at_end - unit_at_begin > 1) {
    // accumulate units in between
    std::vector<dash::team_unit_t> units_in_between(
        unit_at_end - unit_at_begin - 1);
    std::iota(
        std::begin(units_in_between),
        std::end(units_in_between),
        dash::team_unit_t{unit_at_begin + 1});

    dist = std::accumulate(
        std::begin(units_in_between),
        std::end(units_in_between),
        dist,
        [&mem_space](const index_type &total, const dash::team_unit_t &unit) {
          return total + mem_space.capacity(unit) / sizeof(value_type);
        });
  }
  return is_reverse ? -dist : dist;
}

}  // namespace dash

#endif  // DASH__GLOB_PTR_H_
