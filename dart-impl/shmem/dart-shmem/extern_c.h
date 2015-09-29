#ifndef EXTERN_C_H_INCLUDED
#define EXTERN_C_H_INCLUDED

/*
  EXTERN_C_BEGIN and EXTERN_C_END
  macros for C++ interoperability; just makes
  it easier for emacs indentation 
*/

#ifdef __cplusplus
#define EXTERN_C_BEGIN  extern "C" {
#define EXTERN_C_END    }
#else
#define EXTERN_C_BEGIN
#define EXTERN_C_END
#endif

#endif /* EXTERN_C_H_INCLUDED */

