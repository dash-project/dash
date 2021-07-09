#ifndef DASH__MEMORY__INTERNAL__GLOB_HEAP_TYPES_H__INCLUDED
#define DASH__MEMORY__INTERNAL__GLOB_HEAP_TYPES_H__INCLUDED

#include <dash/dart/if/dart_globmem.h>

namespace dash {
namespace internal {

template<
  typename SizeType,
  typename ElementType >
struct glob_dynamic_mem_bucket_type
{
  SizeType      allocated_size;
  SizeType      size;
  ElementType * lptr;
  dart_gptr_t   gptr;
  bool          attached;
};

template<
  typename SizeType,
  typename ElementType >
struct glob_dynamic_contiguous_mem_bucket_type
{
  SizeType      size;
  ElementType * lptr;
};

} // namespace internal
} // namespace dash

#endif // DASH__MEMORY__INTERNAL__GLOB_HEAP_TYPES_H__INCLUDED
