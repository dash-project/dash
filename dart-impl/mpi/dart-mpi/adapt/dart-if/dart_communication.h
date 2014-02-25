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

dart_ret_t dart_barrier(dart_team_t team);

dart_ret_t dart_bcast(void *buf, size_t nbytes, 
		      dart_unit_t root, dart_team_t team);

dart_ret_t dart_scatter(void *sendbuf, void *recvbuf, size_t nbytes, 
			dart_unit_t root, dart_team_t team);

dart_ret_t dart_gather(void *sendbuf, void *recvbuf, size_t nbytes, 
		       dart_unit_t root, dart_team_t team);
  
dart_ret_t dart_allgather(void *sendbuf, void *recvbuf, size_t nbytes, 
			  dart_team_t team);



typedef struct dart_handle_struct *dart_handle_t;

/* blocking versions of one-sided communication operations */
dart_ret_t dart_get_blocking(void *dest, dart_gptr_t ptr, size_t nbytes);
dart_ret_t dart_put_blocking(dart_gptr_t ptr, void *src, size_t nbytes);

/* non-blocking versions returning a handle */
dart_ret_t dart_get(void *dest, dart_gptr_t ptr, 
		    size_t nbytes, dart_handle_t *handle);
dart_ret_t dart_put(dart_gptr_t ptr, void *src, 
		    size_t nbytes, dart_handle_t *handle);
  
/* wait and test for the completion of a single handle */
dart_ret_t dart_wait(dart_handle_t handle);
dart_ret_t dart_test(dart_handle_t handle);

/* wait and test for the completion of multiple handles */
dart_ret_t dart_waitall(dart_handle_t *handle, size_t n);
dart_ret_t dart_testall(dart_handle_t *handle, size_t n);


#define DART_INTERFACE_OFF


#ifdef __cplusplus
}
#endif

#endif /* DART_COMMUNICATION_H_INCLUDED */
