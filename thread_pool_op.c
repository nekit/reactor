#include "thread_pool_op.h"
#include "event_queue_op.h"
#include "log.h"
#include <memory.h>
#include <stdlib.h>

void * work ( void * arg ) {

  static int worker_idx = 1;
  TRACE_MSG ( "worker %d started\n", worker_idx++ );


  // TODO... O_o
  

  return NULL;
}

int init_thread_pool ( thread_pool_t * tp, int n, reactor_pool_t * rp ) {

  TRACE_MSG ( "thread poll initializing\n" );

  tp -> n = n;
  tp -> rct_pool_p = rp;
  tp -> worker = malloc ( tp -> n * sizeof (pthread_t) );
  if ( NULL == tp -> worker ) {

    ERROR_MSG ( "memory problem: malloc\n" );
    return -1;
  }

  tp -> epfd = epoll_create ( tp -> n );
  if ( -1 == tp -> epfd ) {

    ERROR_MSG ( "epoll create failed with n = %d\n", tp -> n );
    return -1;
  }

  return 0;  
}

int start_thread_pool ( thread_pool_t * tp ) {

  int i;
  for ( i = 0; i < tp -> n; ++i )
    if ( 0 != pthread_create ( &tp -> worker[i], NULL, work, (void *) tp -> rct_pool_p) ) {

      ERROR_MSG ( "failed to create worker %d\n", i + 1 );
      return -1;
    }

  
  return 0;
}

void free_thread_pool ( thread_pool_t * tp ) {

  free ( tp -> worker );
  tp -> worker = NULL;
  tp -> rct_pool_p = NULL;
}
