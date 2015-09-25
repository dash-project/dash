#ifndef HANDLE_QUEUE_H
#define HANDLE_QUEUE_H
#include <stdlib.h>
#include "dart.h"
#include "dart_types.h"
#include "dart_communication.h"
#include "dart_communication_priv.h"

struct queue_node{
    struct dart_handle_struct * handle;
    struct queue_node * next;
};

typedef struct queue{
    size_t size;
    struct queue_node * front;
    struct queue_node * back;
}queue_t;


dart_ret_t init_handle_queue(queue_t * q);
dart_ret_t destroy_handle_queue(queue_t * q);
dart_ret_t enqueue_handle(queue_t * q, struct dart_handle_struct handle);
dart_ret_t dequeue_handle(queue_t * q);
dart_ret_t front_handle(queue_t * q, struct dart_handle_struct * handle);

#endif /* HANDLE_QUEUE_H */
