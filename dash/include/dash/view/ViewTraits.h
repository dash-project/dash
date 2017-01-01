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
    static constexpr bool value = sizeof(test<T>(0)) == sizeof(yes);
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
    typedef ContainerT                                          origin_type;
    
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

#ifdef DOXYGEN

/**
 * Returns a reference to the specified object's origin, or the object
 * itself if it is not a View type.
 * Inverse operation to \c dash::apply.
 *
 */
template <class Viewable>
constexpr typename Viewable::origin_type &
origin(const Viewable & v);

#else // DOXYGEN

// ------------------------------------------------------------------------
// dash::origin(View)

template <class ViewT>
inline typename std::enable_if<
  detail::_is_view<ViewT>::value,
  const typename ViewT::origin_type &
>::type
origin(const ViewT & view) {
  return view.origin();
}

template <class ViewT>
inline typename std::enable_if<
  detail::_is_view<ViewT>::value,
  typename ViewT::origin_type &
>::type
origin(ViewT & view) {
  return view.origin();
}

// ------------------------------------------------------------------------
// dash::origin(Container)

template <class ContainerT>
constexpr typename std::enable_if<
  !detail::_is_view<ContainerT>::value,
  const ContainerT &
>::type
origin(const ContainerT & container) {
  return container;
}

template <class ContainerT>
inline typename std::enable_if<
  !detail::_is_view<ContainerT>::value,
  ContainerT &
>::type
origin(ContainerT & container) {
  return container;
}

#endif // DOXYGEN

/**
 * Inverse operation to \c dash::origin.
 *
 */
template <class ViewTypeA, class ViewTypeB>
constexpr auto apply(
  ViewTypeA & view_a,
  ViewTypeB & view_b) -> decltype(view_a.apply(view_b)) {
  return view_a.apply(view_b);
}


} // namespace dash

#endif // DASH__VIEW__VIEW_TRAITS_H__INCLUDED
