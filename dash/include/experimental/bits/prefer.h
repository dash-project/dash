#ifndef STD_EXPERIMENTAL_BITS_PREFER_H
#define STD_EXPERIMENTAL_BITS_PREFER_H

#include <experimental/bits/has_require_member.h>
#include <experimental/bits/has_prefer_free.h>
#include <experimental/bits/is_constexpr_present.h>

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {
namespace prefer_impl {

struct prefer_fn
{
  template<class Executor, class Property>
  constexpr auto operator()(Executor&& ex, Property&&) const
    -> typename std::enable_if<std::decay<Property>::type::is_preferable
      && is_constexpr_present_impl::eval<typename std::decay<Executor>::type, typename std::decay<Property>::type>::value,
        typename std::decay<Executor>::type>::type
  {
    return std::forward<Executor>(ex);
  }

  template<class Executor, class Property>
  constexpr auto operator()(Executor&& ex, Property&& p) const
    noexcept(noexcept(std::forward<Executor>(ex).require(std::forward<Property>(p))))
    -> typename std::enable_if<std::decay<Property>::type::is_preferable
      && !is_constexpr_present_impl::eval<typename std::decay<Executor>::type, typename std::decay<Property>::type>::value,
        decltype(std::forward<Executor>(ex).require(std::forward<Property>(p)))>::type
  {
    return std::forward<Executor>(ex).require(std::forward<Property>(p));
  }

  template<class Executor, class Property>
  constexpr auto operator()(Executor&& ex, Property&& p) const
    noexcept(noexcept(prefer(std::forward<Executor>(ex), std::forward<Property>(p))))
    -> typename std::enable_if<std::decay<Property>::type::is_preferable
      && !is_constexpr_present_impl::eval<typename std::decay<Executor>::type, typename std::decay<Property>::type>::value
        && !has_require_member_impl::eval<typename std::decay<Executor>::type, typename std::decay<Property>::type>::value
          && has_prefer_free_impl::eval<typename std::decay<Executor>::type, typename std::decay<Property>::type>::value,
            decltype(prefer(std::forward<Executor>(ex), std::forward<Property>(p)))>::type
  {
    return prefer(std::forward<Executor>(ex), std::forward<Property>(p));
  }

  template<class Executor, class Property>
  constexpr auto operator()(Executor&& ex, Property&&) const noexcept
    -> typename std::enable_if<std::decay<Property>::type::is_preferable
      && !is_constexpr_present_impl::eval<typename std::decay<Executor>::type, typename std::decay<Property>::type>::value
        && !has_require_member_impl::eval<typename std::decay<Executor>::type, typename std::decay<Property>::type>::value
          && !has_prefer_free_impl::eval<typename std::decay<Executor>::type, typename std::decay<Property>::type>::value,
            typename std::decay<Executor>::type>::type
  {
    return std::forward<Executor>(ex);
  }

  template<class Executor, class Property0, class Property1, class... PropertyN>
  constexpr auto operator()(Executor&& ex, Property0&& p0, Property1&& p1, PropertyN&&... pn) const
    noexcept(noexcept(std::declval<prefer_fn>()(std::declval<prefer_fn>()(std::forward<Executor>(ex), std::forward<Property0>(p0)), std::forward<Property1>(p1), std::forward<PropertyN>(pn)...)))
    -> decltype(std::declval<prefer_fn>()(std::declval<prefer_fn>()(std::forward<Executor>(ex), std::forward<Property0>(p0)), std::forward<Property1>(p1), std::forward<PropertyN>(pn)...))
  {
    return (*this)((*this)(std::forward<Executor>(ex), std::forward<Property0>(p0)), std::forward<Property1>(p1), std::forward<PropertyN>(pn)...);
  }
};

template<class T = prefer_fn> constexpr T customization_point{};

} // namespace prefer_impl
} // namespace execution
} // inline namespace executors_v1
} // namespace experimental
} // namespace std

#endif // STD_EXPERIMENTAL_BITS_PREFER_H
