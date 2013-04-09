/*
 * dart_group.c
 *
 *  Created on: Apr 3, 2013
 *      Author: maierm
 */

#include "shmem_group.h"
#include <stdio.h>

int dart_group_init(dart_group_t *group)
{
	int i;

	group->nmem = 0;
	for (i = 0; i < MAXSIZE_GROUP; i++)
	{
		(group->g2l)[i] = -1;
		(group->l2g)[i] = -1;
	}
	return DART_OK;
}

int dart_group_fini(dart_group_t *group)
{
	group->nmem = 0;
	return DART_OK;
}

int dart_group_copy(const dart_group_t *g, dart_group_t *gout)
{
	int i;

	gout->nmem = g->nmem;
	for (i = 0; i < MAXSIZE_GROUP; i++)
	{
		(gout->g2l)[i] = (g->g2l)[i];
		(gout->l2g)[i] = (g->l2g)[i];
	}
	return DART_OK;
}

void dart_group_rebuild(dart_group_t *g)
{
	int i, n;

	// rebuild the data structure, based only on the g2l array
	// if (g2l[i]>=0) then the unit with global id i is part
	// of the group.
	n = 0;
	for (i = 0; i < MAXSIZE_GROUP; i++)
	{
		if ((g->g2l[i]) >= 0)
		{
			(g->l2g)[n] = i;
			(g->g2l)[i] = n;
			n++;
		}
	}

	g->nmem = n;
}

int dart_group_union(const dart_group_t *g1, const dart_group_t *g2,
		dart_group_t *gout)
{
	int i;

	for (i = 0; i < MAXSIZE_GROUP; i++)
	{
		(gout->g2l)[i] = -1;
		if ((g1->g2l)[i] >= 0 || (g2->g2l)[i] >= 0)
		{
			// just set g2l[i] to 1 to indicate that i is a member
			// dart_group_rebuild then updates the group data structure
			(gout->g2l)[i] = 1;
		}
	}

	dart_group_rebuild(gout);
	return DART_OK;
}

int dart_group_intersect(const dart_group_t *g1, const dart_group_t *g2,
		dart_group_t *gout)
{
	int i;

	for (i = 0; i < MAXSIZE_GROUP; i++)
	{
		(gout->g2l)[i] = -1;
		if ((g1->g2l)[i] >= 0 && (g2->g2l)[i] >= 0)
		{
			// set to 1 to indicate that i is a member
			// dart_group_rebuild then updates the group data structure
			(gout->g2l)[i] = 1;
		}
	}

	dart_group_rebuild(gout);
	return DART_OK;
}

int dart_group_addmember(dart_group_t *g, int unitid)
{
	g->g2l[unitid] = 1;
	dart_group_rebuild(g);

	return DART_OK;
}

int dart_group_delmember(dart_group_t *g, int unitid)
{
	g->g2l[unitid] = -1;
	dart_group_rebuild(g);

	return DART_OK;
}

int dart_group_ismember(const dart_group_t *g, int unitid)
{
	return ((g->g2l)[unitid] >= 0);
}

int dart_group_split(const dart_group_t *g, size_t nsplits,
		dart_group_t *gsplit)
{
	int i, j, k;
	int nmem = (g->nmem);
	int bdiv = nmem / nsplits;
	int brem = nmem % nsplits;
	int bsize;

	j = 0;
	for (i = 0; i < nsplits; i++)
	{
		bsize = (i < brem) ? (bdiv + 1) : bdiv;
		dart_group_init(&(gsplit[i]));

		for (k = 0; (k < bsize) && (j < nmem); k++, j++)
		{
			(gsplit[i].g2l)[g->l2g[j]] = 1;
		}

		dart_group_rebuild(&(gsplit[i]));
	}

	return DART_OK;
}

void dart_group_dump(const dart_group_t *g)
{
	int i;
	fprintf(stderr, "%2d members: ", g->nmem);
	for (i = 0; i < g->nmem; i++)
	{
		fprintf(stderr, "%d ", g->l2g[i]);
	}
	fprintf(stderr, "\n");
}
