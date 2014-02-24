/** @file dart_adapt_globmem.h
 *  @date 20 Nov 2013
 *  @brief Function prototypes for all the related global pointer operations.
 */

#ifndef DART_ADAPT_GLOBMEM_H_INCLUDED
#define DART_ADAPT_GLOBMEM_H_INCLUDED

#include "dart_deb_log.h"
#include "dart_globmem.h"
#ifdef __cplusplus
extern "C" {
#endif	

#define DART_INTERFACE_ON

#define DART_GPTR_COPY(gptr_, gptrt_)                           \
	  ({gptr_.addr_or_offs.offset = gptrt_.addr_or_offs.offset;	\
	   gptr_.flags = gptrt_.flags;					\
	   gptr_.segid = gptrt_.segid;					\
	   gptr_.unitid = gptrt_.unitid;})

/** @brief Fetch virtual address of the memory segment pointed by gptr. 
 *
 *  @param[in] gptr	Global pointer.
 *  @param[out] addr	Virtual address.
 *  @return 		Dart status. 
 */
dart_ret_t dart_adapt_gptr_getaddr (const dart_gptr_t gptr, void **addr);

/** @brief Make the gptr point to the memory segment,
 *         which is specified by addr.
 *
 *  @param[in,out] gptr	Global pointer.
 *  @param[in] addr	Virtual address.
 *  @return		Dart status.
 *  @note Offset would always be used instead of addr for global pointer.
 *        So here, related offset would be calculated first according to the specified 
 *        addr and then written into gptr.
 */
dart_ret_t dart_adapt_gptr_setaddr (dart_gptr_t* gptr, void *addr);

/** @brief Only allocate a segment of memory in the calling unit
 *         with the size specified by nbytes. Local operation.
 *
 *  @param[in] nbytes	 Size of allocated memory.
 *  @param[out] gptr	 Point to the allocated memory.
 *  @return 		 Dart status.
 */
dart_ret_t dart_adapt_memalloc(size_t nbytes, dart_gptr_t *gptr);

/** @brief Only free the allocated memory segment pointed by gptr.
 *         Local operation.
 *  
 *  Be paired with the above local allocation operation.
 *  @param[in] gptr	 Point to the memory segment to be freed.
 *  @return		 Dart status.
 */
dart_ret_t dart_adapt_memfree(dart_gptr_t gptr);

/** @brief Allocate a memory region across the teamid.
 *         Collective on teamid.
 *
 *  @param[in] teamid	Team the allocation based on.
 *  @param[in] nbytes	Size of each memory segment.
 *  @param[out] gptr	Point to the allocated memory region.
 *  @return		Dart status.
 */
dart_ret_t dart_adapt_team_memalloc_aligned(dart_team_t teamid, 
				      size_t nbytes, dart_gptr_t *gptr);

/** @brief Free the allocated memory region spanning teamid pointed by gptr.
 *         Collective on teamid.
 *
 *  Be paired with the above collective allocation operation.
 *  @param[in] teamid	Team the free based on.
 *  @param[in] gptr	Point to the memory region to be freed.
 *  @return		Dart status.
 */
dart_ret_t dart_adapt_team_memfree(dart_team_t teamid, dart_gptr_t gptr);


#define DART_INTERFACE_OFF

#ifdef __cplusplus
}
#endif

#endif /* DART_ADAPT_GLOBMEM_H_INCLUDED */

