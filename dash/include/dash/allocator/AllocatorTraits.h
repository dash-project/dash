#ifndef DASH__ALLOCATOR__ALLOCATOR_TRAITS_H__INCLUDED
#define DASH__ALLOCATOR__ALLOCATOR_TRAITS_H__INCLUDED

#include <dash/memory/MemorySpace.h>

#include <memory>


namespace dash {



struct collective_allocator_tag { };

struct noncollective_allocator_tag { };

template <class Allocator>
struct allocator_traits : public std::allocator_traits<Allocator>
{
  typedef typename Allocator::allocator_category      allocator_category;

  /* std::allocator_traits takes care of this stuff */
  /*
  typedef typename allocator_type::value_type                   value_type;
  typedef typename allocator_type::pointer                         pointer;

  typedef typename
      dash::pointer_traits<pointer>::template rebind<const value_type>
    const_pointer;
  typedef typename
      dash::pointer_traits<pointer>::template rebind<void>
    void_pointer;
  typedef typename
      dash::pointer_traits<pointer>::template rebind<const void>
    const_void_pointer;

  typedef typename dash::pointer_traits<pointer>::difference_type
    difference_type;
  typedef typename std::make_unsigned<difference_type>::type
    size_type;

  template <class U>
  using rebind_alloc  = typename Allocator::template rebind<U>::other;

  template <class U>
  using rebind_traits = dash::allocator_traits<
                          typename Allocator::template rebind<U> >;
                          */

};

} // namespace dash

#endif // DASH__ALLOCATOR__ALLOCATOR_TRAITS_H__INCLUDED
