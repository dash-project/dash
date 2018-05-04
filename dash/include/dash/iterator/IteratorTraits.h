#ifndef DASH__ITERATOR_TRAITS_H__INCLUDED
#define DASH__ITERATOR_TRAITS_H__INCLUDED

#include <iterator>
#include <type_traits>

#include <dash/Meta.h>

#include <dash/iterator/GlobIter.h>
#include <dash/iterator/GlobViewIter.h>

namespace dash {

namespace detail {

template <typename Iterator>
struct is_global_iterator : std::false_type {
};

template <
    typename ElementType,
    class PatternType,
    class GlobMemType,
    class PointerType,
    class ReferenceType>
struct is_global_iterator<GlobViewIter<
    ElementType,
    PatternType,
    GlobMemType,
    PointerType,
    ReferenceType>> : std::true_type {
};

template <
    typename ElementType,
    class PatternType,
    class GlobMemType,
    class PointerType,
    class ReferenceType>
struct is_global_iterator<GlobIter<
    ElementType,
    PatternType,
    GlobMemType,
    PointerType,
    ReferenceType>> : std::true_type {
};

/// iterator traits index_type

DASH__META__DEFINE_TRAIT__HAS_TYPE(index_type)

template <class _Iter, bool = has_type_index_type<_Iter>::value>
struct iterator_traits_index_type {
  typedef typename _Iter::index_type type;
};

template <class _Iter>
struct iterator_traits_index_type<_Iter, false> {
  typedef dash::default_index_t type;
};

}  // namespace detail

template <typename Iterator>
struct iterator_traits : std::iterator_traits<Iterator> {
  using is_global_iterator = typename detail::is_global_iterator<Iterator>;
  using index_type =
      typename detail::iterator_traits_index_type<Iterator>::type;
};

}  // namespace dash
#endif
