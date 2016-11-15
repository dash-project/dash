#ifndef DART__COMMUNICATION_H_
#define DART__COMMUNICATION_H_

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_globmem.h>

/**
 * \file dart_communication.h
 *
 * \brief A set of basic communication routines in DART.
 *
 * The semantics of the routines below are the same as with MPI. The only
 * difference is that DART doesn't specify data types but operates on
 * raw buffers instead. Message sizes are thus specified in bytes.
 */

/**
 * \defgroup  DartCommunication  Collective communication routines in DART
 * \ingroup   DartInterface
 *
 */
#ifdef __cplusplus
extern "C" {
#endif

#define DART_INTERFACE_ON

dart_ret_t dart_barrier(
  dart_team_t team);

/**
 * DART Equivalent to MPI broadcast.
 *
 * \param buf    Buffer that is the source (on \c root) or the destination of the broadcast.
 * \param nbytes The number of bytes to broadcast/receive.
 * \param root   The unit that broadcasts data to all other members in \c team
 * \param team   The team to participate in the broadcast.
 *
 * \return \c DART_OK on success, any of \ref dart_ret_t otherwise.
 *
 * \ingroup DartCommunication
 */
dart_ret_t dart_bcast(
  void *buf,
  size_t nbytes,
	dart_unit_t root,
  dart_team_t team);

/**
 * DART Equivalent to MPI scatter.
 *
 * \param sendbuf The buffer containing the data to be sent by unit \c root.
 * \param recvbuf The buffer to hold the received data.
 * \param nbytes  Number of bytes sent to each process and received by each unit.
 * \param root    The unit that scatters data to all units in \c team.
 * \param team    The team to participate in the scatter.
 *
 * \return \c DART_OK on success, any of \ref dart_ret_t otherwise.
 *
 * \ingroup DartCommunication
 */
dart_ret_t dart_scatter(
  void *sendbuf,
  void *recvbuf,
  size_t nbytes,
	dart_unit_t root,
  dart_team_t team);

/**
 * DART Equivalent to MPI gather.
 *
 * \param sendbuf The buffer containing the data to be sent by each unit.
 * \param recvbuf The buffer to hold the received data on unit \c root.
 * \param nbytes  Number of bytes sent by each process and received from each unit at unit \c root.
 * \param root    The unit that gathers all data from units in \c team.
 * \param team    The team to participate in the gather.
 *
 * \return \c DART_OK on success, any of \ref dart_ret_t otherwise.
 *
 * \ingroup DartCommunication
 */
dart_ret_t dart_gather(
  void *sendbuf,
  void *recvbuf,
  size_t nbytes,
	dart_unit_t root,
  dart_team_t team);

/**
 * DART Equivalent to MPI allgather.
 *
 * \param sendbuf The buffer containing the data to be sent by each unit.
 * \param recvbuf The buffer to hold the received data.
 * \param nbytes  Number of bytes sent by each process and received from each unit.
 * \param team    The team to participate in the allgather.
 *
 * \return \c DART_OK on success, any of \ref dart_ret_t otherwise.
 *
 * \ingroup DartCommunication
 */
dart_ret_t dart_allgather(
  void *sendbuf,
  void *recvbuf,
  size_t nbytes,
	dart_team_t team);

/**
 * DART Equivalent to MPI allgatherv.
 *
 * \param sendbuf     The buffer containing the data to be sent by each unit.
 * \param nsendbytes  Number of bytes to be sent by this unit.
 * \param recvbuf     The buffer to hold the received data.
 * \param nrecvcounts Array containing the number of bytes to receive from each unit.
 * \param recvdispls  Array containing the displacements of data received from each unit in \c recvbuf.
 * \param team        The team to participate in the allgatherv.
 *
 * \return \c DART_OK on success, any of \ref dart_ret_t otherwise.
 *
 * \ingroup DartCommunication
 */
dart_ret_t dart_allgatherv(
  void        * sendbuf,
  size_t        nsendbytes,
  void        * recvbuf,
  int         * nrecvbytes,
  int         * recvdispls,
  dart_team_t   teamid);

/**
 * DART Equivalent to MPI allreduce.
 *
 * \param sendbuf The buffer containing the data to be sent by each unit.
 * \param recvbuf The buffer to hold the received data.
 * \param nbytes  Number of bytes sent by each process and received from each unit.
 * \param dtype   The data type to use in the reduction operation \c op.
 * \param op      The reduction operation to perform.
 * \param team The team to participate in the allreduce.
 *
 * \return \c DART_OK on success, any of \ref dart_ret_t otherwise.
 *
 * \ingroup DartCommunication
 */
dart_ret_t dart_allreduce(
  void           * sendbuf,
  void           * recvbuf,
  size_t           nbytes,
  dart_datatype_t  dtype,
  dart_operation_t op,
  dart_team_t      team);

/**
 * DART Equivalent to MPI reduce.
 *
 * \todo Why is this not generic?
 *
 * \ingroup DartCommunication
 */
dart_ret_t dart_reduce_double(
  double *sendbuf,
  double *recvbuf,
  dart_team_t team);

typedef struct dart_handle_struct * dart_handle_t;

/**
 * 'REGULAR' variant of dart_get.
 * When this functions returns, neither local nor remote completion
 * is guaranteed. A later fence/flush operation is needed to guarantee
 * local and remote completion.
 *
 * \param dest The local destination buffer to store the data to.
 * \param ptr  A global pointer determining the source of the get operation.
 * \param nbytes The number of bytes to transfer.
 *
 * \return \c DART_OK on success, any of \ref dart_ret_t otherwise.
 *
 * \ingroup DartCommunication
 */
dart_ret_t dart_get(
  void        * dest,
  dart_gptr_t   ptr,
  size_t        nbytes);
/**
 * 'REGULAR' variant of dart_put.
 * When this functions returns, neither local nor remote completion
 * is guaranteed. A later fence/flush operation is needed to guarantee
 * local and remote completion.
 *
 * \param ptr  A global pointer determining the target of the put operation.
 * \param src  The local source buffer to load the data from.
 * \param nbytes The number of bytes to transfer.
 *
 * \return \c DART_OK on success, any of \ref dart_ret_t otherwise.
 *
 * \ingroup DartCommunication
 */
dart_ret_t dart_put(
  dart_gptr_t   ptr,
  const void  * src,
  size_t        nbytes);

/**
 * DART Equivalent to MPI_Accumulate.
 *
 * \return \c DART_OK on success, any of \ref dart_ret_t otherwise.
 *
 * \ingroup DartCommunication
 */
dart_ret_t dart_accumulate(
  dart_gptr_t      gptr,
  void  *          values,
  size_t           nelem,
  dart_datatype_t  dtype,
  dart_operation_t op,
  dart_team_t      team);

/**
 * DART Equivalent to MPI_Fetch_and_op.
 *
 * \return \c DART_OK on success, any of \ref dart_ret_t otherwise.
 *
 * \ingroup DartCommunication
 */
dart_ret_t dart_fetch_and_op(
  dart_gptr_t      gptr,
  void *           value,
  void *           result,
  dart_datatype_t  dtype,
  dart_operation_t op,
  dart_team_t      team);

/**
 * 'HANDLE' variant of dart_get.
 * Neither local nor remote completion is guaranteed. A later
 * dart_wait*() call or a fence/flush operation is needed to guarantee
 * completion.
 *
 * \return \c DART_OK on success, any of \ref dart_ret_t otherwise.
 *
 * \ingroup DartCommunication
 */
dart_ret_t dart_get_handle(
  void            * dest,
  dart_gptr_t       ptr,
  size_t            nbytes,
  /// [OUT] Pointer to DART handle to instantiate for later use with
  ///       \c dart_wait, \c dart_wait_all etc.
  dart_handle_t   * handle);

/**
 * 'HANDLE' variant of dart_put.
 * Neither local nor remote completion is guaranteed. A later
 * dart_wait*() call or a fence/flush operation is needed to guarantee
 * completion.
 *
 * \return \c DART_OK on success, any of \ref dart_ret_t otherwise.
 *
 * \ingroup DartCommunication
 */
dart_ret_t dart_put_handle(
  dart_gptr_t     ptr,
  const void    * src,
  size_t          nbytes,
  /// [OUT] Pointer to DART handle to instantiate for later use with
  ///       \c dart_wait, \c dart_wait_all etc.
  dart_handle_t * handle);

/**
 * 'BLOCKING' variant of dart_get.
 * Both local and remote completion is guaranteed.
 *
 * \return \c DART_OK on success, any of \ref dart_ret_t otherwise.
 *
 * \ingroup DartCommunication
 */
dart_ret_t dart_get_blocking(
  void *dest,
  dart_gptr_t ptr,
  size_t nbytes);
/**
 * 'BLOCKING' variant of dart_put.
 * Both local and remote completion is guaranteed.
 *
 * \return \c DART_OK on success, any of \ref dart_ret_t otherwise.
 *
 * \ingroup DartCommunication
 */
dart_ret_t dart_put_blocking(
  dart_gptr_t  ptr,
  const void * src,
  size_t       nbytes);

/**
 * Guarantees local and remote completion of all pending puts and
 * gets on a certain memory allocation / window / segment for the
 * target unit specified in gptr. -> MPI_Win_flush()
 *
 * \return \c DART_OK on success, any of \ref dart_ret_t otherwise.
 *
 * \ingroup DartCommunication
 */
dart_ret_t dart_flush(
  dart_gptr_t gptr);

/**
 * Guarantees local and remote completion of all pending puts and
 * gets on a certain memory allocation / window / segment for all
 * target units. -> MPI_Win_flush_all()
 *
 * \return \c DART_OK on success, any of \ref dart_ret_t otherwise.
 *
 * \ingroup DartCommunication
 */
dart_ret_t dart_flush_all(
  dart_gptr_t gptr);

/**
 * Guarantees local completion of all pending puts and
 * gets on a certain memory allocation / window / segment for the
 * target unit specified in gptr. -> MPI_Win_flush_local()
 *
 * \return \c DART_OK on success, any of \ref dart_ret_t otherwise.
 *
 * \ingroup DartCommunication
 */
dart_ret_t dart_flush_local(
  dart_gptr_t gptr);

/**
 * Guarantees local completion of all pending puts and
 * gets on a certain memory allocation / window / segment for the
 * all units. -> MPI_Win_flush_local_all()
 *
 * \return \c DART_OK on success, any of \ref dart_ret_t otherwise.
 *
 * \ingroup DartCommunication
 */
dart_ret_t dart_flush_local_all(
  dart_gptr_t gptr);

/**
 * Wait for the local and remote completion of an operation.
 *
 * \ingroup DartCommunication
 */
dart_ret_t dart_wait(
  dart_handle_t handle);
/**
 * Wait for the local and remote completion of operationis.
 *
 * \return \c DART_OK on success, any of \ref dart_ret_t otherwise.
 *
 * \ingroup DartCommunication
 */
dart_ret_t dart_waitall(
  dart_handle_t *handle,
  size_t n);

/**
 * Wait for the local completion of an operation.
 *
 * \return \c DART_OK on success, any of \ref dart_ret_t otherwise.
 *
 * \ingroup DartCommunication
 */
dart_ret_t dart_wait_local(
    dart_handle_t handle);
/**
 * Wait for the local completion of operations.
 *
 * \return \c DART_OK on success, any of \ref dart_ret_t otherwise.
 *
 * \ingroup DartCommunication
 */
dart_ret_t dart_waitall_local(
    dart_handle_t *handle,
    size_t n);

/**
 * Wait for the local completion of an operation.
 *
 * \return \c DART_OK on success, any of \ref dart_ret_t otherwise.
 *
 * \ingroup DartCommunication
 */
dart_ret_t dart_test_local(
  dart_handle_t handle,
  int32_t *result);
/**
 * Wait for the local completion of operations.
 *
 * \return \c DART_OK on success, any of \ref dart_ret_t otherwise.
 *
 * \ingroup DartCommunication
 */
dart_ret_t dart_testall_local(
  dart_handle_t *handle,
  size_t n,
  int32_t *result);


#define DART_INTERFACE_OFF

#ifdef __cplusplus
}
#endif

#endif /* DART__COMMUNICATION_H_ */
