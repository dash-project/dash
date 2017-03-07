#ifndef DART__GLOBMEM_H
#define DART__GLOBMEM_H

/* DART v4.0 */

#ifdef __cplusplus
extern "C" {
#endif

#define DART_INTERFACE_ON

/**
 * \file dart_globmem.h
 *
 * ### DART global memory allocation and referencing ###
 *
 */

/*
  --- DART global pointers ---

  DART global pointers are 128 bits wide and use the following layout:

  0         1         2         3         4         5         6
 0123456789012345678901234567890123456789012345678901234567890123
 |------[32 bit unit id]--------|-[segment id]--|-[flags/resv]--|
 |-----------[either a virt. address or an offset]--------------|
*/


typedef struct
{
  dart_unit_t unitid;  /* 32 bits for the unit ID */
  int16_t     segid;   /* 16 bits for the segment ID */
  uint16_t    flags;   /* 16 bits are reserved for flags */
  union                /* 64 bits for an offset or address */
  {
    uint64_t offset;
    void*    addr;
  } addr_or_offs;
} dart_gptr_t;

#ifdef __cplusplus
#define DART_GPTR_NULL (dart_gptr_t { DART_UNDEFINED_UNIT_ID, 0, 0, { 0 } })
#else
#define DART_GPTR_NULL ((dart_gptr_t)({ .unitid = DART_UNDEFINED_UNIT_ID, \
                                        .segid  =  0,                     \
                                        .flags  =  0,                     \
                                        .addr_or_offs.offset = 0 }))
#endif


/*
 * If the DART_FLAG_LOCALADDR flag is set, the 'addr_or_offs' field of
 * the global pointer holds a valid local virtual address on the
 * calling unit
 */
#define DART_FLAG_LOCALADDR    (0x0001)



#define DART_GPTR_SET_FLAG(gptr_, flag_)	\
  ((gptr_.flag) |= (flag_))

#define DART_GPTR_GET_FLAG(gptr_, flag_)	\
  ((gptr_.flag) &= (flag_))


#define DART_GPTR_ISNULL(gptr_)				\
  (gptr_.unitid<0 && gptr_.segid==0 &&			\
   gptr_.flags==0 && gptr_.addr_or_offs.addr==0)


#define DART_GPTR_EQUAL(gptr1_, gptr2_ )		\
  ((gptr1_.unitid == gptr2_.unitid) &&			\
   (gptr1_.segid == gptr2_.segid) &&			\
   (gptr1_.flags == gptr2_.flags) &&			\
   (gptr1_.addr_or_offs.offset ==			\
    gptr2_.addr_or_offs.offset) )


/*
  Get the local virtual address for the specified global pointer.
  I.e., if the global pointer has affinity with to the calling unit,
  addr is resolved to the local virtual address represented by the
  global pointer.
*/
dart_ret_t dart_gptr_getaddr(const dart_gptr_t gptr, void **addr);

/*
  Form a global pointer for the specified local virtual address
*/
dart_ret_t dart_gptr_setaddr(dart_gptr_t *gptr, void *addr);

/*
  Add 'offs' to the address specified by the global pointer.
*/
dart_ret_t dart_gptr_incaddr(dart_gptr_t *gptr, int32_t offs);

/*
  Set the unit information for the specified global pointer.
*/
dart_ret_t dart_gptr_setunit(dart_gptr_t *gptr, dart_unit_t unit);

#if 0 /* KF */

/*
  Allocates nbytes of memory in the global address space of the
  calling unit and returns a global pointer to it.  This is *not* a
  collective function.
 */
dart_ret_t dart_memalloc(size_t nbytes, dart_gptr_t *gptr);
dart_ret_t dart_memfree(dart_gptr_t gptr);


/*
  Collective function on the specified team to allocate nbytes of
  memory in each unit's global address space with a type_disp as the
  local disposition (size in bytes) of the allocated type.
  The allocated memory is team-aligned (i.e., a global pointer to
  anywhere in the allocation can easily be formed locally. The global
  pointer to the beginning of the allocation is is returned in gptr on
  each participating unit. I.e., Each participating unit has to call
  dart_team_memalloc_aligned with the same specification of teamid and
  nbytes. Each unit will receive the a global pointer to the beginning
  of the allocation (on unit 0) in gptr.

  Accessibility of memory allocated with this function is limited to
  those units that are part of the team allocating the memory. I.e.,
  if unit X was not part of the team that allocated the memory M, then
  X may not be able to acces a memory location in M.

 */
dart_ret_t dart_team_memalloc_aligned(dart_team_t   teamid,
				      size_t        nbytes,
				      dart_gptr_t * gptr);

dart_ret_t dart_team_memfree(dart_team_t teamid, dart_gptr_t gptr);

/*
  Collective functions similar to dart_team_memalloc_aligned() and
  dart_team_memfree() but dart_team_memregister_aligned() and
  dart_team_memderegister() work on previously externally allocated
  memory and don't perform any memory allocation or de-allocation
  themselves.
*/

dart_ret_t dart_team_memregister_aligned(dart_team_t teamid,
					 size_t nbytes,
					 void *addr, dart_gptr_t *gptr);

dart_ret_t dart_team_memderegister(dart_team_t teamid, dart_gptr_t gptr);

#endif


#define DART_INTERFACE_OFF

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DART__GLOBMEM_H */

