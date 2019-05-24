/** @file dart_synchronization_priv.h
 *  @date 25 Aug 2014
 *  @brief Definition of dart_lock_struct.
 */

#ifndef DART_ADAPT_SYNCHRONIZATION_PRIV_H_INCLUDED
#define DART_ADAPT_SYNCHRONZIATION_PRIV_H_INCLUDED

#include <stdio.h>
#include <GASPI.h>
#include <dash/dart/if/dart_synchronization.h>

#define DART_GPTR_COPY(gptr_, gptrt_)                           \
    ({gptr_.addr_or_offs.offset = gptrt_.addr_or_offs.offset;   \
    gptr_.flags = gptrt_.flags;                 \
    gptr_.segid = gptrt_.segid;                 \
    gptr_.unitid = gptrt_.unitid;})

/** @brief Dart lock type.
 *  */
struct dart_lock_struct
{
    dart_gptr_t gptr; /**Location of the atomic variable*/
    dart_team_t teamid;
    int32_t is_acquired; /**< Indicate if certain unit has acquired the lock or not. */
};

#endif /* DART_ADAPT_SYNCHRONIZATION_PRIV_H_INCLUDED */
