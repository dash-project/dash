#ifndef DASH__INTERNAL__ALLOCATOR__GLOB_DYNAMIC_MEM_TYPES_H__INCLUDED
#define DASH__INTERNAL__ALLOCATOR__GLOB_DYNAMIC_MEM_TYPES_H__INCLUDED

namespace dash {
namespace internal {

template<
  typename SizeType,
  typename ElementType >
struct glob_dynamic_mem_bucket_type
{
  SizeType            size;
  ElementType       * lptr;
  dart_gptr_t         gptr;
  bool                attached = false;
};

} // namespace internal
} // namespace dash

#endif // DASH__INTERNAL__ALLOCATOR__GLOB_DYNAMIC_MEM_TYPES_H__INCLUDED
