/*
 * dart_onesided.h
 *
 *  Created on: Apr 7, 2013
 *      Author: maierm
 */

#ifndef DART_ONESIDED_H_
#define DART_ONESIDED_H_

/*
 --- DART onesided communication ---
 */

typedef struct
{
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

#endif /* DART_ONESIDED_H_ */
