#ifndef DART__COLLECTIVE_H
#define DART__COLLECTIVE_H

/* DART v4.0 */

#ifdef __cplusplus
extern "C" {
#endif

#define DART_INTERFACE_ON

/**
 * \file dart_collective.h
 *
 * ### DART collective operations ###
 *
 */

dart_ret_t dart_team_barrier(dart_team_t team);


dart_ret_t dart_team_bcast(dart_team_t team,
			   dart_unit_t root,
			   void *buffer, int count,
			   dart_datatype_t datatype);


dart_ret_t dart_team_reduce(dart_team_t team,
			    dart_unit_t root,
			    const void* sendbuf, void* recvbuf,
			    int count,
			    dart_datatype_t datatype,
			    dart_operation_t operation);


dart_ret_t dart_team_allreduce(dart_team_t team,
			       const void* sendbuf, void* recvbuf,
			       int count,
			       dart_datatype_t datatype,
			       dart_operation_t operation);


dart_ret_t dart_team_gather(dart_team_t team,
			    dart_unit_t root,
			    const void* sendbuf,
			    int sendcount, dart_datatype_t sendtype,
			    void* recvbuf,
			    int recvcount, dart_datatype_t recvtype);


dart_ret_t dart_team_allgather(dart_team_t team,
			       const void* sendbuf,
			       int sendcount, dart_datatype_t sendtype,
			       void* recvbuf,
			       int recvcount, dart_datatype_t recvtype);


dart_ret_t dart_team_scatter(dart_team_t team,
			     dart_unit_t root,
			     const void* sendbuf,
			     int sendcount, dart_datatype_t sendtype,
			     void* recvbuf,
			     int recvcount, dart_datatype_t recvtype);


dart_ret_t dart_team_allscatter(dart_team_t team,
				const void* sendbuf,
				int sendcount, dart_datatype_t sendtype,
				void* recvbuf,
				int recvcount, dart_datatype_t recvtype);


#define DART_INTERFACE_OFF

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* DART__COLLECTIVE_H */

