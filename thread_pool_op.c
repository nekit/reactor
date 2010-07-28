#include "thread_pool_op.h"
#include "event_queue_op.h"
#include "log.h"
#include "handle_event.h"
#include <memory.h>
#include <stdlib.h>

void * work ( void * arg ) {

  static int worker_idx = 1;
  INFO_MSG ( "worker %d started\n", worker_idx++ );

  thread_pool_t * tp_p  = arg;
  reactor_pool_t * rct_p_p = tp_p -> rct_pool_p;
  struct epoll_event ev;
  for ( ; ; ) {

    pop_event_queue ( &rct_p_p -> event_queue, &ev );
    tp_p -> handle_event ( &ev, rct_p_p );
  }  

  return NULL;
}

int init_thread_pool ( thread_pool_t * tp, int n, reactor_pool_t * rp ) {

  TRACE_MSG ( "thread poll initializing %d\n", n );
  
  tp -> n = n;
  tp -> rct_pool_p = rp;
  tp -> worker = malloc ( tp -> n * sizeof (pthread_t) );
  if ( NULL == tp -> worker ) {

    ERROR_MSG ( "memory problem: malloc\n" );
    return -1;
  }

  //
  tp -> handle_event = handle_event;

  return 0;  
}

int start_thread_pool ( thread_pool_t * tp ) {

  int i;
  for ( i = 0; i < tp -> n; ++i )
    if ( 0 != pthread_create ( &tp -> worker[i], NULL, work, (void *) tp ) ) {

      ERROR_MSG ( "failed to create worker %d\n", i + 1 );
      return -1;
    }

  DEBUG_MSG ( "thread pool started successfully\n" );
  
  return 0;
}

void free_thread_pool ( thread_pool_t * tp ) {

  free ( tp -> worker );
  tp -> worker = NULL;
  tp -> rct_pool_p = NULL;
}
