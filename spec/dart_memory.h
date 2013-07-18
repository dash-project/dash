#ifndef DART_MEMORY_H_INCLUDED
#define DART_MEMORY_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#define DART_INTERFACE_ON

/*
 Allocates nbytes of memory in the global address space of the calling
 unit and returns a global pointer to it.  This is *not* a collective
 function but a local one.
 */
dart_ret_t dart_memalloc(size_t nbytes, dart_gptr_t *gptr);
dart_ret_t dart_memfree(dart_gptr_t gptr);


dart_ret_t dart_team_memalloc_aligned(dart_team_t teamid, 
				      size_t nbytes, dart_gptr_t *gptr);
dart_ret_t dart_team_memfree(dart_team_t teamid, dart_gptr_t gptr);

#define DART_INTERFACE_OFF

#ifdef __cplusplus
}
#endif


#endif /* DART_MEMORY_H_INCLUDED */
