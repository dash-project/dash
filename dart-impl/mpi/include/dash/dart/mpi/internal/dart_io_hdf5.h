/**
 * \file dash/dart/mpi/internal/dart_io_hdf5.h
 */
#ifndef DART__MPI__INTERNAL__IO_HDF5_H__
#define DART__MPI__INTERNAL__IO_HDF5_H__

#include <dash/dart/if/dart_types.h>

#include <hdf5.h>
#include <hdf5_hl.h>

/** creates an hdf5 property list identifier for parallel IO */
dart_ret_t dart__io__hdf5__prep_mpio(
    hid_t plist_id,
    dart_team_t team);

#endif // DART__MPI__INTERNAL__IO_HDF5_H__

