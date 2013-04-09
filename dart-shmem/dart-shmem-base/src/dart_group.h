/*
 * dart_group.h
 *
 *  Created on: Apr 3, 2013
 *      Author: maierm
 */

#ifndef DART_GROUP_H_
#define DART_GROUP_H_

#include <stddef.h>

// max. number of members in a group
#define MAXSIZE_GROUP 100

// a simple data structure to represent subsets of units
// and to facilitate simple set operations on them
// this simple approach will only work for very small group sizes
// but should be sufficient for a shmem implementation
typedef struct dart_group_struct
{
	// current number of members in the group
	int nmem;

	// g2l is indexed by global unit ids, l2g is
	// indexed by local ids, both arrays are initialized to -1
	// and values >= 0 indicate a valid entry

	// l2g[i] gives the global unit id for local id i
	// g2l[j] gives the local unit id for global id j
	int g2l[MAXSIZE_GROUP];
	int l2g[MAXSIZE_GROUP];
} dart_group_t;

int dart_group_init(dart_group_t *g);
int dart_group_fini(dart_group_t *g);

int dart_group_copy(const dart_group_t *gin, dart_group_t *gout);
int dart_group_union(const dart_group_t *g1, const dart_group_t *g2,
		dart_group_t *gout);
int dart_group_intersect(const dart_group_t *g1, const dart_group_t *g2,
		dart_group_t *gout);
int dart_group_addmember(dart_group_t *g, int unitid);
int dart_group_delmember(dart_group_t *g, int unitid);
int dart_group_ismember(const dart_group_t *g, int unitid);
int dart_group_split(const dart_group_t *g, size_t n, dart_group_t *gout);

#endif /* DART_GROUP_H_ */
