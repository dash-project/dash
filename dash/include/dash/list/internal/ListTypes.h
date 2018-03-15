#ifndef DASH__INTERNAL__LIST__LIST_TYPES_H__INCLUDED
#define DASH__INTERNAL__LIST__LIST_TYPES_H__INCLUDED

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_globmem.h>

namespace dash {
namespace internal {

template<typename ElementType>
struct ListNode
{
private:
  typedef ListNode<ElementType> self_t;

public:
  ElementType  value;
  //TODO: this should be a global node pointer
  self_t     * lprev = nullptr;
  self_t     * lnext = nullptr;
  dart_gptr_t  gprev = DART_GPTR_NULL;
  dart_gptr_t  gnext = DART_GPTR_NULL;
};

} // namespace internal
} // namespace dash

#endif // DASH__INTERNAL__LIST__LIST_TYPES_H__INCLUDED
