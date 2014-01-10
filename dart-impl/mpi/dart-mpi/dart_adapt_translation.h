/** @file dart_adapt_translation.h
 *  @date 21 Nov 2013
 *  @brief Function prototypes for the operations on translation table.
 */
#ifndef DART_ADAPT_TRANSLATION_H_INCLUDED
#define DART_ADAPT_TRANSLATION_H_INCLUDED

#include<stdio.h>
#include<mpi.h>
#define MAX_NUMBER 256
#include "dart_types.h"
#include "dart_deb_log.h"

/* Global object for one-sided communication on memory region allocated with 'local allocation'. */
MPI_Win win_local_alloc; 

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

/** @brief Initialize the translation table for specified team.
 *  
 *  Every translation table is arranged in a linked list fashion, so what "transtable_create" has done 
 *  is just to let the translation table on specified team (uniqueid) be NULL.
 *
 *  @param[in] uniqueid	The position in array "convertform", used to determine a team uniquely.
 *  @return		Dart status.
 */
dart_ret_t dart_adapt_transtable_create (int uniqueid);

/** @brief Add a new item into the specified translation table.
 *  
 *  Every translation table is arranged in an increasing order based upon 'offset'.
 *
 *  @param[in] uniqueid	Ditto.
 *  @param[in] item	Record to be inserted into the specified translation table.
 *  @return		Dart status.
 */
dart_ret_t dart_adapt_transtable_add (int uniqueid, info_t item);

/** @brief Remove a item from the specified translation table.
 *
 *  Offset can determine a record in the every translation table uniquely.
 */ 
dart_ret_t dart_adapt_transtable_remove (int uniqueid, int offset);

/** @brief Query a item associated with the specified offset.
 *
 *  "Difference = offset - begin" is what we ultimately want,
 *  which is the displacement relative to the beginning address of 
 *  sub-memory associated with specified dart allocation. 
 *
 *  @param[in] uniqueid	Ditto.
 *  @param[in] offset
 *  @param[out] begin	The same as the difference.
 *  @param[out] win	A MPI window object. 
 */

dart_ret_t dart_adapt_transtable_query (int uniqueid, int offset, int *begin, MPI_Win *win);

#endif /*DART_ADAPT_TRANSLATION_H_INCLUDED*/
