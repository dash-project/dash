#ifndef DART_GASPI_GROUP_IMPL_H
#define DART_GASPI_GROUP_IMPL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_team_group.h>

#define MAXSIZE_GROUP 256
#define INVALID_GASPI_GROUP -1

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

#ifdef __cplusplus
}
#endif

#endif /* DART_GASPI_GROUP_IMPL_H */
