/**
 * \file dash/dart/mpi/internal/dart_io_hdf5.c
 */

#ifdef DART_ENABLE_HDF5

#define _GNU_SOURCE
#include <dash/dart/mpi/internal/dart_io_hdf5.h>

#include <dash/dart/base/logging.h>
#include <dash/dart/mpi/dart_team_private.h>


dart_ret_t dart__io__hdf5__prep_mpio(
    hid_t plist_id,
    dart_team_t teamid)
{
  MPI_Comm comm;
  uint16_t index;
  int      result;
  DART_LOG_TRACE("dart__io__hdf5__prep_mpio() team:%d", teamid);

  result = dart_adapt_teamlist_convert(teamid, &index);
  if (result == -1) {
    DART_LOG_ERROR("ddart__io__hdf5__prep_mpio ! team:%d "
                   "dart_adapt_teamlist_convert failed", teamid);
    return DART_ERR_INVAL;
  }

  comm = dart_team_data[index].comm;
  herr_t status = H5Pset_fapl_mpio(plist_id, comm, MPI_INFO_NULL);
  if(status < 0){
    return DART_ERR_OTHER;
  } 
  return DART_OK;
}

#endif //DART_ENABLE_HDF5
