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

  typedef typename detail::allocator_traits_difference_type<Alloc>::type
      difference_type;
  typedef typename detail::allocator_traits_size_type<Alloc>::type
      size_type;
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

  template <class U>
  using rebind_alloc = typename allocator_type::template rebind<U>::other;

  template <class U>
  using rebind_traits = dash::allocator_traits<rebind_alloc<U>>;
  */

  /*
  static pointer allocate(allocator_type& a, size_type n)
  {
    return a.allocate(n);
  }

  static void deallocate(allocator_type& a, pointer p, size_type n)
  {
    a.deallocate(p, n);
  }
  */
};

}  // namespace dash

#endif  // DASH__ALLOCATOR__ALLOCATOR_TRAITS_H__INCLUDED
