
#include "dart_types.h"
#include "dart_groups.h"
#include "dart_groups_impl.h"

dart_ret_t dart_group_init(dart_group_t *group)
{
  group->bla=23;
  group->blabla=23.32;
  return DART_OK;
}

dart_ret_t dart_group_fini(dart_group_t *group)
{
  return DART_OK;
}

dart_ret_t dart_group_copy(const dart_group_t *gin,
                           dart_group_t *gout)
{
  return DART_OK;
}

dart_ret_t dart_group_union(const dart_group_t *g1,
                            const dart_group_t *g2,
                            dart_group_t *gout)
{
  return DART_OK;
}


dart_ret_t dart_group_intersect(const dart_group_t *g1,
                                const dart_group_t *g2,
                                dart_group_t *gout)
{
  return DART_OK;
}

dart_ret_t dart_group_addmember(dart_group_t *g, int32_t unitid)
{
  return DART_OK;
}

dart_ret_t dart_group_delmember(dart_group_t *g, int32_t unitid)
{
  return DART_OK;
}

dart_ret_t dart_group_ismember(const dart_group_t *g, 
			       int32_t unitid, int32_t *ismember)
{
  *ismember=1;
  return DART_OK;
}

dart_ret_t dart_group_size(const dart_group_t *g, size_t *size)
{
  *size=23;
  return DART_OK;
}

dart_ret_t dart_group_getmembers(const dart_group_t *g, int32_t *unitids)
{
  return DART_OK;
}

dart_ret_t dart_group_split(const dart_group_t *g, uint32_t n,
                            dart_group_t *gout)
{
  return DART_OK;
}

dart_ret_t dart_group_sizeof(size_t *size)
{
  *size=sizeof(dart_group_t);
  return DART_OK;
}

