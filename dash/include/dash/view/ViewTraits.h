#ifndef DASH__VIEW__VIEW_TRAITS_H__INCLUDED
#define DASH__VIEW__VIEW_TRAITS_H__INCLUDED

#include <type_traits>


namespace dash {

/**
 * Specialization of \c dash::view_traits for container types.
 *
 */
template <
  class ContainerT,
  class LocalT =
          typename std::enable_if<
            !std::is_same<void, ContainerT>::value,
            typename ContainerT::local_type
          >::type >
struct view_traits {
  /// Whether the view type is a projection (has less dimensions than the
  /// view origin).
  std::integral_constant<bool, false>                 is_projection;
  /// Whether the view type is the view origin.
  std::integral_constant<bool, true>                  is_origin;
  /// Whether the view / container type is a local view.
  /// \note A container type is local if it is identical to its \c local_type
  std::integral_constant<bool, std::is_same<ContainerT, LocalT>::value >
                                                      is_local;
};

template <class ViewT>
struct view_traits<ViewT, void> {
  /// \note Alternative: specialize struct view_traits for \c (DimDiff := 0)
  std::integral_constant<bool, (ViewT::dimdiff != 0)> is_projection;
  std::integral_constant<bool, false>                 is_origin;
  std::integral_constant<bool, ViewT::is_local ||
                               view_traits<typename ViewT::origin_type>
                                 ::is_local::value >  is_local;
};

/**
 * Inverse operation to \c dash::apply.
 *
 */

template <class ViewT>
const typename ViewT::origin_type & origin(const ViewT & view) {
  return view.origin();
}

template <class ViewT>
typename ViewT::origin_type & origin(ViewT & view) {
  return view.origin();
}

/**
 * Inverse operation to \c dash::origin.
 *
 */
template <class ViewTypeA, class ViewTypeB>
auto apply(
  ViewTypeA & view_a,
  ViewTypeB & view_b) -> decltype(view_a.apply(view_b)) {
  return view_a.apply(view_b);
}


} // namespace dash

#endif // DASH__VIEW__VIEW_TRAITS_H__INCLUDED
