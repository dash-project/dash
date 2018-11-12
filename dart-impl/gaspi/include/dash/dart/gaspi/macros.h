#ifndef __MACROS_H__
#define __MACROS_H__

#include <stdlib.h>


/*
 * Define a print macro to print debug info. This macro helps us to keep track of what is going on with our code.
 * It exists to help us, so if you don't want any of these debug messages just remove the flag -DDEBUG from the Makefile and the generated code
 * won't have any instruction related with this macro. (Yes it will save you space...)
 */
#ifdef DEBUG
#include <stdio.h>
#define log(level, format, ...)					\
  do{								\
    fprintf(stderr, "%s  %s:%d: ", level, __FILE__, __LINE__);	\
    fprintf(stderr, format, ## __VA_ARGS__);			\
    fprintf(stderr, "\n");					\
  }while(0)
#else
#define log(level, str, ...)
#endif

#define I "INFO"
#define W "WARNNING"
#define E "ERROR"
#define F "FATAL ERROR"

/*
 * Simple macro to malloc data and check if the allocation was successfull.
 * A simple use can be char* string = alloc(char, 5); which allocates a string with 5 characters.
 */
#define alloc(type, how_many)				\
  (type *) __alloc(malloc(how_many * sizeof(type)));	

static inline void* __alloc(void* x){
  if(x)
    return x;
  log(F,"malloc failed.");				
  exit(1);
  return 0;
}

#endif
    
