#ifndef DART_COLLECTIVE_H_INCLUDED
#define DART_COLLECTIVE_H_INCLUDED

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

#define DART_INTERFACE_OFF


#ifdef __cplusplus
}
#endif

#endif /* DART_COLLECTIVE_H_INCLUDED */
