#ifndef DART_GASPI_H
#define DART_GASPI_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include <stdbool.h>
#include <GASPI.h>
#include <dash/dart/if/dart.h>
#include <dash/dart/if/dart_types.h>
#include <dash/dart/gaspi/dart_seg_stack.h>
#include <dash/dart/gaspi/gaspi_utils.h>

#define DART_MAX_TEAM_NUMBER (256)
#define DART_INTERFACE_ON
#define DART_GASPI_BUFFER_SIZE 1 << 16 //2^(sizeof(gaspi_rank_t) * 8) * sizeof(int32_t)

extern gaspi_rank_t dart_gaspi_rank_num;
extern gaspi_rank_t dart_gaspi_rank;
extern const gaspi_segment_id_t dart_gaspi_buffer_id;
extern const gaspi_segment_id_t dart_onesided_seg;

#define PUT_COMPLETION_VALUE 255
extern const gaspi_segment_id_t put_completion_src_seg;
extern const gaspi_segment_id_t put_completion_dst_seg;
extern char put_completion_dst_storage;

extern gaspi_pointer_t dart_gaspi_buffer_ptr;
extern const gaspi_segment_id_t dart_mempool_seg_localalloc;
extern const gaspi_segment_id_t dart_fallback_seg;
extern bool dart_fallback_seg_is_allocated;

extern seg_stack_t pool_gaspi_seg_ids;
extern seg_stack_t pool_dart_seg_ids;

#define DART_INTERFACE_OFF

#ifdef __cplusplus
}
#endif

#endif /* DART_GASPI_H */
