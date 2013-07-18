#ifndef DART_ONESIDED_H_INCLUDED
#define DART_ONESIDED_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#define DART_INTERFACE_ON

typedef struct dart_handle_struct *dart_handle_t;

/* blocking versions of one-sided communication operations */
dart_ret_t dart_get(void *dest, dart_gptr_t ptr, size_t nbytes);
dart_ret_t dart_put(dart_gptr_t ptr, void *src, size_t nbytes);

/* non-blocking versions returning a handle */
dart_ret_t dart_get_nb(void *dest, dart_gptr_t ptr, 
		       size_t nbytes, dart_handle_t *handle);
dart_ret_t dart_put_nb(dart_gptr_t ptr, void *src, 
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

#endif /* DART_ONESIDED_H_INCLUDED */
