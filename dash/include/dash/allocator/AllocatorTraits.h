#ifndef DASH__ALLOCATOR__ALLOCATOR_TRAITS_H__INCLUDED
#define DASH__ALLOCATOR__ALLOCATOR_TRAITS_H__INCLUDED

#include <dash/Meta.h>
#include <dash/memory/MemorySpace.h>

#include <memory>

namespace dash {

namespace detail {

DASH__META__DEFINE_TRAIT__HAS_TYPE(local_pointer)

template <class _Alloc, bool = has_type_local_pointer<_Alloc>::value>
struct allocator_traits_local_pointer {
  typedef typename _Alloc::local_pointer type;
};

template <class _Alloc>
struct allocator_traits_local_pointer<_Alloc, false> {
  typedef typename std::allocator_traits<
      typename _Alloc::local_allocator_type>::pointer type;
};

DASH__META__DEFINE_TRAIT__HAS_TYPE(const_local_pointer)

template <class _Alloc, bool = has_type_const_local_pointer<_Alloc>::value>
struct allocator_traits_const_local_pointer {
  typedef typename _Alloc::const_local_pointer type;
};

template <class _Alloc>
struct allocator_traits_const_local_pointer<_Alloc, false> {
  typedef typename std::allocator_traits<
      typename _Alloc::local_allocator_type>::const_pointer type;
};

DASH__META__DEFINE_TRAIT__HAS_TYPE(local_void_pointer)

template <class _Alloc, bool = has_type_local_void_pointer<_Alloc>::value>
struct allocator_traits_local_void_pointer {
  typedef typename _Alloc::local_void_pointer type;
};

template <class _Alloc>
struct allocator_traits_local_void_pointer<_Alloc, false> {
  typedef typename std::allocator_traits<
      typename _Alloc::local_allocator_type>::void_pointer type;
};

DASH__META__DEFINE_TRAIT__HAS_TYPE(const_local_void_pointer)

template <
    class _Alloc,
    bool = has_type_const_local_void_pointer<_Alloc>::value>
struct allocator_traits_const_local_void_pointer {
  typedef typename _Alloc::const_local_void_pointer type;
};

template <class _Alloc>
struct allocator_traits_const_local_void_pointer<_Alloc, false> {
  typedef typename std::allocator_traits<
      typename _Alloc::local_allocator_type>::const_void_pointer type;
};

DASH__META__DEFINE_TRAIT__HAS_TYPE(pointer)

template <class _Alloc, bool = has_type_pointer<_Alloc>::value>
struct allocator_traits_pointer {
  typedef typename _Alloc::pointer type;
};

template <class _Alloc>
struct allocator_traits_pointer<_Alloc, false> {
  typedef dart_gptr_t type;
};

DASH__META__DEFINE_TRAIT__HAS_TYPE(const_pointer)

template <class _Alloc, bool = has_type_const_pointer<_Alloc>::value>
struct allocator_traits_const_pointer {
  typedef typename _Alloc::const_pointer type;
};

template <class _Alloc>
struct allocator_traits_const_pointer<_Alloc, false> {
  typedef const dart_gptr_t type;
};

DASH__META__DEFINE_TRAIT__HAS_TYPE(void_pointer)

template <class _Alloc, bool = has_type_void_pointer<_Alloc>::value>
struct allocator_traits_void_pointer {
  typedef typename _Alloc::void_pointer type;
};

template <class _Alloc>
struct allocator_traits_void_pointer<_Alloc, false> {
  typedef dart_gptr_t type;
};

DASH__META__DEFINE_TRAIT__HAS_TYPE(const_void_pointer)

template <class _Alloc, bool = has_type_const_void_pointer<_Alloc>::value>
struct allocator_traits_const_void_pointer {
  typedef typename _Alloc::const_void_pointer type;
};

template <class _Alloc>
struct allocator_traits_const_void_pointer<_Alloc, false> {
  typedef const dart_gptr_t type;
};

DASH__META__DEFINE_TRAIT__HAS_TYPE(difference_type)

template <class _Alloc, bool = has_type_difference_type<_Alloc>::value>
struct allocator_traits_difference_type {
  typedef typename _Alloc::difference_type type;
};

template <class _Alloc>
struct allocator_traits_difference_type<_Alloc, false> {
  typedef dash::gptrdiff_t type;
};

DASH__META__DEFINE_TRAIT__HAS_TYPE(size_type)

template <class _Alloc, bool = has_type_size_type<_Alloc>::value>
struct allocator_traits_size_type {
  typedef typename _Alloc::size_type type;
};

template <class _Alloc>
struct allocator_traits_size_type<_Alloc, false> {
  typedef dash::default_size_t type;
};
}  // namespace detail

template <class Alloc>
struct allocator_traits {
  // Mandatory definitions
  typedef Alloc                                allocator_type;
  typedef typename Alloc::value_type           value_type;
  typedef typename Alloc::allocation_policy    allocation_policy;
  typedef typename Alloc::local_allocator_type local_allocator;

  // Optional Definitions
  typedef typename detail::allocator_traits_difference_type<Alloc>::type
                                                                   difference_type;
  typedef typename detail::allocator_traits_size_type<Alloc>::type size_type;

  // Global Allocator Traits
  typedef typename detail::allocator_traits_pointer<Alloc>::type pointer;
  typedef typename detail::allocator_traits_const_pointer<Alloc>::type
      const_pointer;
  typedef typename detail::allocator_traits_void_pointer<Alloc>::type
      void_pointer;
  typedef typename detail::allocator_traits_const_void_pointer<Alloc>::type
      const_void_pointer;

  // Local Allocator Traits
  typedef typename detail::allocator_traits_local_pointer<Alloc>::type
      local_pointer;
  typedef typename detail::allocator_traits_const_local_pointer<Alloc>::type
      const_local_pointer;
  typedef typename detail::allocator_traits_local_void_pointer<Alloc>::type
      local_void_pointer;
  typedef
      typename detail::allocator_traits_const_local_void_pointer<Alloc>::type
          const_local_void_pointer;

  /*
  typedef
      typename dash::pointer_traits<pointer>::template rebind<const
  value_type> const_pointer; typedef typename
  dash::pointer_traits<pointer>::template rebind<void> void_pointer; typedef
  typename dash::pointer_traits<pointer>::template rebind<const void>
      const_void_pointer;

  typedef typename allocator_type::const_pointer      const_pointer;
  typedef typename allocator_type::void_pointer       void_pointer;
  typedef typename allocator_type::const_void_pointer const_void_pointer;

  typedef typename allocator_type::difference_type difference_type;
  typedef typename allocator_type::size_type       size_type;

  typedef
      typename dash::pointer_traits<pointer>::difference_type difference_type;
  typedef typename std::make_unsigned<difference_type>::type size_type;

  */

  template <class U>
  using rebind_alloc = typename allocator_type::template rebind<U>;

  template <class U>
  using rebind_traits = dash::allocator_traits<rebind_alloc<U>>;

  static pointer allocate(allocator_type& a, size_type n)
  {
    return a.allocate(n);
  }

  static void deallocate(allocator_type& a, pointer p, size_type n)
  {
    a.deallocate(p, n);
  }
};

}  // namespace dash

#endif  // DASH__ALLOCATOR__ALLOCATOR_TRAITS_H__INCLUDED
