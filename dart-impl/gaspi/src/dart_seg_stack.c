#include <dash/dart/gaspi/dart_seg_stack.h>
#include <dash/dart/base/logging.h>

#include <assert.h>
#include <stdio.h>

dart_ret_t seg_stack_init(seg_stack_t * stack, gaspi_segment_id_t begin, size_t count)
{
    DART_LOG_TRACE("seg_stack_init");
    stack->segids = (gaspi_segment_id_t *) malloc(sizeof(gaspi_segment_id_t) * count);

    if(stack->segids == NULL)
    {
        return DART_ERR_OTHER;
    }

    stack->top  = -1;
    stack->size = count;

    for(gaspi_segment_id_t i = (begin + count-1); i >= begin  ; --i)
    {
        if(seg_stack_push(stack, i) != DART_OK)
        {
            return DART_ERR_OTHER;
        }
    }

    return DART_OK;
}

bool seg_stack_isempty(seg_stack_t * stack)
{
    return stack->top < 0;
}

bool seg_stack_isfull(seg_stack_t * stack)
{
    return stack->top == (stack->size - 1);
}

dart_ret_t seg_stack_push(seg_stack_t * stack, gaspi_segment_id_t segid_in)
{
    if(stack->segids == NULL || seg_stack_isfull(stack))
    {
        DART_LOG_ERROR("seg_stack_push: seg_stack is full, no further push possible");
        return DART_ERR_OTHER;
    }
    stack->segids[++(stack->top)] = segid_in;

    return DART_OK;
}

dart_ret_t seg_stack_pop(seg_stack_t * stack, gaspi_segment_id_t * segid_out)
{
    if(stack->segids == NULL || seg_stack_isempty(stack))
    {
        DART_LOG_ERROR("seg_stack_pop: seg_stack is empty, no further pop possible")
        return DART_ERR_OTHER;
    }
    *segid_out = stack->segids[(stack->top)--];

    return DART_OK;
}

dart_ret_t seg_stack_fill(seg_stack_t * stack, gaspi_segment_id_t begin, size_t count)
{
    if(stack->segids == NULL || seg_stack_isfull(stack) || !seg_stack_isempty(stack))
    {
        DART_LOG_ERROR("seg_stack_fill: stack->segids == NULL || seg_stack_isfull || !seg_stack_isempty");
        return DART_ERR_OTHER;
    }

    for(gaspi_segment_id_t i = begin ; i < (begin + count) ; ++i)
    {
        if(seg_stack_push(stack, i) != DART_OK)
        {
            DART_LOG_ERROR("seg_stack_fill: could not push onto seg_stack while seg_stack_fill");
            return DART_ERR_OTHER;
        }
    }
    return DART_OK;
}

dart_ret_t seg_stack_finish(seg_stack_t * stack)
{
    if(stack->segids != NULL)
    {
        free(stack->segids);
    }
    stack->top = -1;
    stack->size = 0;
    return DART_OK;
}
