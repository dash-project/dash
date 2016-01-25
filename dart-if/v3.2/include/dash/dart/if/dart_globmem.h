#ifndef DART_GLOBMEM_H_INCLUDED
#define DART_GLOBMEM_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#define DART_INTERFACE_ON

/*
  --- DART global pointers ---

  There are multiple options for representing the global
  pointer that come to mind:

  1) struct with pre-defined members (say, unit id
    and local address)
  2) an opaque object that leaves the details to a specific
    implementation and is manipulated only through pointers
  3) a fixed size integer data type (say 64 bit or 128 bit),
    manipulated through c macros that packs all the
    relevant information

 There are pros and cons to each option...

 Another question is that of offsets vs. addresses: Either a local
 virtual address is directly included and one in which the pointer
 holds something like a segment ID and an offset within that segment.

 If we want to support virtual addresses then 64 bits is not enough to
 represent the pointer. If we only support segment offsets, 64 bit
 could be sufficient

 Yet another question is what kind of operations are supported on
 global pointers. For example UPC global pointers keep "phase"
 information that allows pointer arithmetic (the phase is needed for
 knowing when you have to move to the next node).

 PROPOSAL: Don't include phase information with pointers on the DART
 level, but don't preclude support the same concept on the DASH level.

  */

  /*
 PROPOSAL: use 128 bit global pointers with the following layout:


 0         1         2         3         4         5         6
 0123456789012345678901234567890123456789012345678901234567890123
 |------<32 bit unit id>--------|-<segment id>--|--flags/resv---|
 |-----------<either a virtual address or an offset>------------|

  */


typedef struct
{
  dart_unit_t unitid;
  int16_t     segid;
  uint16_t    flags;
  union
  {
    uint64_t offset;
    void*    addr;
  } addr_or_offs;
} dart_gptr_t;

#define DART_GPTR_NULL ((dart_gptr_t){-1, 0, 0, 0})

#define DART_GPTR_ISNULL(gptr_)			\
  (gptr_.unitid<0 && gptr_.segid==0 &&		\
   gptr_.flags==0 && gptr_.addr_or_offs.addr==0)

#define DART_GPTR_EQUAL(gptr1_, gptr2_ )		\
  ((gptr1_.unitid == gptr2_.unitid) &&			\
   (gptr1_.segid == gptr2_.segid) &&			\
   (gptr1_.flags == gptr2_.flags) &&			\
   (gptr1_.addr_or_offs.offset ==			\
    gptr2_.addr_or_offs.offset) )


/* get the local memory address for the specified global pointer
   gptr. I.e., if the global pointer has affinity to the local unit,
   return the local memory address.
*/
dart_ret_t dart_gptr_getaddr(const dart_gptr_t gptr, void **addr);


/* set the local memory address for the specified global pointer such
   the the specified address
*/
dart_ret_t dart_gptr_setaddr(dart_gptr_t *gptr, void *addr);

/* add 'offs' to the address specified by the global pointer */
dart_ret_t dart_gptr_incaddr(dart_gptr_t *gptr, int32_t offs);

/* set the unit information for the specified global pointer */
dart_ret_t dart_gptr_setunit(dart_gptr_t *gptr, dart_unit_t);


/*
 Allocates nbytes of memory in the global address space of the calling
 unit and returns a global pointer to it.  This is *not* a collective
 function.
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
dart_ret_t dart_team_memalloc_aligned(
  dart_team_t   teamid,
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


#define DART_INTERFACE_OFF

#ifdef __cplusplus
}
#endif

#endif /* DART_GLOBMEM_H_INCLUDED */

