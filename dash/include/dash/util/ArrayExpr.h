#ifndef DASH__UTIL__ARRAY_EXPR_H__INCLUDED
#define DASH__UTIL__ARRAY_EXPR_H__INCLUDED

#include <dash/util/IndexSequence.h>

#include <array>
#include <initializer_list>


namespace dash {
namespace ce {


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

template <
  class         ValueT,
  std::size_t   NElem,
  std::size_t   NDrop >
constexpr std::array<ValueT, NElem - NDrop>
drop(
  const std::array<ValueT, NElem> & values) {
  return drop_impl<ValueT, NElem, NDrop>(
           values,
           dash::ce::make_index_sequence<NElem - NDrop>());
}



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

template <
  class         ValueT,
  std::size_t   NElem,
  std::size_t   NTake >
constexpr std::array<ValueT, NTake>
take(
  const std::array<ValueT, NElem> & values) {
  return take_impl<ValueT, NElem, NTake>(
           values,
           dash::ce::make_index_sequence<NTake>());
}



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

} // namespace ce
} // namespace dash

#endif // DASH__UTIL__ARRAY_EXPR_H__INCLUDED
