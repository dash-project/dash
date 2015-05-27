#ifndef DART_GROUPS_IMPL_H_INCLUDED
#define DART_GROUPS_IMPL_H_INCLUDED

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_team_group.h>

#include <dash/dart/shmem/extern_c.h>
EXTERN_C_BEGIN

#define MAXSIZE_GROUP 256

// 
// a simple data structure to represent subsets of units
// and to facilitate simple set operations on them.
//
// this simple approach will only work for very small group sizes
// but should be sufficient for the shmem implementation
//
struct dart_group_struct {
  // current number of members in the group
  int nmem;
  
  // g2l is indexed by global unit ids, l2g is
  // indexed by local ids, both arrays are initialized to -1
  // and values >= 0 indicate a valid entry
  
  // l2g[i] gives the global unit id for local id i
  // g2l[j] gives the local unit id for global id j
  int g2l[MAXSIZE_GROUP];
  int l2g[MAXSIZE_GROUP];
};


EXTERN_C_END


#endif /* DART_GROUPS_IMPL_H_INCLUDED */
