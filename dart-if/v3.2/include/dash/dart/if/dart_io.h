/**
 * \file dash/dart/if/dart_io.h
 *
 * A set of routines to query and remodel the locality domain hierarchy
 * and the logical arrangement of teams.
 *
 */
#ifndef DART__IO_H_
#define DART__IO_H_

#include <dash/dart/if/dart_types.h>


/**
 * \defgroup  DartLocality  Locality- and topolgy discovery
 * \ingroup   DartInterface
 */
#ifdef __cplusplus
extern "C" {
#endif

#define DART_INTERFACE_ON
dart_ret_t dart__io__hdf5__prep_mpio(
    hid_t plist_id,
    dart_team_t teamid);

#define DART_INTERFACE_OFF

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DART__IO_H_*/
