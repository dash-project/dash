/** @file dart_synchronization_priv.h
 *  @date 25 Aug 2014
 *  @brief Definition of dart_lock_struct.
 */

#ifndef DART_ADAPT_SYNCHRONIZATION_PRIV_H_INCLUDED
#define DART_ADAPT_SYNCHRONZIATION_PRIV_H_INCLUDED

#include <stdio.h>
#include <GASPI.h>
#include <dash/dart/if/dart_globmem.h>
#include <dash/dart/if/dart_synchronization.h>

/** @brief Dart lock type.
 *  */
struct dart_lock_struct
{
    dart_gptr_t gptr; /**Location of the atomic variable*/
    dart_team_t teamid;
    int32_t is_acquired; /**< Indicate if certain unit has acquired the lock or not. */
};

#endif /* DART_ADAPT_SYNCHRONIZATION_PRIV_H_INCLUDED */
