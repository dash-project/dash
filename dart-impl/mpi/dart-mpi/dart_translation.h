/** @file dart_translation.h
 *  @date 25 Aug 2014
 *  @brief Function prototypes for the operations on translation table.
 */

#ifndef DART_ADAPT_TRANSLATION_H_INCLUDED
#define DART_ADAPT_TRANSLATION_H_INCLUDED

#include <stdio.h>
#include <mpi.h>
#include "dart_types.h"
#include "dart_deb_log.h"

/* Global object for one-sided communication on memory region allocated with 'local allocation'. */
extern MPI_Win dart_win_local_alloc; 
extern MPI_Win dart_sharedmem_win_local_alloc;
/** @brief Definition of translation table.
 * 
 *  This translation table is created for dart collective memory allocation.
 *  Here, several features for translation table are itemized below:
 *
 *  - created every time a collective global memory is generated.
 *
 *  - store the one-to-one correspondence relationship between global pointer and shared memory window.
 *
 *  - arranged in an increasing order based upon global pointer.
 * 
 *  @note global pointer (offset) <-> shared memory window (win object), win object should be determined by offset uniquely.
 */
typedef struct
{
	MPI_Win win;
}GMRh;

typedef struct
{
	uint64_t offset; /* The displacement relative to the base address of the allocated collective memory memory segment. */
	size_t size;
	MPI_Aint* disp; /* the address set of the memory location of all the units in certain team. */
	GMRh handle;
}info_t;

struct node
{
	info_t trans;
	struct node* next;
};
typedef struct node node_info_t;
typedef node_info_t* node_t;


/* -- The operations on translation table -- */

/** @brief Initialize the translation table for the specified team.
 *  
 *  Every translation table is arranged in a linked list fashion, so what "transtable_create" has done 
 *  is just to let the translation table on the specified team (teamid) be NULL.
 *
 *  @param[in] index	Used to determine a team uniquely for certain unit.
 */
int dart_adapt_transtable_create (int index);

/** @brief Add a new item into the specified translation table.
 *  
 *  Every translation table is arranged in an increasing order based upon 'offset'.
 *
 *  @param[in] index	Ditto.
 *  @param[in] item	Record to be inserted into the specified translation table.
 */
int dart_adapt_transtable_add (int index, info_t item);

/** @brief Remove a item from the specified translation table.
 *
 *  Offset can determine a record in the every translation table uniquely.
 */ 
int dart_adapt_transtable_remove (int index, uint64_t offset);

/** @brief Query the shared memory window object associated with the specified offset.
 *
 *  "Difference = offset - begin" is what we ultimately want,
 *  which is the displacement relative to the beginning address of 
 *  sub-memory associated with specified dart collective global allocation. 
 *
 *  @param[in] index	Ditto.
 *  @param[in] offset
 *  @param[out] win	A MPI window object. 
 *
 *  @retval non-negative integer Search successfully, it indicates the beginning
 *  address of sub-memory associated with specified dart collective global allocation.
 *  @retval negative integer Failure.
 */

int dart_adapt_transtable_get_win (int index, uint64_t offset, uint64_t *base, MPI_Win *win);

//int dart_adapt_transtable_get_addr (int index, int offset, int *begin, void **addr);

/** @brief Query the address set of the memory location of all the units in specified team.
 *
 *  The address set information targets for the dart inter-node communication, which means
 *  the one-sided communication proceed within the dynamic window object instead of the 
 *  shared memory window object.
 *
 *  @retval ditto
 */

int dart_adapt_transtable_get_disp (int index, uint64_t offset, dart_unit_t rel_unit, uint64_t *base, MPI_Aint* disp_s);

/** @brief Destroy the translation table associated with the speicified team.
 */
int dart_adapt_transtable_destroy (int index);

#endif /*DART_ADAPT_TRANSLATION_H_INCLUDED*/
