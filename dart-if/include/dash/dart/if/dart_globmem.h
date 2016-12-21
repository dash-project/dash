#ifndef DART_GLOBMEM_H_INCLUDED
#define DART_GLOBMEM_H_INCLUDED

/**
 * \file dart_globmem.h
 *
 * \defgroup  DartGlobMem    Global memory and PGAS address semantics
 * \ingroup   DartInterface
 *
 * Routines for allocation and reclamation of global memory regions and pointer semantics in partitioned global address space.
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

/** \cond DART_HIDDEN_SYMBOLS */
#define DART_INTERFACE_ON
/** \endcond */

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

/**
 * DART Global pointer type.
 *
 * \ingroup DartGlobMem
 */
typedef struct
{
  /** The unit holding the memory element */
  dart_unit_t unitid;
  /** The segment ID of the allocation */
  int16_t     segid;
  /** Reserved */
  uint16_t    flags;
  /** Absolute address or relative offset */
  union
  {
    uint64_t offset;
    void*    addr;
  } addr_or_offs;
} dart_gptr_t;

/**
 * A NULL global pointer
 * \ingroup DartGlobMem
 */
#ifdef __cplusplus
#define DART_GPTR_NULL (dart_gptr_t { -1, 0, 0, { 0 } })
#else
#define DART_GPTR_NULL \
((dart_gptr_t)({ .unitid = -1, \
                 .segid  =  0, \
                 .flags  =  0, \
                 .addr_or_offs.offset = 0 }))
#endif

/**
 * Test for NULL global pointer
 *
 * \ingroup DartGlobMem
 */
#define DART_GPTR_ISNULL(gptr_)			\
  (gptr_.unitid<0 && gptr_.segid==0 &&		\
   gptr_.flags==0 && gptr_.addr_or_offs.addr==0)

/**
 * Compare two global pointers
 *
 * \ingroup DartGlobMem
 */
#define DART_GPTR_EQUAL(gptr1_, gptr2_ )		\
  ((gptr1_.unitid == gptr2_.unitid) &&			\
   (gptr1_.segid == gptr2_.segid) &&			\
   (gptr1_.flags == gptr2_.flags) &&			\
   (gptr1_.addr_or_offs.offset ==			\
    gptr2_.addr_or_offs.offset) )


/**
 * Get the local memory address for the specified global pointer
 * gptr. I.e., if the global pointer has affinity to the local unit,
 * return the local memory address.
 *
 * \param gptr Global pointer
 * \param[out] addr Pointer to a pointer that will hold the local address if the \c gptr points to a local memory element.
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \threadsafe
 * \ingroup DartGlobMem
 */
dart_ret_t dart_gptr_getaddr(const dart_gptr_t gptr, void **addr);

/**
 * Set the local memory address for the specified global pointer such
 * the the specified address.
 *
 * \param gptr Global pointer
 * \param addr Pointer holding the local address to set in \c gptr.
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \threadsafe
 * \ingroup DartGlobMem
 */
dart_ret_t dart_gptr_setaddr(dart_gptr_t *gptr, void *addr);

/**
 * Add 'offs' to the address specified by the global pointer
 *
 * \param gptr Global pointer
 * \param offs Offset by which to increment \c gptr
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \threadsafe
 * \ingroup DartGlobMem
 */
dart_ret_t dart_gptr_incaddr(dart_gptr_t *gptr, int32_t offs);

/**
 * Set the unit information for the specified global pointer.
 *
 * \param gptr Global Pointer
 * \param unit The global unit to set in \c gptr
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \threadsafe
 * \ingroup DartGlobMem
 */
dart_ret_t dart_gptr_setunit(dart_gptr_t *gptr, dart_global_unit_t unit);

/**
 * Allocates memory for \c nelem elements of type \c dtype in the global
 * address space of the calling unit and returns a global pointer to it.
 * This is *not* a collective function.
 *
 * \param nelem The number of elements of type \c dtype to allocate.
 * \param dtype The type to use.
 * \param[out] gptr Global Pointer to hold the allocation
 *
 * \todo Does dart_memalloc really allocate in _global_ memory?
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \threadsafe_none
 * \ingroup DartGlobMem
 */
dart_ret_t dart_memalloc(
  size_t            nelem,
  dart_datatype_t   dtype,
  dart_gptr_t     * gptr);

/**
 * Frees memory in the global address space allocated by a previous call
 * of \ref dart_memalloc.
 * This is *not* a collective function.
 *
 * \param gptr Global pointer to the memory allocation to free
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \threadsafe_none
 * \ingroup DartGlobMem
 */
