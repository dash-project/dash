#ifndef STD_EXPERIMENTAL_BITS_PREFER_ONLY_H
#define STD_EXPERIMENTAL_BITS_PREFER_ONLY_H

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {
namespace prefer_only_impl {

template<class>
struct type_check
{
  typedef void type;
};

template<class InnerProperty, class = void>
struct prefer_only_base {};

template<class InnerProperty>
struct prefer_only_base<InnerProperty,
  typename type_check<typename InnerProperty::polymorphic_query_result_type>::type>
{
  using polymorphic_query_result_type =
    typename InnerProperty::polymorphic_query_result_type;
};

template<class, class InnerProperty>
inline auto property_value(const InnerProperty& property)
  noexcept(noexcept(std::declval<const InnerProperty>().value()))
    -> decltype(std::declval<const InnerProperty>().value())
{
  return property.value();
}

} // namespace prefer_only_impl

template<class InnerProperty>
struct prefer_only : prefer_only_impl::prefer_only_base<InnerProperty>
{
  InnerProperty property;

  static constexpr bool is_requirable = false;
  static constexpr bool is_preferable = InnerProperty::is_preferable;

  template<class Executor, class Type = decltype(InnerProperty::template static_query_v<Executor>)>
    static constexpr Type static_query_v = InnerProperty::template static_query_v<Executor>;

  constexpr prefer_only(const InnerProperty& p) : property(p) {}

  template<class Dummy = int>
  constexpr auto value() const
    noexcept(noexcept(prefer_only_impl::property_value<Dummy, InnerProperty>()))
      -> decltype(prefer_only_impl::property_value<Dummy, InnerProperty>())
  {
    return prefer_only_impl::property_value(property);
  }

  template<class Executor, class Property, class = typename std::enable_if<std::is_same<Property, prefer_only>::value>::type>
  friend auto prefer(Executor ex, const Property& p)
    noexcept(noexcept(execution::prefer(std::move(ex), std::declval<const InnerProperty>())))
      -> decltype(execution::prefer(std::move(ex), std::declval<const InnerProperty>()))
  {
    return execution::prefer(std::move(ex), p.property);
  }

  template<class Executor, class Property, class = typename std::enable_if<std::is_same<Property, prefer_only>::value>::type>
  friend constexpr auto query(const Executor& ex, const Property& p)
    noexcept(noexcept(execution::query(ex, std::declval<const InnerProperty>())))
    -> decltype(execution::query(ex, std::declval<const InnerProperty>()))
  {
    return execution::query(ex, p.property);
  }
};

} // namespace execution
} // inline namespace executors_v1
} // namespace experimental
} // namespace std

#endif // STD_EXPERIMENTAL_BITS_PREFER_ONLY_H
