
#include <pthread.h>
#include <stdio.h>

#include <dash/dart/shmem/shmem_p2p_if.h>
#include <dash/dart/shmem/dart_helper_thread.h>

static struct work_queue queue = {
  PTHREAD_MUTEX_INITIALIZER,
  PTHREAD_COND_INITIALIZER, 
  PTHREAD_COND_INITIALIZER };

void dart_work_queue_init()
{
  queue.nitems    = 0;
  queue.next_push = 0;
  queue.next_pop  = 0;
  
  int i;
  for( i=0; i<MAXNUM_WORK_ITEMS; i++ ) {
    queue.work[i].selector=WORK_NONE;
  }
}

void dart_work_queue_shutdown()
{
  work_item_t item;
  item.selector = WORK_SHUTDOWN;

  dart_work_queue_push_item(&item);
}

void dart_work_queue_pop_item( work_item_t *item )
{
  pthread_mutex_lock( &(queue.lock) );

  while(queue.nitems==0) {
    pthread_cond_wait( &(queue.cond_not_empty),
		       &(queue.lock) );
  }

  item->selector = queue.work[queue.next_pop].selector;
  item->buf      = queue.work[queue.next_pop].buf;
  item->nbytes   = queue.work[queue.next_pop].nbytes;
  item->unit     = queue.work[queue.next_pop].unit;
  item->team     = queue.work[queue.next_pop].team;
  item->gptr     = queue.work[queue.next_pop].gptr;
  item->handle   = queue.work[queue.next_pop].handle;
  
  queue.next_pop = (queue.next_pop+1)%MAXNUM_WORK_ITEMS;
  queue.nitems--;

  if( queue.nitems==MAXNUM_WORK_ITEMS-1) {
    pthread_cond_signal( &(queue.cond_not_full) );
  }

  pthread_mutex_unlock( &(queue.lock) );
}

void dart_work_queue_push_item( work_item_t *item )
{
  pthread_mutex_lock( &(queue.lock) );

  while(queue.nitems>=MAXNUM_WORK_ITEMS) {
    pthread_cond_wait( &(queue.cond_not_full),
		       &(queue.lock) );
  }

  queue.work[queue.next_push].selector = item->selector;

  queue.work[queue.next_push].buf      = item->buf;
  queue.work[queue.next_push].nbytes   = item->nbytes;
  queue.work[queue.next_push].unit     = item->unit;
  queue.work[queue.next_push].team     = item->team;
  queue.work[queue.next_push].gptr     = item->gptr;
  queue.work[queue.next_push].handle   = item->handle;
  
  queue.next_push = (queue.next_push+1)%MAXNUM_WORK_ITEMS;
  queue.nitems++;

  if( queue.nitems==1 ) {
    pthread_cond_signal( &(queue.cond_not_empty) );
  }
  pthread_mutex_unlock( &(queue.lock) );
}


void* dart_helper_thread(void *ptr)
{
  work_item_t item;

  while(1) {
    dart_work_queue_pop_item( &item);

    switch( item.selector ) {
    case WORK_NB_SEND:
      dart_helper_thread_send( &item );
      break;
    case WORK_NB_RECV:
      dart_helper_thread_recv( &item );
      break;
    case WORK_SHUTDOWN:
      pthread_exit(0);
      break;
    }
  }
}

void dart_helper_thread_send( work_item_t *item )
{
  void *buf;
  size_t nbytes;
  dart_team_t teamid;
  dart_team_unit_t dest;
    
  buf    = item->buf;
  nbytes = item->nbytes;
  teamid = item->team;
  dest   = item->unit;

  dart_shmem_send(buf, nbytes, teamid, dest);
}

void dart_helper_thread_recv( work_item_t *item )
{
  void *buf;
  size_t nbytes;
  dart_team_t teamid;
  dart_team_unit_t source;
    
  buf    = item->buf;
  nbytes = item->nbytes;
  teamid = item->team;
  source = item->unit;

  dart_shmem_recv(buf, nbytes, teamid, source);
}




#if 0

void* dart_producer(void *ptr)
{
  int i; 
  work_item_t item;

  for(i=0; i<1000000; i++ ) {
    item.selector=WORK_NB_SEND;
    item.value=i;
    push_item( &item);
  }
  
  item.selector=WORK_SHUTDOWN; 
  push_item( &item);
  
}

void* dart_consumer(void *ptr)
{
  int i; 
  work_item_t item;

  while(1) {
    pop_item( &item);
    if( item.selector==WORK_SHUTDOWN ) {
      pthread_exit(0);
    } 

    //fprintf(stdout, "Got item value=%d\n", item.value);
    //fflush(stdout);
  }
  
}


int main(int argc, char* argv[]) 
{
  pthread_t tid;
  
  dart_init_work_queue();

  pthread_create( &tid, 0, &dart_consumer, 0);

  dart_producer(0);

  pthread_join( tid, 0 );
}


#endif
