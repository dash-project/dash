#ifndef DART_HELPER_THREAD_H_INCLUDED
#define DART_HELPER_THREAD_H_INCLUDED

#include <pthread.h>
#include <dash/dart/if/dart.h>

#define MAXNUM_WORK_ITEMS    1024

#define WORK_NONE      1
#define WORK_SHUTDOWN  2
#define WORK_NB_SEND   3
#define WORK_NB_RECV   4
#define WORK_NB_GET    5
#define WORK_NB_PUT    6


typedef struct work_item
{
  int selector;
  
  void           *buf;
  size_t         nbytes;
  dart_team_unit_t    unit;
  dart_team_t    team;
  dart_gptr_t    gptr;
  dart_handle_t  *handle;
} 
work_item_t;


struct work_queue
{
  pthread_mutex_t   lock;
  pthread_cond_t    cond_not_empty;
  pthread_cond_t    cond_not_full;
  int               nitems;
  int               next_push;
  int               next_pop;

  work_item_t work[MAXNUM_WORK_ITEMS];
};


void dart_work_queue_init();

void dart_work_queue_pop_item( work_item_t *item );
void dart_work_queue_push_item( work_item_t *item );
void dart_work_queue_shutdown();


void dart_helper_thread_send( work_item_t *item );
void dart_helper_thread_recv( work_item_t *item );

void* dart_helper_thread(void*);


#endif /* DART_HELPER_THREAD_H_INCLUDED */
