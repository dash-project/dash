/*
 * dart_communication.h
 *
 *  Created on: Apr 9, 2013
 *      Author: maierm
 */

#ifndef DART_COMMUNICATION_H_
#define DART_COMMUNICATION_H_

/*
 --- DART collective communication ---

 It will be useful to include some set of collective communication and
 synchronization calls in the runtime such that for example when
 developing or using the DASH template library we don't have to step
 outside the DASH to do simple things such as barriers.xx  */

/* broadcast of data from one team member to all others
 semantics are like in MPI, but the broadcast works on raw
 bytes and not with datatypes
 */
int dart_bcast(void* buf, size_t nbytes, int root, int team);

/*
 gather and scatter with similar semantics as in MPI;
 size specifies the message size between each pair of
 communicating processes
 */
int dart_scatter(void *sendbuf, void *recvbuf, size_t nbytes, int root,
		int team);
int dart_gather(void *sendbuf, void *recvbuf, size_t nbytes, int root, int team);

/*
 --- DART onesided communication ---
 */

typedef struct {
} dart_handle_t;

/* blocking versions of one-sided communication operations */
void dart_get(void *dest, gptr_t ptr, size_t nbytes);
void dart_put(gptr_t ptr, void *src, size_t nbytes);

/* non-blocking versions returning a handle */
dart_handle_t dart_get_nb(void *dest, gptr_t ptr, size_t nbytes);
dart_handle_t dart_put_nb(gptr_t ptr, void *src, size_t nbytes);

/* wait and test for the completion of a single handle */
int dart_wait(dart_handle_t handle);
int dart_test(dart_handle_t handle);

/* wait and test for the completion of multiple handles */
int dart_waitall(dart_handle_t *handle);
int dart_testall(dart_handle_t *handle);

/* TODO:
 - Do we need bulk version of the above (like in Gasnet)
 - Do we need a way to specify the data to trasmit in
 a more complex way - strides, offsets, etc. (like in Global Arrays)
 */

#endif /* DART_COMMUNICATION_H_ */
