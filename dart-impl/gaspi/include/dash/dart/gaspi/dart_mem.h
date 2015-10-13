//https://github.com/cloudwu/buddy

#ifndef BUDDY_MEMORY_ALLOCATION_H
#define BUDDY_MEMORY_ALLOCATION_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <inttypes.h>

#include <GASPI.h>
#include "handle_queue.h"

#define DART_MAX_TEAM_NUMBER (256)
#define DART_MAX_LENGTH (1024*1024*16)

struct dart_buddy {
    int level;
    uint8_t tree[1];
};

extern queue_t * dart_non_collective_rma_request;
extern char* dart_mempool_localalloc;
extern struct dart_buddy* dart_localpool;

extern const gaspi_segment_id_t dart_transferpool_seg;
extern const gaspi_size_t dart_transferpool_size;
extern struct dart_buddy* dart_transferpool;

struct dart_buddy* dart_buddy_new(int level);
void dart_buddy_delete(struct dart_buddy *);
uint64_t dart_buddy_alloc(struct dart_buddy *, size_t size);
int dart_buddy_free(struct dart_buddy *, uint64_t offset);
int buddy_size(struct dart_buddy *, uint64_t offset);
void buddy_dump(struct dart_buddy *);

#endif
