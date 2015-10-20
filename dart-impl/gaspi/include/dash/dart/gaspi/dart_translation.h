/** @file dart_translation.h
 *  @date 25 Aug 2014
 *  @brief Function prototypes for the operations on translation table.
 */

#ifndef DART_ADAPT_TRANSLATION_H_INCLUDED
#define DART_ADAPT_TRANSLATION_H_INCLUDED

#include <stdio.h>
#include <GASPI.h>
#include <dash/dart/if/dart_types.h>
/** @brief Definition of translation table.
 *
 *  This global translation table is created for dart collective memory allocation.
 *  Here, several features for translation table are itemized below:
 *
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
    /* the gaspi segment id set of the memory location of all the units in certain team. */
    gaspi_segment_id_t * gaspi_seg_ids;
    gaspi_segment_id_t own_gaspi_seg_id;
    size_t unit_count;
}info_t;

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
 *  @param[in] item Record to be inserted into the translation table.
 */
int dart_adapt_transtable_add (info_t item);

/** @brief Remove a item from the translation table.
 *
 *  Seg_id can determine a record in the translation table uniquely.
 *
 *  @param[in] seg_id
 */
int dart_adapt_transtable_remove (int16_t seg_id);

int dart_adapt_transtable_get_entry(int16_t seg_id, node_t * entry);

int dart_adapt_transtable_get_local_gaspi_seg_id(int16_t seg_id, gaspi_segment_id_t * own_segid);

int dart_adapt_transtable_get_gaspi_seg_id (int16_t seg_id, dart_unit_t rel_unit, gaspi_segment_id_t * segid);

/** @brief Query the length of the global memory block indicated by the specified seg_id.
 *
 *  @retval ditto
 */
int dart_adapt_transtable_get_size (int16_t seg_id, size_t* size);

/** @brief Destroy the translation table associated with the speicified team.
 */
int dart_adapt_transtable_destroy ();

#endif /*DART_ADAPT_TRANSLATION_H_INCLUDED*/
