#ifndef DART_SEG_STACK_H
#define DART_SEG_STACK_H
#include <dash/dart/if/dart_types.h>
#include <GASPI.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct seg_stack{
    int top;
    size_t size;
    gaspi_segment_id_t * segids;
}seg_stack_t;

dart_ret_t seg_stack_init(seg_stack_t * stack, gaspi_segment_id_t begin, size_t count);
dart_ret_t seg_stack_finish(seg_stack_t * stack);
dart_ret_t seg_stack_fill(seg_stack_t * stack, gaspi_segment_id_t begin, size_t count);
dart_ret_t seg_stack_pop(seg_stack_t * stack, gaspi_segment_id_t * segid_out);
dart_ret_t seg_stack_push(seg_stack_t * stack, gaspi_segment_id_t segid_in);
bool seg_stack_isempty(seg_stack_t * stack);
bool seg_stack_isfull(seg_stack_t * stack);


#endif /* DART_SEG_STACK_H */
