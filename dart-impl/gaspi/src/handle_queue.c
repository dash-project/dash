#include <dash/dart/gaspi/handle_queue.h>

#include <assert.h>
#include <stdio.h>

dart_ret_t init_handle_queue(queue_t * q)
{
    q->front = NULL;
    q->back = NULL;
    q->size = 0;

    return DART_OK;
}

dart_ret_t destroy_handle_queue(queue_t * q)
{
    int ret = DART_OK;
    while(q->size > 0)
    {
        ret = dequeue_handle(q);
        if(ret != DART_OK)
        {
            fprintf(stderr, "Error in destroy_handle_queue\n");
            return ret;
        }
    }
    q->front = NULL;
    q->back = NULL;

    return ret;
}

dart_ret_t enqueue_handle(queue_t * q, struct dart_handle_struct * handle)
{
    struct queue_node * node = (struct queue_node *) malloc(sizeof(struct queue_node));
    assert(node);

    struct dart_handle_struct * tmp_handle = (struct dart_handle_struct *) malloc(sizeof(struct dart_handle_struct));
    assert(tmp_handle);

    tmp_handle->local_seg  = handle->local_seg;
    tmp_handle->remote_seg = handle->remote_seg;
    tmp_handle->queue      = handle->queue;

    node->handle = tmp_handle;
    node->next = NULL;
    q->size = q->size + 1;

    if(q->front == NULL && q->back == NULL)
    {
        q->front = node;
        q->back  = node;

        return DART_OK;
    }
    if(q->back->next != NULL)
    {
        fprintf(stderr, "Error in enqueue function: back->next != NULL\n");
        return DART_ERR_INVAL;
    }
    q->back->next = node;
    q->back = node;

    return DART_OK;
}

dart_ret_t front_handle(queue_t * q, struct dart_handle_struct * handle)
{
    if(q->front == NULL)
    {
        fprintf(stderr, "Error in queue_front function: queue is empty\n");
        return DART_ERR_NOTINIT;
    }

    handle->local_seg  = q->front->handle->local_seg;
    handle->remote_seg = q->front->handle->remote_seg;
    handle->queue      = q->front->handle->queue;

    return DART_OK;
}

dart_ret_t dequeue_handle(queue_t * q)
{
    struct queue_node * tmp = q->front;
    if(tmp == NULL)
    {
        fprintf(stderr, "Queue is empty\n");
        return DART_ERR_NOTINIT;
    }
    if(q->front == q->back)
    {
        q->front = NULL;
        q->back = NULL;
    }
    else
    {
        q->front = tmp->next;
    }
    free(tmp->handle);
    free(tmp);
    q->size = q->size - 1;

    return DART_OK;
}
