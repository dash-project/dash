#ifndef DASH__VIEW__VIEW_TRAITS_H__INCLUDED
#define DASH__VIEW__VIEW_TRAITS_H__INCLUDED


namespace dash {

template <class ViewT>
struct view_traits {

  // alternative: specialize struct view_traits for DimDiff := 0
  constexpr static bool is_projection = (ViewT::dimdiff != 0);

};

/**
 * Inverse operation to \c dash::apply.
 *
 */
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
