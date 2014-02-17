#ifndef DART_MEMPOOL_H_INCLUDED
#define DART_MEMPOOL_H_INCLUDED

#include "extern_c.h"

EXTERN_C_BEGIN

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "dart_mempool_private.h"

struct dart_opaque_mempool;
typedef struct dart_opaque_mempool* dart_mempool;
#define DART_MEMPOOL_NULL ((dart_mempool)0)

dart_mempool dart_mempool_create(void *pos, size_t size);
void dart_mempool_destroy(dart_mempool pool);

void* dart_mempool_alloc(dart_mempool pool, size_t size);
int dart_mempool_free(dart_mempool pool, void* pos);

void dart_mempool_print(dart_mempool pool, FILE* f);


EXTERN_C_END

#endif /* DART_MEMPOOL_H_INCLUDED */
