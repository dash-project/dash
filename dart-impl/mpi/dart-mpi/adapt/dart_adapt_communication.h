/** @file dart_adapt_communication.h
 *  @date 20 Nov 2013
 *  @brief Function prototypes for the dart communication operations.
 *
 *  This contains both collective operations and one_sided operations.
 *  Besides, it provides us two kinds of one_sided operations
 *  --- blocking and non-blocking respectively.
 */
#ifndef DART_ADAPT_COMMUNICATION_H_INCLUDED
#define DART_ADAPT_COMMUNICATION_H_INCLUDED

#include <mpi.h>
#include "dart_types.h"
#include "dart_deb_log.h"
#include "dart_communication.h"
#ifdef __cplusplus
extern "C" {
#endif

#define DART_INTERFACE_ON

/** @brief Dart handle type for non-blocking one-sided operations.
 */
struct dart_handle_struct
{
	MPI_Request request;
	int32_t unitid;
	MPI_Win mpi_win;
};

/* -- Collective routines -- */

/** @brief Acheive barrier on specified teamid.
 *
 *  @param[in] teamid	 Team the barrier operation based on.
 *  @return		 Dart status.
 */
dart_ret_t dart_adapt_barrier(dart_team_t teamid);

dart_ret_t dart_adapt_bcast(void *buf, size_t nbytes, dart_unit_t root, dart_team_t teamid);

dart_ret_t dart_adapt_scatter(void *sendbuf, void *recvbuf, size_t nbytes, dart_unit_t root, dart_team_t teamid);

dart_ret_t dart_adapt_gather(void *sendbuf, void *recvbuf, size_t nbytes, dart_unit_t root, dart_team_t teamid);
  
dart_ret_t dart_adapt_allgather(void *sendbuf, void *recvbuf, size_t nbytes, dart_team_t teamid);

/* -- One_sided routines -- */

/* Blocking versions of one-sided communication operations. */

/** @brief Read the data with the size of nbytes in bytes 
 *  from the remote memory specified by gptr to the memory 
 *  starting at dest.
 *
 *  This is a blocking reading, the origin buffer starting at dest would be ready
 *  after it returns. And it tries to read successive nbytes data starting at
 *  the memory address pointed by gptr.
 *  @param[out] dest	 Return the resulting data in origin.
 *  @param[in] gptr	 Indicate the target buffer to be accessed.	
 *  @param[in] nbytes	 The size of data in bytes to be read.
 *  @return		 Dart status.	
 */
dart_ret_t dart_adapt_get_blocking(void *dest, dart_gptr_t gptr, size_t nbytes);

dart_ret_t dart_adapt_put_blocking(dart_gptr_t gptr, void *src, size_t nbytes);

/* Non-blocking versions returning a handle. */
dart_ret_t dart_adapt_get(void *dest, dart_gptr_t gptr, size_t nbytes, dart_handle_t *handle);

/** @brief Write the data with size of nbytes in bytes from 
 *  the memory starting at src to the remote memory specified by gptr.
 *  
 *  This is a non-blocking writing, the target buffer starting at the memory address
 *  pointed by gptr will not be guaranteed to be ready after it returns. it returns
 *  a handle that will be passed to wait/test operations to help us make sure 
 *  if the RMA operation has been finished both at the origin and target or not.
 *  @param[in] gptr	Indicate the target buffer to be accessed.
 *  @param[in] src	The data to be sent.
 *  @param[in] nbytes	The size of data in bytes to be written.
 *  @param[out] handle	Work with wait/test to help to finish the pending RMA operations.
 *  @return		Dart status.
 */

dart_ret_t dart_adapt_put(dart_gptr_t gptr, void *src, size_t nbytes, dart_handle_t *handle);

/** @brief Receive the handle passed from a non-blocking operation,
 *  to make sure the operation finishing both at the origin and 
 *  the target.
 */
dart_ret_t dart_adapt_wait(dart_handle_t handle);
dart_ret_t dart_adapt_test(dart_handle_t handle);

dart_ret_t dart_adapt_waitall(dart_handle_t *handle, size_t n);
dart_ret_t dart_adapt_testall(dart_handle_t *handle, size_t n);


#define DART_INTERFACE_OFF


#ifdef __cplusplus
}
#endif

#endif /* DART_ADAPT_COMMUNICATION_H_INCLUDED */
