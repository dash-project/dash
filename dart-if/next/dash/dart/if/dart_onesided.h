#ifndef DART__ONESIDED_H
#define DART__ONESIDED_H

/* DART v4.0 */

#ifdef __cplusplus
extern "C" {
#endif

#define DART_INTERFACE_ON

/**
 * \file dart_onesided.h
 *
 * ### DART one-sided communication operations. ###
 *
 * With respect to their completion semantics, DART supports the
 * following three classes of operations:
 *
 * 1) *regular* (e.g., dart_put, dart_get): Neither local nor remote
 * completion is guaranteed when these operations return. A call to a
 * data synchronization operation is required to guarantee completion.
 *
 * 2) *blocking* (e.g., dart_put_blocking): Local completion is
 * guaranteed, remote completion is not guaranteed when these
 * operations return. However, for get operations local completion
 * naturally also implies remote completion.
 *
 * 3) *handle* (e.g., dart_put_handle): Neither local nor remote
 * completion is guaranteed when these operations return. These
 * operations set a handle that can later be used to test/wait for
 * completion. When a test or wait call indicates that the operations
 * have been completed, the same guarantees as in the blocking case
 * hold.
 *
 * TODO: describe strided variants
 */


/**
 @brief   Non-blocking one-sided put

 @param     gpdest   Global destination of the operation (global dart pointer)
 @param      lpsrc   Local source of the operation (local dart pointer)
 @param     nbytes   The size of the local buffer in bytes
 */
dart_ret_t dart_put(dart_gptr_t gpdest, dart_gptr_t lpsrc, size_t nbytes);

/**
 @brief   Non-blocking one-sided get

 @param     lpdest   Local destination of the operation (local dart pointer)
 @param      gpsrc   Global source of the operation (global dart pointer)
 @param     nbytes   The size of the local buffer in bytes
 */
dart_ret_t dart_get(dart_gptr_t lpdest, dart_gptr_t gpsrc, size_t nbytes);


/**
 @brief   Blocking one-sided put

 @param     gpdest   Global destination of the operation (global dart pointer)
 @param      lpsrc   Local source of the operation (local dart pointer)
 @param     nbytes   The size of the local buffer in bytes
 */
dart_ret_t dart_put_blocking(dart_gptr_t gpdest,
			     dart_gptr_t lpsrc, size_t nbytes);

/**
 @brief   Blocking one-sided get

 @param     lpdest   Local destination of the operation (local dart pointer)
 @param      gpsrc   Global source of the operation (global dart pointer)
 @param     nbytes   The size of the local buffer in bytes
 */
dart_ret_t dart_get_blocking(dart_gptr_t lpdest,
			     dart_gptr_t gpsrc, size_t nbytes);


/**
 @brief   One-sided put (handle version)

 @param     gpdest   Global destination of the operation (global dart pointer)
 @param      lpsrc   Local source of the operation (local dart pointer)
 @param     nbytes   The size of the local buffer in bytes
 @param     handle   Handle used to test or wait for completion
 */
dart_ret_t dart_put_handle(dart_gptr_t gpdest,
			   dart_gptr_t lpsrc, size_t nbytes,
			   dart_handle_t handle);

/**
 @brief   One-sided get (handle version)

 @param     lpdest   Local destination of the operation (local dart pointer)
 @param      gpsrc   Global source of the operation (global dart pointer)
 @param     nbytes   The size of the local buffer in bytes
 @param     handle   Handle used to test or wait for completion
 */
dart_ret_t dart_get_handle(dart_gptr_t lpdest,
			   dart_gptr_t gpsrc, size_t nbytes,
			   dart_handle_t handle);


/**
 @brief   Non-blocking one-sided strided put

 @param     gpdest   Global destination of the operation (global dart pointer)
 @param      lpsrc   Local source of the operation (local dart pointer)
 @param    blocksz   Block size in bytes
 @param     stride   Stride length in bytes
 @param    nblocks   Number of blocks
 */
dart_ret_t dart_put_strided(dart_gptr_t gpdest, dart_gptr_t lpsrc,
			    size_t blocksz, size_t stride,
			    size_t nblocks);

/**
 @brief   Non-blocking one-sided strided get

 @param     lpdest   Local destination of the operation (local dart pointer)
 @param      gpsrc   Global source of the operation (global dart pointer)
 @param    blocksz   Block size in bytes
 @param     stride   Stride length in bytes
 @param    nblocks   Number of blocks
 */
dart_ret_t dart_get_strided(dart_gptr_t lpdest, dart_gptr_t gpsrc,
			    size_t blocksz, size_t stride,
			    size_t nblocks);


/**
 @brief   Blocking one-sided strided put

 @param     gpdest   Global destination of the operation (global dart pointer)
 @param      lpsrc   Local source of the operation (local dart pointer)
 @param    blocksz   Block size in bytes
 @param     stride   Stride length in bytes
 @param    nblocks   Number of blocks
 */
dart_ret_t dart_put_strided_blocking(dart_gptr_t gpdest,
				     dart_gptr_t lpsrc,
				     size_t blocksz, size_t stride,
				     size_t nblocks);

/**
 @brief   Blocking one-sided strided get

 @param     lpdest   Local destination of the operation (local dart pointer)
 @param      gpsrc   Global source of the operation (global dart pointer)
 @param    blocksz   Block size in bytes
 @param     stride   Stride length in bytes
 @param    nblocks   Number of blocks
 */
dart_ret_t dart_get_strided_blocking(dart_gptr_t lpdest,
				     dart_gptr_t gpsrc,
				     size_t blocksz, size_t stride,
				     size_t nblocks);


/**
 @brief   One-sided strided put (handle version)

 @param     gpdest   Global destination of the operation (global dart pointer)
 @param      lpsrc   Local source of the operation (local dart pointer)
 @param    blocksz   Block size in bytes
 @param     stride   Stride length in bytes
 @param    nblocks   Number of blocks
 @param     handle   Handle used to test or wait for completion
 */
dart_ret_t dart_put_strided_handle(dart_gptr_t gpdest,
				   dart_gptr_t lpsrc,
				   size_t blocksz, size_t stride,
				   size_t nblocks,
				   dart_handle_t handle);


/**
 @brief   One-sided strided get (handle version)

 @param     lpdest   Local destination of the operation (local dart pointer)
 @param      gpsrc   Global source of the operation (global dart pointer)
 @param    blocksz   Block size in bytes
 @param     stride   Stride length in bytes
 @param    nblocks   Number of blocks
 @param     handle   Handle used to test or wait for completion
 */
dart_ret_t dart_get_strided_handle(dart_gptr_t lpdest,
				   dart_gptr_t gpsrc,
				   size_t blocksz, size_t stride,
				   size_t nblocks,
				   dart_handle_t handle);


/**
 @brief   Non-blocking one-sided transform operation

 @param     gpdest   Global destination of the operation (global dart pointer)
 @param      lpsrc   Local source of the operation (local dart pointer)
 @param      nelem   Number of elements to transform
 @param      dtype   Datatype of the elements
 @param         op   Operation to apply
 */
dart_ret_t dart_transform(dart_gptr_t gpdest,
			  dart_gptr_t lpsrc, size_t nelem,
			  dart_datatype_t dtype,
			  dart_operation_t op);


/**
 @brief Guarantees local completion of all pending puts, gets, and
        transform operations to a single target

 @param     gptr    Global pointer that specifies the memory window/segment
 @param   target    The target of the flush operation
*/
dart_ret_t dart_flush_local(dart_gptr_t gptr,
			    dart_unit_t target);


/**
 @brief Guarantees local and remote completion of all pending puts,
        gets, and transform operations to a single target

 @param     gptr    Global pointer that specifies the memory window/segment
 @param   target    The target of the flush operation
*/
dart_ret_t dart_flush(dart_gptr_t gptr,
		      dart_unit_t target);


/**
 @brief Guarantees local completion of all pending puts, gets, and
        transform operations to all targets

 @param     gptr    Global pointer that specifies the memory window/segment
*/
dart_ret_t dart_flush_local_all(dart_gptr_t gptr);


/**
 @brief Guarantees local and remote completion of all pending puts,
        gets, and transform operations to all targets

 @param     gptr    Global pointer that specifies the memory window/segment
*/
dart_ret_t dart_flush_all(dart_gptr_t gptr);



#define DART_INTERFACE_OFF

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DART__ONESIDED_H */

