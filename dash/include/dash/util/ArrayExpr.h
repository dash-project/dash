#ifndef DASH__UTIL__ARRAY_EXPR_H__INCLUDED
#define DASH__UTIL__ARRAY_EXPR_H__INCLUDED

#include <dash/util/IndexSequence.h>

#include <array>
#include <initializer_list>


namespace dash {
namespace ce {

template <
  class ValueT,
  int   NElem,
  int   PivotIdx >
class split_array
{
  typedef dash::ce::split_array<ValueT, NElem, PivotIdx> self_t;

  constexpr static int NElemLeft  = PivotIdx;
  constexpr static int NElemRight = NElem - PivotIdx;

  const std::array<ValueT, NElemLeft>    _lpart;
  const std::array<ValueT, NElemRight>   _rpart;

public:
  constexpr split_array(
    const std::initializer_list<ValueT> & values)
    : split_array(
        values,
        dash::ce::make_index_sequence<NElemLeft>(),
        dash::ce::make_index_sequence<NElemRight>())
  { }

  constexpr const std::array<ValueT, NElemLeft> & left() const {
    return _lpart;
  }

  constexpr const std::array<ValueT, NElemLeft> & right() const {
    return _rpart;
  }

private:
  template <
    int... LIs,
    int... RIs >
  constexpr split_array(
    const std::initializer_list<ValueT> & values,
    dash::ce::index_sequence<LIs...>,
    dash::ce::index_sequence<RIs...>)
  : _lpart( {{ (*(values.begin() + LIs))... }} )
  , _rpart( {{ (*(values.begin() + PivotIdx + RIs))... }} )
  { }
};

} // namespace ce
} // namespace dash

#endif // DASH__UTIL__ARRAY_EXPR_H__INCLUDED
