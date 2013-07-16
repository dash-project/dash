#ifndef DART_STDINT_H_INCLUDED
#define DART_STDINT_H_INCLUDED

#include <limits.h>

#if (UINT_MAX==4294967295)
typedef unsigned int uint32_t;
typedef int int32_t;
typedef unsigned long long uint64_t;
typedef long long int64_t;
typedef unsigned short uint16_t;
typedef short int16_t;
#endif




#endif /* DART_STDINT_H_INCLUDED */
