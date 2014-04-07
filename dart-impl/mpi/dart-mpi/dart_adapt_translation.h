/** @file dart_adapt_translation.h
 *  @date 25 Mar 2014
 *  @brief Function prototypes for the operations on translation table.
 */

#ifndef DART_ADAPT_TRANSLATION_H_INCLUDED
#define DART_ADAPT_TRANSLATION_H_INCLUDED

#include <stdio.h>
#include <mpi.h>
#include "dart_types.h"
#include "dart_deb_log.h"

/* Global object for one-sided communication on memory region allocated with 'local allocation'. */
extern MPI_Win win_local_alloc; 

/** @brief Definition of translation table.
 * 
 *  This translation table is created for dart collective memory allocation.
 *  Here, several features for translation table are itemized below:
 *
 *  - created every time a new team is generated.
 *
 *  - store the one-to-one correspondence relationship between global pointer and window.
 *
 *  - arranged in an increasing order based upon global pointer.
 * 
 *  @note global pointer (offset) <-> window (win object), win object should be determined by offset uniquely.
 */
typedef struct
{
	MPI_Win win;
}GMRh;

typedef struct
{
	int offset; /* The displacement relative to the base address of the memory segment associated with certain team. */
	int size;
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
int dart_adapt_transtable_remove (int index, int offset);

/** @brief Query a item associated with the specified offset.
 *
 *  "Difference = offset - begin" is what we ultimately want,
 *  which is the displacement relative to the beginning address of 
 *  sub-memory associated with specified dart allocation. 
 *
 *  @param[in] index	Ditto.
 *  @param[in] offset
 *  @param[out] begin	The same as the difference.
 *  @param[out] win	A MPI window object. 
 */

int dart_adapt_transtable_query (int index, int offset, int *begin, MPI_Win *win);

/** @brief Destroy the translation table associated with the speicified team.
 */
int dart_adapt_transtable_destory (int index);

#endif /*DART_ADAPT_TRANSLATION_H_INCLUDED*/
