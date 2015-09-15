#include "dart_seg_stack.h"
#include <assert.h>

int seg_stack_init(seg_stack_t * stack, size_t count)
{
    stack->segids = (gaspi_segment_id_t *) malloc(sizeof(gaspi_segment_id_t) * count);

    if(stack->segids == NULL)
    {
        return SEG_STACK_ERROR;
    }

    stack->top  = -1;
    stack->size = count;

    return SEG_STACK_OK;
}

int seg_stack_isempty(seg_stack_t * stack)
{
    return stack->top < 0;
}

int seg_stack_isfull(seg_stack_t * stack)
{
    return stack->top == (stack->size - 1);
}

int seg_stack_push(seg_stack_t * stack, gaspi_segment_id_t segid_in)
{
    if(seg_stack_isfull(stack) || stack->segids == NULL)
    {
        return SEG_STACK_ERROR;
    }
    stack->segids[++(stack->top)] = segid_in;

    return SEG_STACK_OK;
}

int seg_stack_pop(seg_stack_t * stack, gaspi_segment_id_t * segid_out)
{
    if(seg_stack_isempty(stack) || stack->segids == NULL)
    {
        return SEG_STACK_ERROR;
    }
    *segid_out = stack->segids[(stack->top)--];
    return SEG_STACK_OK;
}

int seg_stack_fill(seg_stack_t * stack, gaspi_segment_id_t begin, size_t count)
{
    if(seg_stack_isfull(stack) || !seg_stack_isempty(stack) || stack->segids == NULL)
    {
        return SEG_STACK_ERROR;
    }

    for(gaspi_segment_id_t i = begin ; i < (begin + count) ; ++i)
    {
        if(seg_stack_push(stack, i) != SEG_STACK_OK)
        {
            return SEG_STACK_ERROR;
        }
    }
    return SEG_STACK_OK;
}

int seg_stack_finish(seg_stack_t * stack)
{
    if(stack->segids != NULL)
    {
        free(stack->segids);
    }
    stack->top = -1;
    stack->size = 0;

    return SEG_STACK_OK;
}