dart_ret_t dart_memfree(dart_gptr_t gptr);

/**
 * Collective function on the specified team to allocate \c nelem elements
 * of type \c dtype of memory in each unit's global address space with a
 * local displacement of the specified type.
 * The allocated memory is team-aligned, i.e., a global pointer to
 * anywhere in the allocation can easily be formed locally. The global
 * pointer to the beginning of the allocation is returned in \c gptr on
 * each participating unit. Each participating unit has to call
 * \c dart_team_memalloc_aligned with the same specification of \c teamid, \c dtype and
 * \c nelem. Each unit will receive the a global pointer to the beginning
 * of the allocation (on unit 0) in \c gptr.
 * Accessibility of memory allocated with this function is limited to
 * those units that are part of the team allocating the memory. I.e.,
 * if unit X was not part of the team that allocated the memory M, then
 * X may not be able to access a memory location in M.
 *
 * \param teamid      The team participating in the collective memory
 *                    allocation.
 * \param nelem       The number of elements to allocate per unit.
 * \param dtype       The data type of elements in \c addr.
 *
 * \param[out]  gptr  Global pointer to store information on the allocation.
 *
 * \return            \c DART_OK on success,
 *                    any other of \ref dart_ret_t otherwise.
 *
 * \threadsafe_none
 * \ingroup DartGlobMem
 */
dart_ret_t dart_team_memalloc_aligned(
  dart_team_t       teamid,
	size_t            nelem,
  dart_datatype_t   dtype,
  dart_gptr_t     * gptr);

/**
 * Collective function to free global memory previously allocated
 * using \ref dart_team_memalloc_aligned.
 * After this operation, the global pointer should not be used in any
 * communication unless re-used in another allocation.
 * After this operation, the global pointer can be reset using \ref DART_GPTR_NULL.
 *
 * \param teamid The team to participate in the collective deallocation.
 * \param gptr Global pointer pointing to the memory to deallocate.
 *
 * \see DART_GPTR_NULL
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \threadsafe_none
 * \ingroup DartGlobMem
 */
dart_ret_t dart_team_memfree(
  dart_team_t teamid,
  dart_gptr_t gptr);

/**
 * Collective function similar to \ref dart_team_memalloc_aligned but on
 * previously externally allocated memory.
 * Does not perform any memory allocation.
 *
 * \param teamid The team to participate in the collective operation.
 * \param nelem  The number of elements already allocated in \c addr.
 * \param dtype  The data type of elements in \c addr.
 * \param addr   Pointer to pre-allocated memory to be registered.
 * \param gptr   Pointer to a global pointer object to set up.
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \see dart_team_memalloc_aligned
 *
 * \threadsafe_none
 * \ingroup DartGlobMem
 */
dart_ret_t dart_team_memregister_aligned(
  dart_team_t       teamid,
	size_t            nelem,
  dart_datatype_t   dtype,
	void            * addr,
	dart_gptr_t     * gptr);

/**
 * Attaches external memory previously allocated by the user.
 * Does not perform any memory allocation.
 *
 * \param teamid The team to participate in the collective operation.
 * \param nelem  The number of elements already allocated in \c addr.
 * \param dtype  The data type of elements in \c addr.
 * \param addr   Pointer to pre-allocated memory to be registered.
 * \param gptr   Pointer to a global pointer object to set up.
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \threadsafe_none
 * \ingroup DartGlobMem
 */
dart_ret_t dart_team_memregister(
  dart_team_t       teamid,
	size_t            nelem,
  dart_datatype_t   dtype,
	void            * addr,
	dart_gptr_t     * gptr);

/**
 * Collective function similar to dart_team_memfree() but on previously
 * externally allocated memory.
 * Does not perform any memory de-allocation.
 *
 * \param teamid The team to participate in the collective operation.
 * \param gptr   Pointer to a global pointer object to set up.
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \see dart_team_memregister
 * \see dart_team_memregister_aligned
 *
 * \threadsafe_none
 * \ingroup DartGlobMem
 */
dart_ret_t dart_team_memderegister(dart_team_t teamid, dart_gptr_t gptr);


/** \cond DART_HIDDEN_SYMBOLS */
#define DART_INTERFACE_OFF
/** \endcond */

#ifdef __cplusplus
}
#endif

#endif /* DART_GLOBMEM_H_INCLUDED */

