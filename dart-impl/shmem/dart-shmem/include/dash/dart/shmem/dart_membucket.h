#ifndef DART_MEMBUCKET_H_INCLUDED
#define DART_MEMBUCKET_H_INCLUDED

#include <stdio.h>
#include <dash/dart/shmem/dart_membucket_priv.h>

#include <dash/dart/shmem/extern_c.h>
EXTERN_C_BEGIN

struct dart_opaque_membucket;
typedef struct dart_opaque_membucket* dart_membucket;
#define DART_MEMBUCKET_NULL ((dart_membucket)0)

dart_membucket dart_membucket_create(void *pos, size_t size);
void dart_membucket_destroy(dart_membucket bucket);

void* dart_membucket_alloc(dart_membucket bucket, size_t size);
int dart_membucket_free(dart_membucket bucket, void* pos);

void dart_membucket_print(dart_membucket bucket, FILE* f);

EXTERN_C_END

#endif /* DART_MEMBUCKET_H_INCLUDED */
