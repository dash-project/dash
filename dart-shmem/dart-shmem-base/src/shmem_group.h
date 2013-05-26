/*
 * shmem_group.h
 *
 *  Created on: Apr 9, 2013
 *      Author: maierm
 */

#ifndef SHMEM_GROUP_H_
#define SHMEM_GROUP_H_
#ifdef __cplusplus
extern "C" {
#endif

#include "dart/dart_return_codes.h"
#include "dart/dart_group.h"

// a simple data structure to represent subsets of units
// and to facilitate simple set operations on them
// this simple approach will only work for very small group sizes
// but should be sufficient for a shmem implementation
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
#endif /* SHMEM_GROUP_H_ */
