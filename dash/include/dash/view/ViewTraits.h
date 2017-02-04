#ifndef DASH__VIEW__VIEW_TRAITS_H__INCLUDED
#define DASH__VIEW__VIEW_TRAITS_H__INCLUDED

#include <type_traits>

#include <dash/Range.h>


namespace dash {

#ifdef DOXYGEN

/**
 * Returns a reference to the specified object's domain, or the object
 * itself if it is not a View type.
 * Inverse operation to \c dash::apply.
 *
 * \concept{DashViewConcept}
 */
template <class Viewable>
constexpr typename Viewable::domain_type &
domain(const Viewable & v);

/**
 * View type traits.
 *
 * \concept{DashViewConcept}
 */
template <class ViewT>
struct view_traits
{
  typedef typename ViewT::domain_type         domain_type;
  typedef typename ViewT::image_type           image_type;
  typedef typename ViewT::origin_type         origin_type;
  typedef typename ViewT::local_type           local_type;
  typedef typename ViewT::global_type         global_type;

  typedef std::integral_constant<bool, value> is_origin;
  typedef std::integral_constant<bool, value> is_view;
  typedef std::integral_constant<bool, value> is_projection;
  typedef std::integral_constant<bool, value> is_local;
};

#else // DOXYGEN

template <class ViewableType>
struct view_traits;

template <class ViewType>
class IndexSetIdentity;

namespace detail {

  template<typename T>
  struct _has_domain_type
  {
  private:
    typedef char                      yes;
    typedef struct { char array[2]; } no;

    template<typename C> static yes test(typename C::domain_type*);
    template<typename C> static no  test(...);
  public:
    static constexpr bool value = sizeof(test<T>(0)) == sizeof(yes);
  };

} // namespace detail

template <class ViewableType>
struct is_view : dash::detail::_has_domain_type<ViewableType> { };



namespace detail {

  // ------------------------------------------------------------------------

  template <
    class ViewableType,
    bool  IsView,
    bool  IsRange >
  struct _view_traits { };

  /**
   * Specialization of \c dash::view_traits for view types.
   */
  template <class ViewT>
  struct _view_traits<ViewT, true, true>
  {
    typedef std::integral_constant<bool, false>                is_projection;
    typedef std::integral_constant<bool, true>                 is_view;
    /// Whether the view is the origin domain.
    typedef std::integral_constant<bool, false>                is_origin;

    typedef typename ViewT::index_type                            index_type;
    typedef typename ViewT::index_set_type                    index_set_type;
    typedef typename ViewT::domain_type                          domain_type;
    typedef std::integral_constant<bool,
      // either view type is local:
      ViewT::is_local::value ||
      // or view domain type is local:
      _view_traits<domain_type,
                   dash::is_view<domain_type>::value,
                   dash::is_range<domain_type>::value
                  >::is_local::value >                         is_local;

    typedef typename ViewT::local_type                            local_type;
    typedef typename ViewT::global_type                          global_type;
    typedef typename std::conditional<is_local::value,
                                      local_type,
                                      global_type >::type         image_type;
    typedef typename dash::view_traits<domain_type>::origin_type origin_type;
  };

  /**
   * Specialization of \c dash::view_traits for container types.
   */
  template <class ContainerT>
  struct _view_traits<ContainerT, false, true> {
    typedef ContainerT                                           origin_type;
    typedef ContainerT                                           domain_type;
    typedef ContainerT                                            image_type;
    typedef ContainerT                                           global_type;
    typedef typename ContainerT::local_type                       local_type;
    typedef typename ContainerT::index_type                       index_type;
    typedef typename dash::IndexSetIdentity<ContainerT>       index_set_type;

    /// Whether the view type is a projection (has less dimensions than the
    /// view's domain type).
    typedef std::integral_constant<bool, false>                is_projection;
    typedef std::integral_constant<bool, false>                is_view;
    /// Whether the view is the origin domain.
    typedef std::integral_constant<bool, true>                 is_origin;
    /// Whether the view / container type is a local view.
    /// \note A container type is local if it is identical to its
    ///       \c local_type
    typedef std::integral_constant<bool, std::is_same<
                                   ContainerT,
                                   typename ContainerT::local_type
                                  >::value >                   is_local;
  };

} // namespace detail

/**
 *
 * \concept{DashViewConcept}
 */
template <class ViewableType>
struct view_traits
: detail::_view_traits<
    ViewableType,
    dash::is_view<ViewableType>::value,
    dash::is_range<ViewableType>::value > {
};

#endif // DOXYGEN

} // namespace dash

#endif // DASH__VIEW__VIEW_TRAITS_H__INCLUDED
