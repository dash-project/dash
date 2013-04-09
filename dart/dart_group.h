/*
 * dart_group.h
 *
 *  Created on: Apr 3, 2013
 *      Author: maierm
 */

#ifndef DART_GROUP_H_
#define DART_GROUP_H_

#include <stddef.h>

/*
 DART groups are objects with local meaning only.
 they are essentially objects representing sets of units
 out of which later teams can be formed. The operations to
 manipulate groups are local (and cheap). The operations to create
 teams are collective and can be expensive.
 The
 */

/* DART groups are represented by an opague type */
struct dart_group_struct;
typedef struct dart_group_struct dart_group_t;

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
