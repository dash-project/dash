#ifndef DASH__UTIL__INDEX_SEQUENCE_H__INCLUDED
#define DASH__UTIL__INDEX_SEQUENCE_H__INCLUDED

#include <dash/Types.h>


namespace dash {

template <
  default_index_t... Is >
struct IndexSequence
{ };

template <
  default_index_t    N,
  default_index_t... Is >
struct make_index_sequence
  : make_index_sequence<N-1, N-1, Is...>
{ };

template <
  default_index_t... Is>
struct make_index_sequence<0, Is...>
  : IndexSequence<Is...>
{ };

} // namespace dash

#endif // DASH__UTIL__INDEX_SEQUENCE_H__INCLUDED
