#ifndef DASH__ITERATOR__INTERNAL__GLOBPTR_BASE_H__INCLUDED
#define DASH__ITERATOR__INTERNAL__GLOBPTR_BASE_H__INCLUDED

#include <iterator>
#include <numeric>
#include <type_traits>

#include <dash/dart/if/dart_globmem.h>
#include <dash/dart/if/dart_types.h>

#include <dash/Exception.h>
#include <dash/memory/MemorySpaceBase.h>

namespace dash {

template <class T, class MemSpaceT>
class GlobPtr;

namespace internal {

template <class LHS, class RHS>
struct is_pointer_assignable {
  static constexpr bool value = std::is_assignable<LHS &, RHS &>::value;
};

template <class T>
struct is_pointer_assignable<void, T> {
  static constexpr bool value = std::true_type::value;
};

template <class T>
struct is_pointer_assignable<const void, T> {
  static constexpr bool value = std::true_type::value;
};

template <class U>
struct is_pointer_assignable<U, void> {
  static constexpr bool value = !std::is_const<U>::value;
};

template <>
struct is_pointer_assignable<void, void> {
  static constexpr bool value = std::true_type::value;
};

static bool is_local(dart_gptr_t gptr)
{
  dart_team_unit_t luid;
  DASH_ASSERT_RETURNS(dart_team_myid(gptr.teamid, &luid), DART_OK);
  return gptr.unitid == luid.id;
}

template <class T, class MemSpaceT>
dash::gptrdiff_t distance(
    dart_gptr_t gbegin,
    dart_gptr_t gend,
    MemSpaceT const * mem_space,
    // Contiguous Tag
    memory_space_noncontiguous);

template <class T, class MemSpaceT>
dash::gptrdiff_t distance(
    dart_gptr_t gbegin,
    dart_gptr_t gend,
    MemSpaceT const * mem_space,
    // Contiguous Tag
    memory_space_contiguous);

template <class T, class MemorySpaceT>
dart_gptr_t increment(
    dart_gptr_t dart_gptr,
    typename MemorySpaceT::size_type,
    MemorySpaceT const *mem_space,
    memory_space_noncontiguous) DASH_NOEXCEPT;

template <class T, class MemorySpaceT>
dart_gptr_t increment(
    dart_gptr_t dart_gptr,
    typename MemorySpaceT::size_type,
    MemorySpaceT const *mem_space,
    memory_space_contiguous) DASH_NOEXCEPT;

template <class T, class MemorySpaceT>
dart_gptr_t decrement(
    dart_gptr_t dart_gptr,
    typename MemorySpaceT::size_type,
    MemorySpaceT const *mem_space,
    memory_space_contiguous) DASH_NOEXCEPT;

template <class T, class MemorySpaceT>
dart_gptr_t decrement(
    dart_gptr_t dart_gptr,
    typename MemorySpaceT::size_type,
    MemorySpaceT const *mem_space,
    memory_space_noncontiguous) DASH_NOEXCEPT;

template <class T, class MemSpaceT>
inline dash::gptrdiff_t distance(
    // First global pointer in range
    dart_gptr_t first,
    // Final global pointer in range
    dart_gptr_t last,
    // The underlying memory space
    MemSpaceT const *mem_space,
    // Contiguous Tag
    memory_space_noncontiguous)
{
  DASH_THROW(dash::exception::NotImplemented, "not implemented yet");
}

template <class T, class MemSpaceT>
inline dash::gptrdiff_t distance(
    // First global pointer in range
    dart_gptr_t first,
    // Final global pointer in range
    dart_gptr_t last,
    // The underlying memory space
    MemSpaceT const *mem_space,
    // Contiguous Tag
    memory_space_contiguous)
{
  using index_type = typename GlobPtr<T, MemSpaceT>::index_type;
  using value_type = T;

  DASH_ASSERT_EQ(
      first.teamid,
      last.teamid,
      "cannot calculate difference between two pointers which do not belong "
      "to the same DART segment");

  DASH_ASSERT_EQ(
      first.segid,
      last.segid,
      "cannot calculate difference between two pointers which do not belong "
      "to the same DART segment");

  bool is_reverse = false;

  if (first.unitid == last.unitid || mem_space == nullptr) {
    // Both pointers in same unit space:
    auto offset_end   = static_cast<index_type>(last.addr_or_offs.offset);
    auto offset_begin = static_cast<index_type>(first.addr_or_offs.offset);

    return (offset_end - offset_begin) /
           static_cast<index_type>(sizeof(value_type));
  }
  else if (first.unitid > last.unitid) {
    // If unit of begin pointer is after unit of end pointer,
    // return negative distance with swapped argument order:
    std::swap(first, last);
    is_reverse = true;
  }

  // Pointers span multiple unit spaces, accumulate sizes of
  // local unit memory ranges in the pointer range:
  index_type const capacity_unit_begin =
      mem_space->capacity(dash::team_unit_t{first.unitid});

  index_type const remainder_unit_begin =
      // remaining capacity of this unit in bytes
      (capacity_unit_begin - first.addr_or_offs.offset)
      // get number of elements
      / sizeof(value_type);

  index_type const remainder_unit_end =
      (last.addr_or_offs.offset / sizeof(value_type));

  // sum remainders of begin and end unit
  index_type dist = remainder_unit_begin + remainder_unit_end;

  if (last.unitid - first.unitid > 1) {
    // accumulate units in between
    std::vector<dash::team_unit_t> units_in_between(
        last.unitid - first.unitid - 1);
    std::iota(
        std::begin(units_in_between),
        std::end(units_in_between),
        dash::team_unit_t{first.unitid + 1});

    dist = std::accumulate(
        std::begin(units_in_between),
        std::end(units_in_between),
        dist,
        [&mem_space](const index_type &total, const dash::team_unit_t &unit) {
          return total + mem_space->capacity(unit) / sizeof(value_type);
        });
  }
  return is_reverse ? -dist : dist;
}

template <class T, class MemorySpaceT>
dart_gptr_t increment(
    dart_gptr_t dart_gptr,
    typename MemorySpaceT::size_type offs,
    MemorySpaceT const * /*mem_space*/,
    memory_space_noncontiguous) DASH_NOEXCEPT
{
  dart_gptr.addr_or_offs.offset += offs * sizeof(T);
  return dart_gptr;
}


template <class T, class MemorySpaceT>
dart_gptr_t increment(
    dart_gptr_t                      gptr,
    typename MemorySpaceT::size_type offs,
    MemorySpaceT const *             mem_space,
    memory_space_contiguous) DASH_NOEXCEPT
{
  using value_type = T;

  auto const gend = static_cast<dart_gptr_t>(mem_space->end());

  if (mem_space == nullptr ||
      distance<T>(gptr, gend, mem_space, memory_space_contiguous{}) <= 0) {
    return gptr;
  }

  auto current_uid = static_cast<dash::team_unit_t>(gptr.unitid);

  // current local size
  auto lsize = mem_space->capacity(current_uid) / sizeof(value_type);

  // current local offset
  auto ptr_offset = gptr.addr_or_offs.offset / sizeof(value_type);

  // unit at global end points to (last_unit + 1, 0)
  auto const unit_end = static_cast<dash::team_unit_t>(gend.unitid);

  if (offs + ptr_offset < lsize) {
    // Case A: Pointer position still in same unit space:
    gptr.addr_or_offs.offset += offs * sizeof(value_type);
  }
  else if (++current_uid >= unit_end) {
    // We are iterating beyond the end
    gptr.addr_or_offs.offset = 0;
    gptr.unitid              = current_uid;
  }
  else {
    // Case B: jump across units and find the correct local offset

    // substract remaining local capacity
    offs -= (lsize - ptr_offset);

    // first iter
    lsize = mem_space->capacity(current_uid) / sizeof(value_type);

    // Skip units until we have ther the correct one or the last valid unit.
    while (offs >= lsize && current_uid < (unit_end - 1)) {
      offs -= lsize;
      ++current_uid;
      lsize = mem_space->capacity(current_uid) / sizeof(value_type);
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

    gptr.addr_or_offs.offset = offs * sizeof(value_type);
    gptr.unitid              = current_uid;
  }

  return gptr;
}

template <class T, class MemorySpaceT>
dart_gptr_t decrement(
    dart_gptr_t gptr,
    typename MemorySpaceT::size_type offs,
    MemorySpaceT const *mem_space,
    memory_space_noncontiguous) DASH_NOEXCEPT
{
  DASH_THROW(dash::exception::NotImplemented, "not implemented yet");
}

template <class T, class MemorySpaceT>
dart_gptr_t decrement(
    dart_gptr_t gptr,
    typename MemorySpaceT::size_type offs,
    MemorySpaceT const *mem_space,
    memory_space_contiguous) DASH_NOEXCEPT
{
  using value_type = T;

  auto const gbegin = static_cast<dart_gptr_t>(mem_space->begin());

  if (mem_space == nullptr ||
      distance<T>(gptr, gbegin, mem_space, memory_space_contiguous{}) >= 0) {
    return gptr;
  }

  auto current_uid = static_cast<dash::team_unit_t>(gptr.unitid);

  // current local offset
  auto ptr_offset = gptr.addr_or_offs.offset / sizeof(value_type);

  // unit at global begin
  auto const unit_begin = static_cast<dash::team_unit_t>(gbegin.unitid);

  if (ptr_offset >= offs) {
    // Case A: Pointer position still in same unit space
    gptr.addr_or_offs.offset -= offs * sizeof(value_type);
  }
  else if (--current_uid < unit_begin) {
    // We iterate beyond the begin
    gptr.addr_or_offs.offset = 0;
    gptr.unitid              = DART_UNDEFINED_UNIT_ID;
  }
  else {
    // Case B: jump across units and find the correct local offset

    // substract remaining local capacity
    offs -= ptr_offset;

    // first iter
    auto lsize = mem_space->capacity(dart_team_unit_t{current_uid}) /
                 sizeof(value_type);

    while (offs >= lsize && current_uid > unit_begin) {
      offs -= lsize;
      --current_uid;
      lsize = mem_space->capacity(dart_team_unit_t{current_uid}) /
              sizeof(value_type);
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

    gptr.addr_or_offs.offset = offs * sizeof(value_type);
    gptr.unitid              = current_uid;
  }
  return gptr;
}

}  // namespace internal

}  // namespace dash
#endif
