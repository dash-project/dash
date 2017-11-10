#ifndef DASH__UTIL__INDEX_SEQUENCE_H__INCLUDED
#define DASH__UTIL__INDEX_SEQUENCE_H__INCLUDED

#include <dash/Types.h>


// Consider:
// http://ldionne.com/2015/11/29/efficient-parameter-pack-indexing/

namespace dash {
namespace ce {


/**
 * Represents a compile-time sequence container
 */
template <
  std::size_t... Is >
struct index_sequence
{ };

/**
 * Generates a compile-sequence integer (size_t) sequence in ascending order
 *
 * Example:
 *
 * template<std::size_t... Is>
 * void print(dash::ce::index_sequence<Is...>&& s)
 * {
 *   for (auto i : { Is... }) std::cout << i << " ";
 *   std::cout << "\n";
 * }
 *
 * print(dash::ce::make_rev_index_sequence<5>());
 * //output: 0 1 2 3 4
 *
 */
template <
  std::size_t    N,
  std::size_t... Is >
struct make_index_sequence
  : make_index_sequence<N-1, N-1, Is...>
{ };

/**
 * Stop condition for a compile-time integer (size_t) sequence in ascending
 * order
 */
template <
  std::size_t... Is>
struct make_index_sequence<0, Is...>
  : index_sequence<Is...>
{ };

/**
 * Generates a compile-sequence integer (size_t) sequence in descending order
 *
 * Example:
 *
 * template<std::size_t... Is>
 * void print(dash::ce::index_sequence<Is...>&& s)
 * {
 *   for (auto i : { Is... }) std::cout << i << " ";
 *   std::cout << "\n";
 * }
 *
 * print(dash::ce::make_rev_index_sequence<5>());
 * //output: 4 3 2 1 0
 *
 */
template <
  std::size_t    N,
  std::size_t... Is >
struct make_rev_index_sequence
  : make_rev_index_sequence<N-1, Is..., N-1>
{ };


/**
 * Stop condition for a compile-time integer (size_t) sequence in descending
 * order
 */
template <
  std::size_t... Is>
struct make_rev_index_sequence<0, Is...>
  : index_sequence<Is...>
{ };

} // namespace ce
} // namespace dash

#endif // DASH__UTIL__INDEX_SEQUENCE_H__INCLUDED
