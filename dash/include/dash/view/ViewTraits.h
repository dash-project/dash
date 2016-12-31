#ifndef DASH__VIEW__VIEW_TRAITS_H__INCLUDED
#define DASH__VIEW__VIEW_TRAITS_H__INCLUDED

#include <type_traits>


namespace dash {

namespace detail {

  template<typename T>
  struct _has_origin_type
  {
  private:
    typedef char                      yes;
    typedef struct { char array[2]; } no;

    template<typename C> static yes test(typename C::origin_type*);
    template<typename C> static no  test(...);
  public:
    static const bool value = sizeof(test<T>(0)) == sizeof(yes);
  };

  template <class ViewableType>
  struct _is_view : _has_origin_type<ViewableType> { };

  // ------------------------------------------------------------------------

  template <
    class ViewableType,
    bool  IsView >
  struct _view_traits { };

  /**
   * Specialization of \c dash::view_traits for view types.
   */
  template <class ViewT>
  struct _view_traits< ViewT, true >
  {
    typedef typename ViewT::origin_type                         origin_type;

    /// \note Alternative: specialize struct view_traits for \c (DimDiff = 0)
    typedef std::integral_constant<bool, (ViewT::dimdiff != 0)> is_projection;
    typedef std::integral_constant<bool, true>                  is_view;
    typedef std::integral_constant<bool, false>                 is_origin;
    typedef std::integral_constant<bool,
      // either view type is local:
      ViewT::is_local::value ||
      // or view origin type is local:
      _view_traits<origin_type,
                   _is_view<origin_type>::value
                  >::is_local::value >                          is_local;
  };

  /**
   * Specialization of \c dash::view_traits for container types.
   */
  template <class ContainerT>
  struct _view_traits< ContainerT, false > {
    /// Whether the view type is a projection (has less dimensions than the
    /// view's origin type).
    typedef std::integral_constant<bool, false>                 is_projection;
    typedef std::integral_constant<bool, false>                 is_view;
    /// Whether the view type is the view origin.
    typedef std::integral_constant<bool, true>                  is_origin;
    /// Whether the view / container type is a local view.
    /// \note A container type is local if it is identical to its
    ///       \c local_type
    typedef std::integral_constant<bool, std::is_same<
                                   ContainerT,
                                   typename ContainerT::local_type
                                  >::value >                    is_local;
  };

} // namespace detail


template <class ViewableType>
struct view_traits
: detail::_view_traits<
    ViewableType,
    detail::_is_view<ViewableType>::value > {
};

/**
 * Inverse operation to \c dash::apply.
 *
 */
template <class ViewT>
const typename ViewT::origin_type & origin(const ViewT & view) {
  return view.origin();
}

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
