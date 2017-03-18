/**
 * \file dash/dart/if/dart_io.h
 *
 * A set of utility routines used to provide parallel io support
 * 
 */
#ifndef DART__IO_H_
#define DART__IO_H_

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_util.h>


/**
 * \defgroup  DartIO  Interface for parallel IO
 * \ingroup   DartInterface
 */
#ifdef __cplusplus
extern "C" {
#endif

#define DART_INTERFACE_ON
/**
 * setup hdf5 for parallel io using mpi-io
 */
dart_ret_t dart__io__hdf5__prep_mpio(
    hid_t plist_id,
    dart_team_t teamid) DART_NOTHROW;

#define DART_INTERFACE_OFF

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DART__IO_H_*/
