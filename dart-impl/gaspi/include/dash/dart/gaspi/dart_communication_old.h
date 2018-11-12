#ifndef DART_COMMUNICATION_H_INCLUDED
#define DART_COMMUNICATION_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#define DART_INTERFACE_ON

/*
  It will be useful to have a set of basic collective communication
  routines in DART. The semantics of the routines below are the same
  as with MPI. The only difference is that DART doesn't specify data
  types and the operates on raw buffers instead. Message size is thus
  specified in bytes.
 */
/**
 * possible
 */
dart_ret_t dart_barrier(dart_team_t team);

/**
 * these collective functions exists in score-p,
 * but they operate over all processes
 * -> not for groups
 */
dart_ret_t dart_bcast(void *buf, size_t nbytes, 
		      dart_unit_t root, dart_team_t team);

dart_ret_t dart_scatter(void *sendbuf, void *recvbuf, size_t nbytes, 
			dart_unit_t root, dart_team_t team);

dart_ret_t dart_gather(void *sendbuf, void *recvbuf, size_t nbytes, 
		       dart_unit_t root, dart_team_t team);
  
dart_ret_t dart_allgather(void *sendbuf, void *recvbuf, size_t nbytes, 
			  dart_team_t team);



  typedef struct dart_handle_struct *dart_handle_t;
  
  /* 
     'REGULAR' variants: 
     
     When these functions return, neither local nor remote completion
     is guaranteed. A later fence operation is needed to guarantee
     local and remote completion.
  */
  
  /**
   *
   * void * dest and  *src does not work efficiently for GASPI
   * because GASPI uses a segment and an offset to specify the
   * data transfer
   * 
   * its more consistent to use dart_gptr_t for source and destination
   * 
   */ 
  dart_ret_t dart_get(void *dest, dart_gptr_t ptr, size_t nbytes);
  dart_ret_t dart_put(dart_gptr_t ptr, void *src,  size_t nbytes);
  
  /*
    'HANDLE' variants:
    
    Neither local nor remote completion is guaranteed. A later
    dart_wait*() call or a fence operation is needed to guarantee
    completion.
  */
  /**
   * case get : local completion == remote completion -> no problem in GASPI
   * 			can be hard to distribute the rma operations over queues
   * case put : ensures only the local completion
   */
  dart_ret_t dart_get_handle(void *dest, dart_gptr_t ptr, 
			     size_t nbytes, dart_handle_t *handle);
  dart_ret_t dart_put_handle(dart_gptr_t ptr, void *src, 
			     size_t nbytes, dart_handle_t *handle);
  
  /*
    'BLOCKING' variants:
    
    Both local and remote completion is guaranteed.
  */
  /**
   * case one-sided blocking get: no problem to realize in GASPI
   * case one-sided blocking put: impossible
   */
  dart_ret_t dart_get_blocking(void *dest, dart_gptr_t ptr, 
			       size_t nbytes);
  dart_ret_t dart_put_blocking(dart_gptr_t ptr, void *src, 
			       size_t nbytes);
  
  /*
    Guarantees local and remote completion of all outstanding puts and
    gets on a certain memory allocation / window / segment for the
    target unit specified in gptr. -> MPI_Win_flush() 
  */
  /**
   * really hard to implement in GASPI
   * 
   * -> no ussage of this function in dart and dash-lib
   */
  dart_ret_t dart_fence(dart_gptr_t gptr);
  
  /* 
     Guarantees local and remote completion of all outstanding puts and
     gets on a certain memory allocation / window / segment for all
     target units. -> MPI_Win_flush_all() 
  */
  /**
   * really hard to implement in GASPI
   * 
   * -> no ussage of this function in dart and dash-lib
   */
  dart_ret_t dart_fence_all(dart_gptr_t gptr);
  
  /* 
     wait for the local and remote completion of operation(s)
  */
  /**
   * local completion is no problem
   * remote completion works only for get operations
   */
  dart_ret_t dart_wait(dart_handle_t handle);
  dart_ret_t dart_waitall(dart_handle_t *handle, size_t n);
  
  /* 
     wait for the local completion of operation(s)
  */
  /**
   * possible with intelligent queue handling in GASPI
   */
  dart_ret_t dart_wait_local(dart_handle_t handle);
  dart_ret_t dart_waitall_local(dart_handle_t *handle, size_t n);

  /* 
     wait for the local completion of operation(s)
  */
  /**
   * possible with intelligent queue handling in GASPI
   */
  dart_ret_t dart_test_local(dart_handle_t handle, 
			     int32_t *result);
  dart_ret_t dart_testall_local(dart_handle_t *handle, 
				size_t n, int32_t *result);
  
#define DART_INTERFACE_OFF


#ifdef __cplusplus
}
#endif

#endif /* DART_COMMUNICATION_H_INCLUDED */
