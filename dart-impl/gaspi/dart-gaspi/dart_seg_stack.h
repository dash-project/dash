#ifndef DART_SEG_STACK_H
#define DART_SEG_STACK_H
#include <GASPI.h>
#include <stdlib.h>

#define SEG_STACK_OK 0
#define SEG_STACK_ERROR 1

typedef struct seg_stack{
    int top;
    size_t size;
    gaspi_segment_id_t * segids;
}seg_stack_t;

int seg_stack_init(seg_stack_t * stack, size_t count);
int seg_stack_finish(seg_stack_t * stack);
int seg_stack_fill(seg_stack_t * stack, gaspi_segment_id_t begin, size_t count);
int seg_stack_pop(seg_stack_t * stack, gaspi_segment_id_t * segid_out);
int seg_stack_push(seg_stack_t * stack, gaspi_segment_id_t segid_in);
int seg_stack_isempty(seg_stack_t * stack);
int seg_stack_isfull(seg_stack_t * stack);


#endif /* DART_SEG_STACK_H */
