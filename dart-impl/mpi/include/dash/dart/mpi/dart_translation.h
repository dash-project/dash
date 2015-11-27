/** @file dart_translation.h
 *  @date 25 Aug 2014
 *  @brief Function prototypes for the operations on translation table.
 */

#ifndef DART_ADAPT_TRANSLATION_H_INCLUDED
#define DART_ADAPT_TRANSLATION_H_INCLUDED

#include <stdio.h>
#include <mpi.h>
#include <dash/dart/if/dart_types.h>
#include <dash/dart/base/logging.h>

/* Global object for one-sided communication on memory region allocated with 'local allocation'. */
extern MPI_Win dart_win_local_alloc; 
#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
extern MPI_Win dart_sharedmem_win_local_alloc;
#endif
/** @brief Definition of translation table.
 * 
 *  This global translation table is created for dart collective memory allocation.
 *  Here, several features for translation table are itemized below:
 **
 *  - created a global collective global memory when a DART program is initiated.
 *
 *  - store the one-to-one correspondence relationship between global pointer and shared memory window.
 *
 *  - arranged in an increasing order based upon global pointer (segid).
 * 
 *  @note global pointer (segid) <-> shared memory window (win object), win object should be determined by seg_id uniquely.
 */
typedef struct
{
	int16_t seg_id; /* seg_id determines a global pointer uniquely */
	size_t size;
	MPI_Aint *disp; /* the address set of the memory location of all units in certain team. */
	char **baseptr;
	char *selfbaseptr;
	MPI_Win win;
}info_t;
#if 0
typedef struct
{
	int16_t seg_id;
	size_t size;
	MPI_Aint* disp;
	char*selfbaseptr;
}info_t;
#endif


struct node
{
	info_t trans;
	struct node* next;
};
typedef struct node node_info_t;
typedef node_info_t* node_t;


/* -- The operations on translation table -- */

/** @brief Initialize this global translation table.
 *  
 *  Every translation table is arranged in a linked list fashion, so what "transtable_create" has done 
 *  is just to let the translation table on the specified team (teamid) be NULL.
 *
 */
int dart_adapt_transtable_create ();

/** @brief Add a new item into the specified translation table.
 *  
 *  The translation table is arranged in an increasing order based upon 'seg_id'.
 *
 *  @param[in] item	Record to be inserted into the translation table.
 */
int dart_adapt_transtable_add (info_t item);

/** @brief Remove a item from the translation table.
 *
 *  Seg_id can determine a record in the translation table uniquely.
 *
 *  @param[in] seg_id 
 */ 
int dart_adapt_transtable_remove (int16_t seg_id);

/** @brief Query the shared memory window object associated with the specified seg_id.
 * 
 *  @param[in] seg_id
 *  @param[out] win	A MPI window object. 
 *
 *  @retval non-negative integer Search successfully.
 *  @retval negative integer Failure.
 */
#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
int dart_adapt_transtable_get_win (int16_t seg_id, MPI_Win *win);
#endif
/** @brief Query the address of the memory location of the specified rel_unit in specified team.
 *
 *  The output disp_s information targets for the dart inter-node communication, which means
 *  the one-sided communication proceed within the dynamic window object instead of the 
 *  shared memory window object.
 *
 *  @retval ditto
 */

int dart_adapt_transtable_get_disp (int16_t seg_id, int rel_unit, MPI_Aint *disp_s);

/** @brief Query the length of the global memory block indicated by the specified seg_id.
 *
 *  @retval ditto
 */
#if !defined(DART_MPI_DISABLE_SHARED_WINDOWS)
int dart_adapt_transtable_get_baseptr (int16_t seg_id, int rel_unit, char**baseptr);
#endif

int dart_adapt_transtable_get_selfbaseptr (int16_t seg_id, char**selfbaseptr);

int dart_adapt_transtable_get_size (int16_t seg_id, size_t* size);

/** @brief Destroy the translation table associated with the speicified team.
 */
int dart_adapt_transtable_destroy ();

#endif /*DART_ADAPT_TRANSLATION_H_INCLUDED*/
