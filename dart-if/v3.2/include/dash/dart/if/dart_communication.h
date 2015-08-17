#ifndef DART__COMMUNICATION_H_
#define DART__COMMUNICATION_H_

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

typedef struct dart_handle_struct *dart_handle_t;

/**
 * 'REGULAR' variant of dart_get.
 * When this functions returns, neither local nor remote completion
 * is guaranteed. A later fence operation is needed to guarantee
 * local and remote completion.
 *
 * \ingroup DartCommuncation
 */
dart_ret_t dart_get(
  void *dest,
  dart_gptr_t ptr, 
  size_t nbytes);
/**
 * 'REGULAR' variant of dart_put.
 * When this functions returns, neither local nor remote completion
 * is guaranteed. A later fence operation is needed to guarantee
 * local and remote completion.
 *
 * \ingroup DartCommuncation
 */
dart_ret_t dart_put(
  dart_gptr_t ptr,
  void *src, 
  size_t nbytes);

/**
 * 'HANDLE' variant of dart_get.
 * Neither local nor remote completion is guaranteed. A later
 * dart_wait*() call or a fence operation is needed to guarantee
 * completion.
 *
 * \ingroup DartCommuncation
 */
dart_ret_t dart_get_handle(
  void *dest,
  dart_gptr_t ptr, 
  size_t nbytes,
  dart_handle_t *handle);
/**
 * 'HANDLE' variant of dart_put.
 * Neither local nor remote completion is guaranteed. A later
 * dart_wait*() call or a fence operation is needed to guarantee
 * completion.
 *
 * \ingroup DartCommuncation
 */
dart_ret_t dart_put_handle(
  dart_gptr_t ptr,
  void *src, 
  size_t nbytes,
  dart_handle_t *handle);

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
  dart_gptr_t ptr,
  void *src, 
  size_t nbytes);

/**
 * Guarantees local and remote completion of all pending puts and
 * gets on a certain memory allocation / window / segment for the
 * target unit specified in gptr. -> MPI_Win_flush() 
 *
 * \ingroup DartCommuncation
 */
dart_ret_t dart_fence(
  dart_gptr_t gptr);

/**
 * Guarantees local and remote completion of all pending puts and
 * gets on a certain memory allocation / window / segment for all
 * target units. -> MPI_Win_flush_all() 
 *
 * \ingroup DartCommuncation
 */
dart_ret_t dart_fence_all(
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
