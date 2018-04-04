#ifndef STD_EXPERIMENTAL_BITS_ALLOCATOR_H
#define STD_EXPERIMENTAL_BITS_ALLOCATOR_H

namespace std {
namespace experimental {
inline namespace executors_v1 {
namespace execution {
namespace allocator_impl {

template<class Derived>
struct property_base
{
  static constexpr bool is_requirable = true;
  static constexpr bool is_preferable = true;

  template<class Executor, class Type = decltype(Executor::query(*static_cast<Derived*>(0)))>
    static constexpr Type static_query_v = Executor::query(Derived());
};

} // namespace allocator_impl

template<class ProtoAllocator>
struct allocator_t : allocator_impl::property_base<allocator_t<ProtoAllocator>>
{
  constexpr explicit allocator_t(const ProtoAllocator& a) : alloc_(a) {}
  constexpr ProtoAllocator value() const { return alloc_; }
  ProtoAllocator alloc_;
};

template<>
struct allocator_t<void> : allocator_impl::property_base<allocator_t<void>>
{
  template<class ProtoAllocator>
  allocator_t<ProtoAllocator> operator()(const ProtoAllocator& a) const
  {
    return allocator_t<ProtoAllocator>(a);
  }
};

constexpr allocator_t<void> allocator;

} // namespace execution
} // inline namespace executors_v1
} // namespace experimental
} // namespace std

#endif // STD_EXPERIMENTAL_BITS_ALLOCATOR_H
