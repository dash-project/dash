#ifndef DASH__UTIL__ARRAY_EXPR_H__INCLUDED
#define DASH__UTIL__ARRAY_EXPR_H__INCLUDED

#include <dash/util/IndexSequence.h>

#include <array>
#include <initializer_list>


namespace dash {
namespace ce {

// -------------------------------------------------------------------------
// drop
// -------------------------------------------------------------------------

namespace detail {
  template <
    class          ValueT,
    std::size_t    NElem,
    std::size_t    NDrop,
    std::size_t... Is >
  constexpr std::array<ValueT, NElem - NDrop>
  drop_impl(
    const std::array<ValueT, NElem> & values,
    dash::ce::index_sequence<Is...>) {
    return {{ std::get<NDrop + Is>(values)... }};
  }
} // namespace detail

/**
 * Drops \c d elements from a given sequence of \c N elements with indices
 * \c (0..d..N).
 */
template <
  class         ValueT,
  std::size_t   NElem,
  std::size_t   NDrop >
constexpr std::array<ValueT, NElem - NDrop>
drop(
  const std::array<ValueT, NElem> & values) {
  return detail::drop_impl<ValueT, NElem, NDrop>(
           values,
           dash::ce::make_index_sequence<NElem - NDrop>());
}

// -------------------------------------------------------------------------
// take
// -------------------------------------------------------------------------

namespace detail {
  template <
    class          ValueT,
    std::size_t    NElem,
    std::size_t    NTake,
    std::size_t... Is >
  constexpr std::array<ValueT, NTake>
  take_impl(
    const std::array<ValueT, NElem> & values,
    dash::ce::index_sequence<Is...>) {
    return {{ std::get<Is>(values)... }};
  }
} // namespace detail

/**
 * Returns sequence of first \c t elements from a given sequence of size
 * \c N with indices \c (0..t..N).
 */
template <
  class         ValueT,
  std::size_t   NElem,
  std::size_t   NTake >
constexpr std::array<ValueT, NTake>
take(
  const std::array<ValueT, NElem> & values) {
  return detail::take_impl<ValueT, NElem, NTake>(
           values,
           dash::ce::make_index_sequence<NTake>());
}

// -------------------------------------------------------------------------
// split
// -------------------------------------------------------------------------

template <
  class         ValueT,
  std::size_t   NElem,
  std::size_t   PivotIdx >
class split
{
  typedef dash::ce::split<ValueT, NElem, PivotIdx> self_t;

  constexpr static std::size_t NElemLeft  = PivotIdx;
  constexpr static std::size_t NElemRight = NElem - PivotIdx;

  const std::array<ValueT, NElem> _values;

public:
  constexpr split(
    const std::initializer_list<ValueT> & values)
    : _values(values)
  { }

  constexpr split(
    const std::array<ValueT, NElem> & values)
    : _values(values)
  { }

  constexpr std::array<ValueT, NElemLeft> left() const {
    return take<ValueT, NElem, PivotIdx>(_values);
  }

  constexpr std::array<ValueT, NElemRight> right() const {
    return drop<ValueT, NElem, PivotIdx>(_values);
  }
};

// -------------------------------------------------------------------------
// append
// -------------------------------------------------------------------------

namespace detail {
  template <
    class          ValueT,
    std::size_t    NElemLeft,
    std::size_t    NElemRight,
    std::size_t... LIs,
    std::size_t... RIs >
  constexpr std::array<ValueT, NElemLeft + NElemRight>
  append_impl(
    const std::array<ValueT, NElemLeft>  & left,
    const std::array<ValueT, NElemRight> & right,
    dash::ce::index_sequence<LIs...>,
    dash::ce::index_sequence<RIs...>) {
    return {
             ( std::get<LIs>(left)  )... ,
             ( std::get<RIs>(right) )...
           };
  }
} // namespace detail

/**
 * Concatenates two lists.
 */
template <
  class          ValueT,
  std::size_t    NElemLeft,
  std::size_t    NElemRight >
constexpr std::array<ValueT, NElemLeft + NElemRight>
append(
  const std::array<ValueT, NElemLeft>  & left,
  const std::array<ValueT, NElemRight> & right) {
  return detail::append_impl(
           left,
           right,
           dash::ce::make_index_sequence<NElemLeft>(),
           dash::ce::make_index_sequence<NElemRight>());
}

/**
 * Appends element to end of list.
 */
template <
  class          ValueT,
  std::size_t    NElemLeft >
constexpr std::array<ValueT, NElemLeft + 1>
append(
  const std::array<ValueT, NElemLeft>  & left,
  const ValueT                         & elem) {
  return detail::append_impl(
           left,
           std::array<ValueT, 1>({ elem }),
           dash::ce::make_index_sequence<NElemLeft>(),
           dash::ce::make_index_sequence<1>());
}

} // namespace ce
} // namespace dash

#endif // DASH__UTIL__ARRAY_EXPR_H__INCLUDED
