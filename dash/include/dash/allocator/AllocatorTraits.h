#ifndef DASH__ALLOCATOR__ALLOCATOR_TRAITS_H__INCLUDED
#define DASH__ALLOCATOR__ALLOCATOR_TRAITS_H__INCLUDED

#include <dash/memory/MemorySpace.h>

#include <memory>

namespace dash {

struct collective_allocator_tag {
};

struct noncollective_allocator_tag {
};

template <class Allocator>
struct allocator_traits {

  typedef Allocator allocator_type;

  typedef typename allocator_type::allocator_category allocator_category;
  typedef typename allocator_type::local_pointer local_pointer;

  //
  typedef typename allocator_type::value_type value_type;
  typedef typename allocator_type::pointer pointer;

  /*
  typedef
      typename dash::pointer_traits<pointer>::template rebind<const value_type>
          const_pointer;
  typedef typename dash::pointer_traits<pointer>::template rebind<void>
      void_pointer;
  typedef typename dash::pointer_traits<pointer>::template rebind<const void>
      const_void_pointer;
      */

  typedef typename allocator_type::const_pointer const_pointer;
  typedef typename allocator_type::void_pointer void_pointer;
  typedef typename allocator_type::const_void_pointer const_void_pointer;

  typedef typename allocator_type::difference_type difference_type;
  typedef typename allocator_type::size_type size_type;

  /*
  typedef
      typename dash::pointer_traits<pointer>::difference_type difference_type;
  typedef typename std::make_unsigned<difference_type>::type size_type;
  */

  template <class U>
  using rebind_alloc = typename allocator_type::template rebind<U>::other;

  template <class U>
  using rebind_traits =
      dash::allocator_traits<rebind_alloc<U>>;


  static pointer allocate(allocator_type& a, size_type n)
      { return a.allocate(n); }

  static void deallocate(allocator_type& a, pointer p, size_type n)
      { a.deallocate(p, n); }

};

}  // namespace dash

#endif  // DASH__ALLOCATOR__ALLOCATOR_TRAITS_H__INCLUDED
