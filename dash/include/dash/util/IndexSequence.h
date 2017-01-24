#ifndef DASH__UTIL__INDEX_SEQUENCE_H__INCLUDED
#define DASH__UTIL__INDEX_SEQUENCE_H__INCLUDED

#include <dash/Types.h>


// Consider:
// http://ldionne.com/2015/11/29/efficient-parameter-pack-indexing/

namespace dash {
namespace ce {

template <
  std::size_t... Is >
struct index_sequence
{ };

template <
  std::size_t    N,
  std::size_t... Is >
struct make_index_sequence
  : make_index_sequence<N-1, N-1, Is...>
{ };

template <
  std::size_t... Is>
struct make_index_sequence<0, Is...>
  : index_sequence<Is...>
{ };

} // namespace ce
} // namespace dash

#endif // DASH__UTIL__INDEX_SEQUENCE_H__INCLUDED
