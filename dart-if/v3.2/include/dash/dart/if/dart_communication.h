#ifndef DART__COMMUNICATION_H_
#define DART__COMMUNICATION_H_

#include <dash/dart/if/dart_types.h>
#include <dash/dart/if/dart_globmem.h>

/**
 * \file dart_communication.h
 *
 * A set of basic collective communication routines in DART.
 * The semantics of the routines below are the same as with MPI. The only
 * difference is that DART doesn't specify data types and the operates on
 * raw buffers instead. Message size is thus specified in bytes.
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
 * \ingroup DartCommuncation
 */
dart_ret_t dart_bcast(
  void *buf,
  size_t nbytes,
	dart_unit_t root,
  dart_team_t team);

/**
 * DART Equivalent to MPI scatter.
 *
 * \ingroup DartCommuncation
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
 * \ingroup DartCommuncation
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
 * \ingroup DartCommuncation
 */
dart_ret_t dart_allgather(
  void *sendbuf,
  void *recvbuf,
  size_t nbytes,
	dart_team_t team);

/**
 * DART Equivalent to MPI allgatherv.
 *
 * \ingroup DartCommuncation
 */
dart_ret_t dart_allgatherv(
  void        * sendbuf,
  size_t        nsendbytes,
  void        * recvbuf,
  int         * nrecvcounts,
  int         * recvdispls,
  dart_team_t   teamid);

/**
 * DART Equivalent to MPI allreduce.
 *
 * \ingroup DartCommuncation
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
 * \ingroup DartCommuncation
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
 * \ingroup DartCommuncation
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
 * \ingroup DartCommuncation
 */
dart_ret_t dart_put(
  dart_gptr_t   ptr,
  const void  * src,
  size_t        nbytes);

/**
 * DART Equivalent to MPI_Accumulate.
 *
 * \ingroup DartCommuncation
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
 * \ingroup DartCommuncation
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
 * \ingroup DartCommuncation
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
 * \ingroup DartCommuncation
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
 * \ingroup DartCommuncation
 */
dart_ret_t dart_get_blocking(
  void *dest,
  dart_gptr_t ptr,
  size_t nbytes);
/**
 * 'BLOCKING' variant of dart_put.
 * Both local and remote completion is guaranteed.
 *
 * \ingroup DartCommuncation
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
 * \ingroup DartCommuncation
 */
dart_ret_t dart_flush(
  dart_gptr_t gptr);

/**
 * Guarantees local and remote completion of all pending puts and
 * gets on a certain memory allocation / window / segment for all
 * target units. -> MPI_Win_flush_all()
 *
 * \ingroup DartCommuncation
 */
dart_ret_t dart_flush_all(
  dart_gptr_t gptr);

/**
 * Guarantees local completion of all pending puts and
 * gets on a certain memory allocation / window / segment for the
 * target unit specified in gptr. -> MPI_Win_flush_local()
 *
 * \ingroup DartCommuncation
 */
dart_ret_t dart_flush_local(
  dart_gptr_t gptr);

/**
 * Guarantees local completion of all pending puts and
 * gets on a certain memory allocation / window / segment for the
 * all units. -> MPI_Win_flush_local_all()
 *
 * \ingroup DartCommuncation
 */
dart_ret_t dart_flush_local_all(
  dart_gptr_t gptr);

/**
 * Wait for the local and remote completion of an operation.
 *
 * \ingroup DartCommuncation
 */
dart_ret_t dart_wait(
  dart_handle_t handle);
/**
 * Wait for the local and remote completion of operationis.
 *
 * \ingroup DartCommuncation
 */
dart_ret_t dart_waitall(
  dart_handle_t *handle,
  size_t n);

/**
 * Wait for the local completion of an operation.
 *
 * \ingroup DartCommuncation
 */
dart_ret_t dart_wait_local(
    dart_handle_t handle);
/**
 * Wait for the local completion of operations.
 *
 * \ingroup DartCommuncation
 */
dart_ret_t dart_waitall_local(
    dart_handle_t *handle,
    size_t n);

/**
 * Wait for the local completion of an operation.
 *
 * \ingroup DartCommuncation
 */
dart_ret_t dart_test_local(
  dart_handle_t handle,
  int32_t *result);
/**
 * Wait for the local completion of operations.
 *
 * \ingroup DartCommuncation
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
