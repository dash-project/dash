#ifndef DART_GROUPS_H_INCLUDED
#define DART_GROUPS_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif
  
#define DART_INTERFACE_ON

/*
  DART groups are objects with local meaning only. They are
  essentially objects representing sets of units, out of which later
  teams can be formed. The operations to manipulate groups are local
  (and cheap).  The operations to create teams are collective and can
  be expensive.
*/

/* DART groups are represented by an opage struct dart_group_struct */
typedef struct dart_group_struct dart_group_t;


/*
  dart_group_init must be called before any other function on the
  group object, dart_group_fini reclaims resources that might be
  associated with the group (if any)
*/
dart_ret_t dart_group_init(dart_group_t *group);
dart_ret_t dart_group_fini(dart_group_t *group);


/* make a copy of the group */
dart_ret_t dart_group_copy(const dart_group_t *gin, 
			   dart_group_t *gout);


/* set union */
dart_ret_t dart_group_union(const dart_group_t *g1,
                            const dart_group_t *g2,
                            dart_group_t *gout);


/* set intersection */
dart_ret_t dart_group_intersect(const dart_group_t *g1,
                                const dart_group_t *g2,
                                dart_group_t *gout);


  /* add a member */
dart_ret_t dart_group_addmember(dart_group_t *g, dart_unit_t unitid);


/* delete a member */
dart_ret_t dart_group_delmember(dart_group_t *g, dart_unit_t unitid);


/* test if unitid is a member */
dart_ret_t dart_group_ismember(const dart_group_t *g, 
			       dart_unit_t unitid, int32_t *ismember);


/* determine the size of the group */
dart_ret_t dart_group_size(const dart_group_t *g,
			   size_t *size);


/* get all the members of the group, unitids must be large enough
   to hold dart_group_size() members */
dart_ret_t dart_group_getmembers(const dart_group_t *g, 
				 dart_unit_t *unitids);


/* split the group into n groups of approx. same size,
   gout must be an array of dart_group_t obejcts of size at least n
*/
dart_ret_t dart_group_split(const dart_group_t *g, size_t n, 
			    dart_group_t *gout);


/* get the size of the opaque object */
dart_ret_t dart_group_sizeof(size_t *size);

#define DART_INTERFACE_OFF

#ifdef __cplusplus
}
#endif

#endif /* DART_GROUPS_H_INCLUDED */
