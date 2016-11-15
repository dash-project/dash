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
 * \defgroup  DartCommunication  Communication routines in DART
 * \ingroup   DartInterface
 *
 */
#ifdef __cplusplus
extern "C" {
#endif

#define DART_INTERFACE_ON

/**
 * \name Collective operations
 * Collective operations involving all units of a given team.
 */

/** \{ */

/**
 * DART Equivalent to MPI_Barrier
 *
 * \param team The team to perform a barrier on.
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \ingroup DartCommunication
 */
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
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
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
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
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
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
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
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
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
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
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
 * \param nelem   Number of elements sent by each process and received from each unit.
 * \param dtype   The data type to use in the reduction operation \c op.
 * \param op      The reduction operation to perform.
 * \param team The team to participate in the allreduce.
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \ingroup DartCommunication
 */
dart_ret_t dart_allreduce(
  void           * sendbuf,
  void           * recvbuf,
  size_t           nelem,
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


/**
 * DART Equivalent to MPI_Accumulate.
 *
 * \param gptr    A global pointer determining the target of the accumulate operation.
 * \param src     The local buffer holding the elements to accumulate.
 * \param nelem   The number of local elements to accumulate per unit.
 * \param dtype   The data type to use in the accumulate operation \c op.
 * \param op      The accumulation operation to perform.
 * \param team    The team to participate in the accumulate.
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
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
 * \param gptr    A global pointer determining the target of the fetch-and-op operation.
 * \param value   Pointer to an element of type \c dtype to be involved in operation \c op on the value referenced by \c gptr.
 * \param nelem   Pointer to an element of type \c dtype to hold the value of the element referenced by \c gptr before the operation \c op.
 * \param dtype   The data type to use in the operation \c op.
 * \param op      The operation to perform.
 * \param team    The team to participate in the operation.
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
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


/** \} */


/**
 * \name Non-blocking single-sided communication routines
 * DART single-sided communication routines that return without guaranteeing completion.
 * Completion will be guaranteed after a flush operation.
 */
/** \{ */

/**
 * 'REGULAR' variant of dart_get.
 * Copy data referenced by a global pointer into local memory.
 * When this functions returns, neither local nor remote completion
 * is guaranteed. A later flush operation is needed to guarantee
 * local and remote completion.
 *
 * \param dest The local destination buffer to store the data to.
 * \param ptr  A global pointer determining the source of the get operation.
 * \param nbytes The number of bytes to transfer.
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \ingroup DartCommunication
 */
dart_ret_t dart_get(
  void        * dest,
  dart_gptr_t   gptr,
  size_t        nbytes);

/**
 * 'REGULAR' variant of dart_put.
 * Copy data from local memory into memory referenced by a global pointer.
 * When this functions returns, neither local nor remote completion
 * is guaranteed. A later flush operation is needed to guarantee
 * local and remote completion.
 *
 * \param gptr  A global pointer determining the target of the put operation.
 * \param src  The local source buffer to load the data from.
 * \param nbytes The number of bytes to transfer.
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \ingroup DartCommunication
 */
dart_ret_t dart_put(
  dart_gptr_t   gptr,
  const void  * src,
  size_t        nbytes);


/**
 * Guarantee completion of all outstanding operations involving a segment on a certain unit
 *
 * Guarantees local and remote completion of all pending puts and
 * gets on a certain memory allocation / window / segment for the
 * target unit specified in gptr.
 * Similar to \c MPI_Win_flush().
 *
 * \param gptr Global pointer identifying the segment and unit to complete outstanding operations for.
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \ingroup DartCommunication
 */
dart_ret_t dart_flush(
  dart_gptr_t gptr);

/**
 * Guarantee completion of all outstanding operations involving a segment on all units
 *
 * Guarantees local and remote completion of all pending puts and
 * gets on a certain memory allocation / window / segment for all
 * target units.
 * Similar to \c MPI_Win_flush_all().
 *
 * \param gptr Global pointer identifying the segment to complete outstanding operations for.
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \ingroup DartCommunication
 */
dart_ret_t dart_flush_all(
  dart_gptr_t gptr);

/**
 * Guarantee local completion of all outstanding operations involving a segment on a certain unit
 *
 * Guarantees local completion of all pending puts and
 * gets on a certain memory allocation / window / segment for the
 * target unit specified in gptr. -> MPI_Win_flush_local()
 *
 * \param gptr Global pointer identifying the segment and unit to complete outstanding operations for.
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \ingroup DartCommunication
 */
dart_ret_t dart_flush_local(
  dart_gptr_t gptr);

/**
 * Guarantee completion of all outstanding operations involving a segment on all units
 *
 * Guarantees local completion of all pending puts and
 * gets on a certain memory allocation / window / segment for the
 * all units. -> MPI_Win_flush_local_all()
 *
 * \param gptr Global pointer identifying the segment to complete outstanding operations for.
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \ingroup DartCommunication
 */
dart_ret_t dart_flush_local_all(
  dart_gptr_t gptr);


/** \} */

/**
 * \name Non-blocking single-sided communication operations using handles
 * The handle can be used to wait for a specific operation to complete using \c wait functions.
 */

/** \{ */


typedef struct dart_handle_struct * dart_handle_t;

/**
 * 'HANDLE' variant of dart_get.
 * Neither local nor remote completion is guaranteed. A later
 * dart_wait*() call or a fence/flush operation is needed to guarantee
 * completion.
 *
 * \param dest Local target memory to store the data.
 * \param gptr Global pointer being the source of the data transfer.
 * \param nbytes The number of bytes to transfer.
 * \param[out] handle Pointer to DART handle to instantiate for later use with \c dart_wait, \c dart_wait_all etc.
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \ingroup DartCommunication
 */
dart_ret_t dart_get_handle(
  void            * dest,
  dart_gptr_t       gptr,
  size_t            nbytes,
  dart_handle_t   * handle);

/**
 * 'HANDLE' variant of dart_put.
 * Neither local nor remote completion is guaranteed. A later
 * dart_wait*() call or a fence/flush operation is needed to guarantee
 * completion.
 *
 * \param gptr Global pointer being the target of the data transfer.
 * \param src  Local source memory to transfer data from.
 * \param nbytes The number of bytes to transfer.
 * \param[out] Pointer to DART handle to instantiate for later use with \c dart_wait, \c dart_wait_all etc.
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \ingroup DartCommunication
 */
dart_ret_t dart_put_handle(
  dart_gptr_t     gptr,
  const void    * src,
  size_t          nbytes,
  dart_handle_t * handle);

/**
 * Wait for the local and remote completion of an operation.
 *
 * \param handle The handle of the operation to wait for.
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \ingroup DartCommunication
 */

dart_ret_t dart_wait(
  dart_handle_t handle);
/**
 * Wait for the local and remote completion of operations.
 *
 * \param handles Array of handles of operations to wait for.
 * \param n Number of \c handles to wait for.
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \ingroup DartCommunication
 */
dart_ret_t dart_waitall(
  dart_handle_t *handles,
  size_t n);

/**
 * Wait for the local completion of an operation.
 *
 * \param handle Handle of an operations to wait for.
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \ingroup DartCommunication
 */
dart_ret_t dart_wait_local(
    dart_handle_t handle);

/**
 * Wait for the local completion of operations.
 *
 * \param handles Array of handles of operations to wait for.
 * \param n Number of \c handles to wait for.
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \ingroup DartCommunication
 */
dart_ret_t dart_waitall_local(
    dart_handle_t *handles,
    size_t n);

/**
 * Test for the local completion of an operation.
 *
 * \param handle The handle of an operation to test for completion.
 * \param[out] result \c True if the operation has completed.
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \ingroup DartCommunication
 */
dart_ret_t dart_test_local(
  dart_handle_t handle,
  int32_t *result);

/**
 * Test for the local completion of operations.
 *
 * \param handles Array of handles of operations to test for completion.
 * \param n Number of \c handles to test for completion.
 * \param[out] result \c True if all operations have completed.
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \ingroup DartCommunication
 */
dart_ret_t dart_testall_local(
  dart_handle_t *handles,
  size_t n,
  int32_t *result);

/** \} */

/**
 * \name Blocking single-sided communication operations
 * These operations will block until completion of put and get is guaranteed.
 */

/** \{ */

/**
 * 'BLOCKING' variant of dart_get.
 * Both local and remote completion is guaranteed.
 *
 * \param dest Local target memory to store the data.
 * \param gptr Global pointer being the source of the data transfer.
 * \param nbytes The number of bytes to transfer.
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \ingroup DartCommunication
 */
dart_ret_t dart_get_blocking(
  void *dest,
  dart_gptr_t gptr,
  size_t nbytes);

/**
 * 'BLOCKING' variant of dart_put.
 * Both local and remote completion is guaranteed.
 *
 * \param gptr Global pointer being the target of the data transfer.
 * \param src  Local source memory to transfer data from.
 * \param nbytes The number of bytes to transfer.
 *
 * \return \c DART_OK on success, any other of \ref dart_ret_t otherwise.
 *
 * \ingroup DartCommunication
 */
dart_ret_t dart_put_blocking(
  dart_gptr_t  gptr,
  const void * src,
  size_t       nbytes);

/** \} */

#define DART_INTERFACE_OFF

#ifdef __cplusplus
}
#endif

#endif /* DART__COMMUNICATION_H_ */
